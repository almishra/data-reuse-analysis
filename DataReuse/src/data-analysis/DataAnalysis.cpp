#include "ForOffload.h"

Rewriter rewriter;
std::vector<ForList> for_list;

class ForOffloadVisitor : public RecursiveASTVisitor<ForOffloadVisitor> {
private:
    ASTContext *astContext; //provides AST context info
    SourceManager *SM;

public:
    explicit ForOffloadVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())), 
          SM(&(astContext->getSourceManager())) { // initialize private members
            rewriter.setSourceMgr(*SM, astContext->getLangOpts());
            llvm::outs() << "Original File\n";
            rewriter.getEditBuffer(SM->getMainFileID()).write(llvm::outs());
            llvm::outs() << "******************************\n";
        }

    virtual bool VisitStmt(Stmt *st) {
        // Ignore if the statement is in System Header files
        if(!st->getBeginLoc().isValid() ||
                SM->isInSystemHeader(st->getBeginLoc()))
            return true;

        if(ForStmt *f = dyn_cast<ForStmt>(st)) {
            for(std::vector<ForList>::iterator it = for_list.begin();
                    it != for_list.end(); it++) {
                if(it->is_within(f->getBeginLoc()))
                    return true;
            }
            ForList fl(SM, f);
            if(CompoundStmt *b = dyn_cast<CompoundStmt>(f->getBody())) {
                if(dyn_cast<ForStmt>(b->body_front()))
                    fl.set_collapse(true);
            } else if(dyn_cast<ForStmt>(f->getBody()))
                fl.set_collapse(true);

            for_list.push_back(fl);
        } else if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(st)) {
            if(dyn_cast<VarDecl>(d->getDecl())) {
                for(std::vector<ForList>::iterator it = for_list.begin();
                        it != for_list.end(); it++) {
                    if(it->is_within(d->getBeginLoc())) {
                        it->addMapIn(d->getDecl());
                        break;
                    }
                }
            }
        } else if(BinaryOperator *b = dyn_cast<BinaryOperator>(st)) {
            if(b->isAssignmentOp()) {
                llvm::errs() << b->getOpcodeStr() << " b ";
                b->getOperatorLoc().dump(*SM);
                llvm::errs() << "\n";
                for(std::vector<ForList>::iterator it = for_list.begin();
                        it != for_list.end(); it++) {
                    if(it->is_within(b->getBeginLoc())) {
                        it->addMapOut(getLeftmostNode(st));
                        break;
                    }
                }
            } else {
                llvm::errs() << b->getOpcodeStr() << " b\n";
            }
        } else if(UnaryOperator *u = dyn_cast<UnaryOperator>(st)) {
            if(u->isPostfix() || u->isPrefix()) {
                llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode()) <<" u ";
                u->getOperatorLoc().dump(*SM);
                llvm::errs() << "\n";
                for(std::vector<ForList>::iterator it = for_list.begin();
                        it != for_list.end(); it++) {
                    if(it->is_within(u->getBeginLoc())) {
                        it->addMapOut(getLeftmostNode(st));
                        break;
                    }
                }
            } else {
                llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode()) <<" u\n";
            }
        } else if(ImplicitCastExpr *i = dyn_cast<ImplicitCastExpr>(st)) {
            llvm::errs() << i->getCastKindName() << " ";
            i->getBeginLoc().dump(*SM);
            llvm::errs() << "\n";
        }
        return true;
    }

    virtual bool VisitDecl(Decl *decl) {
        // Ignore if the declaration is in System Header files
        if(!decl->getLocation().isValid() ||
                SM->isInSystemHeader(decl->getLocation()))
            return true;

        if(VarDecl *v = dyn_cast<VarDecl>(decl)) {
            for(std::vector<ForList>::iterator it = for_list.begin();
                    it != for_list.end(); it++) {
                if(it->is_within(v->getLocation())) {
                    it->addVar(v);
                    break;
                }
            }
        }

        return RecursiveASTVisitor::VisitDecl(decl);
    }

private:
    // Function to get the location of the expanded code
    // If the code is a macro it'll expand the code and then return location
    SourceLocation getCodeLoc(SourceLocation src) {
        if(src.isFileID())
            return src;
        return SM->getExpansionLoc(src);
    }

    ValueDecl *getLeftmostNode(Stmt *st) {
        auto q = st;
        while(q != NULL) {
            Stmt* b = NULL;
            for(Stmt *a: q->children()) {
                if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(a)) {
                    llvm::errs() << "Leftmost node = " << d->getDecl()->getNameAsString() << "\n";
                    return d->getDecl();
                }
                b=a;
                break;
            }
            q=b;
        }
        return NULL;
    }


};

class LoopASTConsumer : public ASTConsumer {
private:
    struct MatchPathSeparator
    {
        bool operator()(char ch) const {
            return ch == '/';
        }
    };

    ForOffloadVisitor *visitor; // doesn't have to be private

    std::string get_code(ForList f) {
        std::string code = "";
        std::vector<ValueDecl> mapIn = f.getMapIn();
        std::vector<ValueDecl> mapOut = f.getMapOut();
        std::vector<ValueDecl> mapInOut = f.getMapInOut();
        if(mapIn.size() > 0 || mapOut.size() > 0 || mapInOut.size() > 0)
            code += "#pragma omp target data ";

        if(mapInOut.size() > 0) {
            code += "map(";
            for(std::vector<ValueDecl>::iterator it2 = mapInOut.begin();
                    it2 != mapInOut.end();it2++) {
                code += it2->getNameAsString();
                code += ",";
            }
            code.pop_back();
            code += ") ";
        }

        if(mapIn.size() > 0) {
            code += "map(to:";
            for(std::vector<ValueDecl>::iterator it2 = mapIn.begin();
                    it2 != mapIn.end();it2++) {
                code += it2->getNameAsString();
                code += ",";
            }
            code.pop_back();
            code += ") ";
        }
        if(mapOut.size() > 0) {
            code += "map(from:";
            for(std::vector<ValueDecl>::iterator it2 = mapOut.begin();
                    it2 != mapOut.end();it2++) {
                code += it2->getNameAsString();
                code += ",";
            }
            code.pop_back();
            code += ")";
        }

        if(mapIn.size() > 0 || mapOut.size() > 0 || mapInOut.size() > 0)
            code += "\n    ";
        code += "#pragma omp target teams distribute parallel for";
        if(f.get_collapse())
            code += " collapse(2)";
        code += "\n    ";
        return code;
    }

    std::string basename(std::string path) {
        return std::string( std::find_if(path.rbegin(), path.rend(),
                    MatchPathSeparator()).base(), path.end());
    }

public:
    explicit LoopASTConsumer(CompilerInstance *CI)
        : visitor(new ForOffloadVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());
        for(std::vector<ForList>::iterator it = for_list.begin();
                it != for_list.end(); ++it) {
            it->dump();
            rewriter.InsertTextBefore(it->getForLoc(), get_code(*it));
        }
        llvm::outs() << "******************************\n";
        llvm::outs() << "Modified File\n";
        FileID id = rewriter.getSourceMgr().getMainFileID();
        rewriter.getEditBuffer(id).write(llvm::outs());
        std::string filename = "/tmp/" +
            basename(rewriter.getSourceMgr().getFilename(rewriter.getSourceMgr().getLocForStartOfFile(id)).str());
        llvm::errs() << "Modified File at " << filename << "\n";
        std::error_code OutErrorInfo;
        std::error_code ok;
        llvm::raw_fd_ostream outFile(llvm::StringRef(filename),
                OutErrorInfo, llvm::sys::fs::F_None);
        if (OutErrorInfo == ok) {
            const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(id);
            outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
        } else {
            llvm::errs() << "Could not create file\n";
        }
    }
};


class LoopPluginAction : public PluginASTAction {
    protected:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                StringRef file) {
            return make_unique<LoopASTConsumer>(&CI);
        }
		bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
			return true;
		}
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<LoopPluginAction> 
X("-data-analysis", "Find if function called from another loop");
