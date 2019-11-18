#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <stack>
#include <map>
#include <vector>
#include <fstream>

using namespace std;
using namespace clang;
using namespace llvm;

Rewriter rewriter;

class Kernel {
    int id;
    Stmt *st;
    bool inLoop;
    Stmt *loop;
    FunctionDecl *FD;

    set<VarDecl*> privList;
    set<ValueDecl*> valIn;
    set<ValueDecl*> valOut;
    set<ValueDecl*> valInOut;

public:
    Kernel(int ID, Stmt *stmt, FunctionDecl *F) : id(ID), st(stmt), FD(F) {
        inLoop = false;
        loop = NULL;
    };

    int getID() { return id; }
    void setInLoop(bool in) { inLoop = in; }
    bool isInLoop() { return inLoop; }
    Stmt *getStmt() { return st; }
    Stmt *getLoop() { return loop; }
    void setLoop(Stmt *l) { loop = l; }
    FunctionDecl *getFunction() { return FD; }
    void setFuction(FunctionDecl *F) { FD = F; }
    void addPrivate(VarDecl *d) { privList.insert(d); }
    set<VarDecl*> getPrivate() { return privList; }
    void addValueIn(ValueDecl *d) { valIn.insert(d); }
    void removeValueIn(ValueDecl *d) { valIn.erase(d); }
    set<ValueDecl*> getValIn() { return valIn; }
    void addValueOut(ValueDecl *d) { valOut.insert(d); }
    void removeValueOut(ValueDecl *d) { valOut.erase(d); }
    set<ValueDecl*> getValOut() { return valOut; }
    void addValueInOut(ValueDecl *d) { valInOut.insert(d); }
    set<ValueDecl*> getValInOut() { return valInOut; }

    // Overloading operator < for sorting of keys in map
    bool operator< (const Kernel& k) const {
        if(k.id < this->id) return true;
        return false;
    }
};

map<Kernel*, vector<Kernel*>> kernel_map;

class KernelInfoVisitor : public RecursiveASTVisitor<KernelInfoVisitor> {
private:
    ASTContext *astContext; //provides AST context info
    SourceManager *SM;

    bool insideLoop;
    Stmt *loop;
    SourceLocation loopEnd;
    FunctionDecl *currentFunction;
    Kernel *lastKernel;
    BeforeThanCompare<SourceLocation> isBefore;
    bool firstPrivate;

    bool isWithin(SourceLocation src) {
        if(!lastKernel) return false;

        OMPExecutableDirective *Dir = dyn_cast<OMPExecutableDirective> (lastKernel->getStmt());
        CapturedStmt *cs;
        for(auto c : Dir->children()) {
            cs = dyn_cast<CapturedStmt>(c);
            //cs->dump();
            break;
        }
        if(!cs) return false;
        if(isBefore(lastKernel->getStmt()->getBeginLoc(), src) && 
                isBefore(src, cs->getEndLoc())) 
            return true;
        return false;
    }

    bool isNotPrivate(ValueDecl* v) {
        for(VarDecl* var: lastKernel->getPrivate()) {
            if(v == var) {
                //llvm::errs() << v->getNameAsString() << " is private\n";
                return false;
            }
        }
        return true;
    }

    template <typename T>
    bool kernel_found(Stmt *st)
    {
        auto cast_value = dyn_cast<T>(st);
        if (cast_value != nullptr) return true;

        return false;
    }

    ValueDecl *getLeftmostNode(Stmt *st) {
        Stmt* q = st;
        while(q != NULL) {
            Stmt* b = NULL;
            for(Stmt *a: q->children()) {
                if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(a)) {
                    //llvm::errs() << "  |- Leftmost node = " << d->getDecl()->getNameAsString() << "\n";
                    return d->getDecl();
                }
                b=a;
                break;
            }
            q=b;
        }
        return NULL;
    }

public:
    SourceManager *getSourceManager() { return SM; };

    explicit KernelInfoVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())), 
          SM(&(astContext->getSourceManager())),
          isBefore(*SM) { // initialize private members
            rewriter.setSourceMgr(*SM, astContext->getLangOpts());
            insideLoop = false;
            loop = NULL;
            lastKernel = NULL;
            firstPrivate = false;
            llvm::outs() << "Original File\n";
            rewriter.getEditBuffer(SM->getMainFileID()).write(llvm::outs());
            llvm::outs() << "******************************\n";
        }

    virtual bool VisitFunctionDecl(FunctionDecl *FD) {
        currentFunction = FD;
        return true;
    }

    virtual bool VisitDecl(Decl *decl) {
        // Ignore if the declaration is in System Header files
        if(!decl->getLocation().isValid() || 
                SM->isInSystemHeader(decl->getLocation()))
             return true;

        if(VarDecl *v = dyn_cast<VarDecl>(decl)) {
            if(isWithin(v->getLocation())) {
                lastKernel->addPrivate(v);
            }
        }

        return true;
    }

    virtual bool VisitStmt(Stmt *st) {
        // Ignore if the statement is in System Header files
        if(!st->getBeginLoc().isValid() ||
                SM->isInSystemHeader(st->getBeginLoc()))
            return true;

        if(insideLoop && isBefore(loopEnd, st->getBeginLoc())) {
            insideLoop = false;
        }

        bool found = false;
        found |= kernel_found<OMPTargetDirective>(st);
        found |= kernel_found<OMPTargetParallelDirective>(st);
        found |= kernel_found<OMPTargetParallelForDirective>(st);
        found |= kernel_found<OMPTargetTeamsDirective>(st);
        found |= kernel_found<OMPTargetTeamsDistributeDirective>(st);
        found |= kernel_found<OMPTargetTeamsDistributeParallelForDirective>(st);

        if(found) {
            int id = lastKernel ? lastKernel->getID() + 1 : 1;
            Kernel *k = new Kernel(id, st, currentFunction);
            lastKernel = k;
            if(insideLoop) {
                k->setInLoop(true);
                k->setLoop(loop);
            }
            vector<Kernel*> vec;
            kernel_map[k] = vec;
        } else {
            if(ForStmt *f = dyn_cast<ForStmt>(st)) {
                if(!insideLoop) {
                    loopEnd = f->getEndLoc();
                    loop = (Stmt*)(f);
                }
                insideLoop = true;
            } else if(WhileStmt *w = dyn_cast<WhileStmt>(st)) {
                if(!insideLoop) {
                    loopEnd = w->getEndLoc();
                    loop = (Stmt*)(w);
                }
                insideLoop = true;
            } else if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(st)) {
                if(!dyn_cast<clang::FunctionDecl>(d->getDecl())) {
                    if(ValueDecl *v = d->getDecl()) {
                        //llvm::errs() << "variable - " << v->getNameAsString() << "\n";
                        if(isWithin(d->getBeginLoc())) {
                            if(isNotPrivate(v)) { //&& !isOutVar(v)) {
                                lastKernel->addValueIn(v);
                            }
                        }
                    }
                }
            } else if(BinaryOperator *b = dyn_cast<BinaryOperator>(st)) {
                if(b->isAssignmentOp()) {
                    //llvm::errs() << b->getOpcodeStr() << " ";
                    //b->getOperatorLoc().dump(*SM);
                    if(isWithin(b->getBeginLoc())) {
                        ValueDecl *v = getLeftmostNode(st);
                        if(isNotPrivate(v)) lastKernel->addValueOut(v);
                    }
                }/* else {
                    llvm::errs() << b->getOpcodeStr() << "\n";
                }*/
            } else if(UnaryOperator *u = dyn_cast<UnaryOperator>(st)) {
                if(u->isPostfix() || u->isPrefix()) {
                    //llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode()) <<" u ";
                    //u->getOperatorLoc().dump(*SM);
                    if(isWithin(u->getBeginLoc())) {
                        ValueDecl *v = getLeftmostNode(st);
                        if(isNotPrivate(v)) lastKernel->addValueOut(v);
                    }
                }/* else {
                    llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode()) <<" u\n";
                }*/
            }
        }
        return true;
    }
};

class KernelInfoASTConsumer : public ASTConsumer {
    KernelInfoVisitor *visitor;
    void checkInOut(Kernel *k) {
        for(ValueDecl *in : k->getValIn()) {
            for(ValueDecl *out : k->getValOut()) {
                if(out == in) {
                    k->addValueInOut(out);
                    k->removeValueOut(out);
                    k->removeValueIn(in);
                }
            }
        }
    }

    string getCode(Kernel* k) {
        string code = "";
        if(k->getValIn().size() > 0 || k->getValOut().size() > 0 || k->getValInOut().size() > 0) {
            code += "#pragma omp target data ";

            if(k->getValIn().size() > 0) {
                code += "map(to:";
                for(ValueDecl *v: k->getValIn()) {
                    code += v->getNameAsString() + ",";
                }
                code.pop_back();
                code += ") ";
            }
            if(k->getValOut().size() > 0) {
                code += "map(from:";
                for(ValueDecl *v: k->getValOut()) {
                    code += v->getNameAsString() + ",";
                }
                code.pop_back();
                code += ") ";
            }
            if(k->getValInOut().size() > 0) {
                code += "map(";
                for(ValueDecl *v: k->getValInOut()) {
                    code += v->getNameAsString() + ",";
                }
                code.pop_back();
                code += ") ";
            }
            code.pop_back();
        }
        return code;
    }

    struct MatchPathSeparator
    {
        bool operator()(char ch) const {
            return ch == '/';
        }
    };

    string basename(string path) {
        return string(find_if(path.rbegin(), path.rend(), 
                    MatchPathSeparator()).base(), path.end());
    }


public:
    explicit KernelInfoASTConsumer(CompilerInstance *CI)
        : visitor(new KernelInfoVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());

        llvm::errs() << "Total " << kernel_map.size() << " kernels:\n";
        for(auto m : kernel_map) {
            checkInOut(m.first);
        }

        // Print kernel information
        int num = 1;
        for(auto m : kernel_map) {
            Kernel *k = m.first;
            llvm::errs() << "Kernel #" << k->getID() << "\n";
            llvm::errs() << " Function: " << k->getFunction()->getNameAsString() << "\n";
            llvm::errs() << " Location: ";
            k->getStmt()->getBeginLoc().print(llvm::errs(), *(visitor->getSourceManager()));
            llvm::errs() << "\n In Loop: ";
            if(k->isInLoop()) {
                llvm::errs() << "Yes ";
                k->getLoop()->getBeginLoc().print(llvm::errs(), *(visitor->getSourceManager()));
            } else {
                llvm::errs() << "No";
            }
            llvm::errs() << "\n";
            llvm::errs() << " Data used\n";
            if(k->getPrivate().size()) llvm::errs() << "  private:\n";
            for(VarDecl *v : k->getPrivate()) {
                llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
            }
            if(k->getValIn().size()) llvm::errs() << "  to:\n";
            for(ValueDecl *v : k->getValIn()) {
                llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
            }
            if(k->getValOut().size()) llvm::errs() << "  from:\n";
            for(ValueDecl *v : k->getValOut()) {
                llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
            }
            if(k->getValInOut().size()) llvm::errs() << "  tofrom:\n";
            for(ValueDecl *v : k->getValInOut()) {
                llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
            }
            llvm::errs() << "\n";
        }

        for(auto m : kernel_map) {
            Kernel *k = m.first;
            string code = getCode(k);
            //llvm::errs() << code << "\n";
            rewriter.InsertTextBefore(k->getStmt()->getBeginLoc().getLocWithOffset(-8), code+"\n");
        }

        FileID id = rewriter.getSourceMgr().getMainFileID();
        /*llvm::outs() << "******************************\n";
        llvm::outs() << "Modified File\n";
        rewriter.getEditBuffer(id).write(llvm::outs());*/
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

class KernelInfoPluginAction : public PluginASTAction {
    protected:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                StringRef file) {
            return make_unique<KernelInfoASTConsumer>(&CI);
        }

		bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
			return true;
		}
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<KernelInfoPluginAction> 
X("-kernel-info", "Locate all omp target kernels and collect all data info");
