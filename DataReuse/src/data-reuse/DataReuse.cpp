#include "DataReuse.h"
#include "clang/Rewrite/Core/Rewriter.h"

#define DEBUG false

Rewriter rewriter;
map<int, vector<Kernel*>> kernel_map;
map<int, vector<Loop*>> loop_map;

bool DataReuseAnalysisVisitor::isWithin(SourceLocation src) {
    if(!lastKernel) return false;

    OMPExecutableDirective *Dir =
        dyn_cast<OMPExecutableDirective> (lastKernel->getStmt());
    CapturedStmt *cs;
    for(auto c : Dir->children()) {
        cs = dyn_cast<CapturedStmt>(c);
        if(DEBUG) cs->dump();
        break;
    }
    if(!cs) return false;
    if(isBefore(lastKernel->getStmt()->getBeginLoc(), src) &&
            isBefore(src, cs->getEndLoc()))
        return true;
    return false;
}

bool DataReuseAnalysisVisitor::isNotPrivate(ValueDecl* v) {
    for(VarDecl* var: lastKernel->getPrivate()) {
        if(v == var) {
            if(DEBUG) llvm::errs() << v->getNameAsString() << " is private\n";
            return false;
        }
    }
    return true;
}

template <typename T>
bool DataReuseAnalysisVisitor::kernel_found(Stmt *st) {
    auto cast_value = dyn_cast<T>(st);
    if (cast_value != nullptr) return true;

    return false;
}

ValueDecl *DataReuseAnalysisVisitor::getLeftmostNode(Stmt *st) {
    Stmt* q = st;
    while(q != NULL) {
        Stmt* b = NULL;
        for(Stmt *a: q->children()) {
            if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(a)) {
                if(DEBUG) {
                    llvm::errs() << "  |- Leftmost node = ";
                    llvm::errs() << d->getDecl()->getNameAsString();
                    llvm::errs() << "\n";
                }
                return d->getDecl();
            }
            b=a;
            break;
        }
        q=b;
    }
    return NULL;
}

SourceManager *DataReuseAnalysisVisitor::getSourceManager() { return SM; };

DataReuseAnalysisVisitor::DataReuseAnalysisVisitor(CompilerInstance *CI)
    : astContext(&(CI->getASTContext())),
      SM(&(astContext->getSourceManager())),
      isBefore(*SM) {
    rewriter.setSourceMgr(*SM, astContext->getLangOpts());
    insideLoop = false;
    //loop = NULL;
    lastKernel = NULL;
    lastLoop = NULL;
    firstPrivate = false;
    llvm::outs() << "Original File\n";
    rewriter.getEditBuffer(SM->getMainFileID()).write(llvm::outs());
    llvm::outs() << "******************************\n";
}

bool DataReuseAnalysisVisitor::VisitFunctionDecl(FunctionDecl *FD) {
    currentFunction = FD;
    return true;
}

bool DataReuseAnalysisVisitor::VisitDecl(Decl *decl) {
    // Ignore if the declaration is in System Header files
    if(!decl->getLocation().isValid() ||
            SM->isInSystemHeader(decl->getLocation())) {
        return true;
    }

    if(VarDecl *v = dyn_cast<VarDecl>(decl)) {
        if(isWithin(v->getLocation())) {
            lastKernel->addPrivate(v);
        }
    }

    return true;
}

bool DataReuseAnalysisVisitor::VisitStmt(Stmt *st) {
    // Ignore if the statement is in System Header files
    if(!st->getBeginLoc().isValid() ||
            SM->isInSystemHeader(st->getBeginLoc()))
        return true;

    if(insideLoop && isBefore(loopEnd, st->getBeginLoc())) {
        insideLoop = false;
    }

    bool found = false;
    // Currently targeting
    //   "target",
    //   "target parallel",
    //   "target parallel for",
    //   "target teams",
    //   "target teams distribute" and
    //   "target teams distribute parallel for"
    // directives for kernel
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
            lastLoop->addKernel(k->getID());
            k->setLoop(lastLoop);
        }
        vector<Kernel*> vec;
        vec.push_back(k);
        kernel_map[id] = vec;
    } else {
        if(dyn_cast<ForStmt>(st) || dyn_cast<WhileStmt>(st)) {
            int loopID = lastLoop ? lastLoop->getID() + 1 : 1;
            Loop *l = new Loop(loopID, st);
            if(!insideLoop) {
                loopEnd = st->getEndLoc();
                l->setStartLoc(st->getBeginLoc());
                l->setEndLoc(loopEnd);
                lastLoop = l;
            } else {
                l->setStartLoc(lastLoop->getStartLoc());
                l->setEndLoc(lastLoop->getEndLoc());
            }
            insideLoop = true;
            vector<Loop*> vec;
            vec.push_back(l);
            loop_map[loopID] = vec;
        } else if(DeclRefExpr *d = dyn_cast<DeclRefExpr>(st)) {
            if(!dyn_cast<clang::FunctionDecl>(d->getDecl())) {
                if(ValueDecl *v = d->getDecl()) {
                    if(DEBUG) {
                        llvm::errs() << "variable - ";
                        llvm::errs() << v->getNameAsString();
                        llvm::errs() << "\n"; // DEBUG
                    }
                    if(isWithin(d->getBeginLoc())) {
                        if(isNotPrivate(v)) {
                            if(lastKernel->isInLoop())
                                lastKernel->getLoop()->addValueIn(v);
                            else
                                lastKernel->addValueIn(v);
                        }
                    }
                }
            }
        } else if(BinaryOperator *b = dyn_cast<BinaryOperator>(st)) {
            if(b->isAssignmentOp()) {
                if(DEBUG) {
                    llvm::errs() << b->getOpcodeStr() << " ";
                    b->getOperatorLoc().dump(*SM);
                }
                if(isWithin(b->getBeginLoc())) {
                    ValueDecl *v = getLeftmostNode(st);
                    if(isNotPrivate(v)) {
                        if(lastKernel->isInLoop())
                            lastKernel->getLoop()->addValueIn(v);
                        else
                            lastKernel->addValueOut(v);
                    }
                }
            } else {
                if(DEBUG) {
                    llvm::errs() << b->getOpcodeStr();
                    llvm::errs() << "\n";
                }
            }
        } else if(UnaryOperator *u = dyn_cast<UnaryOperator>(st)) {
            if(u->isPostfix() || u->isPrefix()) {
                if(DEBUG) {
                    llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode());
                    llvm::errs() <<" u ";
                    u->getOperatorLoc().dump(*SM);
                }
                if(isWithin(u->getBeginLoc())) {
                    ValueDecl *v = getLeftmostNode(st);
                    if(isNotPrivate(v)) {
                        if(lastKernel->isInLoop())
                            lastKernel->getLoop()->addValueIn(v);
                        else
                            lastKernel->addValueOut(v);
                    }
                }
            } else {
                if(DEBUG) {
                    llvm::errs() << UnaryOperator::getOpcodeStr(u->getOpcode());
                    llvm::errs() <<" u\n";
                }
            }
        }
    }
    return true;
}

void DataReuseAnalysisASTConsumer::setInOut(Kernel *k) {
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

string DataReuseAnalysisASTConsumer::getLoopCode(Loop* l) {
    string code = "";
    if(l->getValIn().size() > 0
            || l->getValOut().size() > 0
            || l->getValInOut().size() > 0) {
        code += "#pragma omp target data ";

        if(l->getValIn().size() > 0) {
            code += "map(to:";
            for(ValueDecl *v: l->getValIn()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        if(l->getValOut().size() > 0) {
            code += "map(from:";
            for(ValueDecl *v: l->getValOut()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        if(l->getValInOut().size() > 0) {
            code += "map(tofrom:";
            for(ValueDecl *v: l->getValInOut()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        code.pop_back();
    }
    return code;
}

string DataReuseAnalysisASTConsumer::getCode(Kernel* k) {
    string code = "";
    if(k->getValIn().size() > 0
            || k->getValOut().size() > 0
            || k->getValInOut().size() > 0) {
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
            code += "map(tofrom:";
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

string DataReuseAnalysisASTConsumer::getSharedCode(map<int, vector<Kernel*>>::iterator m) {
    string code = "";
    Kernel *k = m->second.at(0);
    vector<Kernel*> vec = m->second;
    if(k->getSharedValIn().size() > 0
            || k->getSharedValOut().size() > 0
            || k->getSharedValInOut().size() > 0) {
        code += "#pragma omp target data ";
        if(k->getSharedValIn().size() > 0) {
            code += "map(to:";
            for(ValueDecl *v: k->getSharedValIn()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        if(k->getSharedValOut().size() > 0) {
            code += "map(from:";
            for(ValueDecl *v: k->getSharedValOut()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        if(k->getSharedValInOut().size() > 0) {
            code += "map(tofrom:";
            for(ValueDecl *v: k->getSharedValInOut()) {
                code += v->getNameAsString() + ",";
            }
            code.pop_back();
            code += ") ";
        }
        code.pop_back();

        OMPExecutableDirective *Dir =
                        dyn_cast<OMPExecutableDirective>(vec.back()->getStmt());
        CapturedStmt *cs;
        for(auto c : Dir->children()) {
            cs = dyn_cast<CapturedStmt>(c);
            break;
        }
        k->setEndLoc(cs->getEndLoc());
    }
    return code;
}

string DataReuseAnalysisASTConsumer::basename(string path) {
    return string(find_if(path.rbegin(), path.rend(),
                MatchPathSeparator()).base(), path.end());
}

void DataReuseAnalysisASTConsumer::getKernelInfo() {
    // Print kernel information
    llvm::errs() << "Kernel Data Information\n";
    llvm::errs() << "Total " << kernel_map.size() << " kernels:\n";
    for(auto m = kernel_map.begin(); m != kernel_map.end(); m++) {
        int id = m->first;
        Kernel *k = m->second.at(0);
        llvm::errs() << "Kernel #" << k->getID() << "\n";
        llvm::errs() << " Function: " << k->getFunction()->getNameAsString();
        llvm::errs() << "\n Location: ";
        k->getStmt()->getBeginLoc().print(llvm::errs(),
                                          *(visitor->getSourceManager()));
        llvm::errs() << "\n In Loop: ";
        if(k->isInLoop()) {
            llvm::errs() << "Yes ";
            k->getLoop()->getStartLoc().print(llvm::errs(),
                                              *(visitor->getSourceManager()));
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
        if(k->getSharedValIn().size()) llvm::errs() << "  shared to:\n";
        for(ValueDecl *v : k->getSharedValIn()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
        if(k->getSharedValOut().size()) llvm::errs() << "  shared from:\n";
        for(ValueDecl *v : k->getSharedValOut()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
        if(k->getSharedValInOut().size()) llvm::errs() << "  shared tofrom:\n";
        for(ValueDecl *v : k->getSharedValInOut()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
        llvm::errs() << "\n";
    }
}

void DataReuseAnalysisASTConsumer::getLoopInfo() {
    llvm::errs() << "Loop Information\n";
    for(auto m : kernel_map) {
        Kernel *k = m.second.at(0);
        if(k->isInLoop()) {
            m.second.push_back(k);
            k->setLink(k->getID());
            k->setStartLoc(k->getLoop()->getStartLoc());
            k->setEndLoc(k->getLoop()->getEndLoc());
        } else {
            k->setStartLoc(k->getStmt()->getBeginLoc());
            OMPExecutableDirective *Dir =
                                dyn_cast<OMPExecutableDirective>(k->getStmt());
            CapturedStmt *cs;
            for(auto c : Dir->children()) {
                cs = dyn_cast<CapturedStmt>(c);
                break;
            }
            k->setEndLoc(cs->getEndLoc());
        }

        llvm::errs() << "Kernel #" << k->getID();
        if(k->isInLoop()) llvm::errs() << "  -- In Loop";
        llvm::errs() << "\nStart: ";
        k->getStartLoc().print(llvm::errs(), *(visitor->getSourceManager()));
        llvm::errs() << "\nEnd  : ";
        k->getEndLoc().print(llvm::errs(), *(visitor->getSourceManager()));
        llvm::errs() << "\n";
        llvm::errs() << "\n";
    }
    llvm::errs() << "********************************\n";
    for(auto m : loop_map) {
        Loop *l = m.second.at(0);
        llvm::errs() << "Loop ID: " << l->getID() << " - ";
        for(int id : l->getKernels()) {
            llvm::errs() << id << ",";
        }
        llvm::errs() << "\nStart Loc: ";
        l->getStartLoc().print(llvm::errs(), *(visitor->getSourceManager()));
        llvm::errs() << "\nEnd Loc: ";
        l->getEndLoc().print(llvm::errs(), *(visitor->getSourceManager()));
        llvm::errs() << "\n";
        if(l->getValIn().size()) llvm::errs() << "  to:\n";
        for(ValueDecl *v : l->getValIn()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
        if(l->getValOut().size()) llvm::errs() << "  from:\n";
        for(ValueDecl *v : l->getValOut()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
        if(l->getValInOut().size()) llvm::errs() << "  tofrom:\n";
        for(ValueDecl *v : l->getValInOut()) {
            llvm::errs() << "   |-  " << v->getNameAsString() << "\n";
        }
    }
}

void DataReuseAnalysisASTConsumer::getProximityInfo() {
    llvm::errs() << "Proximity Check\n";
    for(map<int, vector<Kernel*>>::iterator m1 = kernel_map.begin();
            m1 != kernel_map.end(); m1++) {
        vector<Kernel*> vec1 = m1->second;
        Kernel *k1 = vec1.at(0);
        for(map<int, vector<Kernel*>>::iterator m2 = kernel_map.find(m1->first);
                m2 != kernel_map.end(); m2++) {
            if(m1->first == m2->first) continue;

            vector<Kernel*> vec2 = m2->second;
            Kernel *k2 = vec2.at(0);
            if(DEBUG) {
                llvm::errs() << k1->getFunction()->getNameInfo().getAsString();
                llvm::errs() << " ";
                llvm::errs() << k2->getFunction()->getNameInfo().getAsString();
                llvm::errs() << "\n";
            }
            if(k1->getFunction() == k2->getFunction()) {
                //llvm::errs() << k1->isLinkedTo() << "\n";
                if(k1->isLinkedTo()) {
                    map<int, vector<Kernel*>>::iterator kLink =
                                              kernel_map.find(k1->isLinkedTo());
                    kLink->second.push_back(k2);
                    k2->setLink(kLink->first);
                } else {
                    //llvm::errs() << "--------";
                    //for(Kernel* v : vec1) {
                    //    llvm::errs() << v->getID() << " ";
                    //}
                    //llvm::errs() << "\n";
                    vec1.push_back(k2);
                    kernel_map[m1->first] = vec1;
                    k2->setLink(k1->getID());
                    //llvm::errs() << "--------";
                    //for(Kernel* v : vec1) {
                    //    llvm::errs() << v->getID() << " ";
                    //}
                    //llvm::errs() << "\n";
                }
            }
        }
    }

    for(auto m : kernel_map) {
        Kernel *k = m.second.at(0);
        vector<Kernel*> vec = m.second;
        llvm::errs() << k->getFunction()->getNameInfo().getAsString() << "\n";
        llvm::errs() << k->getID() << " ";

        for(Kernel* v : vec) {
            llvm::errs() << v->getID() << " ";
        }
        llvm::errs() << "\n";
    }
}

void DataReuseAnalysisASTConsumer::getDataReuseInfo() {
    llvm::errs() << "Data Reuse Info\n";
    for(auto m : kernel_map) {
        Kernel *k1 = m.second.at(0);
        // compare to
        /*set<ValueDecl*> k1In = k1->getValIn();
          for(Kernel *k2 : m.second) {
          set<ValueDecl*> k2In = k2->getValIn();
          set<ValueDecl*> intersect;
          set_intersection(k1In.begin(), k1In.end(),
          k2In.begin(), k2In.end(),
          inserter(intersect, intersect.begin()));
          }*/
        // compare from with to
        set<ValueDecl*> k1Out = k1->getValOut();
        for(int i=1; i< m.second.size(); i++) {
            Kernel *k2 = m.second.at(i);
            //llvm::errs() << k1->getID() << " " << k2->getID() << "\n";
            if(k1==k2) continue;
            set<ValueDecl*> k2In = k2->getValIn();
            set<ValueDecl*> intersect;
            set_intersection(k1Out.begin(), k1Out.end(),
                    k2In.begin(), k2In.end(),
                    inserter(intersect, intersect.begin()));
            for(auto i : intersect) {
                //llvm::errs() << i->getNameAsString() << "\n";
                k1->removeValueOut(i);
                k1->removeValueIn(i);
                k1->addSharedValueIn(i);
                k2->removeValueIn(i);
            }
        }
    }
}

void DataReuseAnalysisASTConsumer::codeTransformation() {
    for(map<int, vector<Kernel*>>::iterator m = kernel_map.begin();
            m != kernel_map.end(); m++) {
        Kernel *k = m->second.at(0);
        string code = getCode(k);
        string shared_code = getSharedCode(m);

        SourceLocation loc;
        //llvm::errs() << "Shared: " << shared_code << "\n";
        //llvm::errs() << "Code: " << code << "\n";

        if(code != "") {
            loc = k->getStmt()->getBeginLoc().getLocWithOffset(-8);
            rewriter.InsertTextBefore(loc, code+"\n");
        }
        if(shared_code != "") {
            loc = k->getStartLoc().getLocWithOffset(-8);
            rewriter.InsertTextBefore(loc, shared_code+"\n{\n");
            loc = k->getEndLoc().getLocWithOffset(2);
            rewriter.InsertTextAfter(loc, "\n}\n");
        }
    }

    for(map<int, vector<Loop*>>::iterator m = loop_map.begin();
            m != loop_map.end(); m++) {
        Loop *l = m->second.at(0);
        string code = "";
        if(l->getKernels().size() > 0) {
            code = getLoopCode(l);
        }

        if(code != "") {
            SourceLocation loc = l->getStartLoc();//.getLocWithOffset(-1);
            rewriter.InsertTextBefore(loc, code+"\n");
        }
    }


    FileID id = rewriter.getSourceMgr().getMainFileID();
    /*llvm::outs() << "******************************\n";
      llvm::outs() << "Modified File\n";
      rewriter.getEditBuffer(id).write(llvm::outs());*/
    std::string filename = "/tmp/" +
        basename(rewriter.getSourceMgr()
                         .getFilename(rewriter.getSourceMgr()
                                              .getLocForStartOfFile(id))
                         .str());
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

DataReuseAnalysisASTConsumer::DataReuseAnalysisASTConsumer(CompilerInstance *CI)
: visitor(new DataReuseAnalysisVisitor(CI)) // initialize the visitor
{ }

void DataReuseAnalysisASTConsumer::HandleTranslationUnit(ASTContext &Context) {
    visitor->TraverseDecl(Context.getTranslationUnitDecl());

    //llvm::errs() << "Press Enter to continue\n";
    //getchar();
    // Kernel Information
    getKernelInfo();
    //llvm::errs() << "Press Enter to continue\n";
    //getchar();

    // Check loop
    getLoopInfo();
    //llvm::errs() << "Press Enter to continue\n";
    //getchar();

    // Proximity Check
    getProximityInfo();
    //llvm::errs() << "Press Enter to continue\n";
    //getchar();

    // Data Reuse Information
    getDataReuseInfo();

    for(auto m : kernel_map) {
        setInOut(m.second.at(0));
    }
    getKernelInfo();

    // Get code
    codeTransformation();
}

unique_ptr<ASTConsumer> DataReuseAnalysisPluginAction::CreateASTConsumer(
        CompilerInstance &CI,
        StringRef file) {
    return make_unique<DataReuseAnalysisASTConsumer>(&CI);
}

bool DataReuseAnalysisPluginAction::ParseArgs(const CompilerInstance &CI,
        const vector<string> &args) {
    return true;
}

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<DataReuseAnalysisPluginAction>
X("-data-reuse", "Data Reuse Analysis");
