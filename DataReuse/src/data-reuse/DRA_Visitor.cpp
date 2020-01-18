#include "DRA_Visitor.h"

using namespace std;
using namespace clang;

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

DataReuseAnalysisVisitor::DataReuseAnalysisVisitor(CompilerInstance *CI)
    : SM(&(CI->getASTContext().getSourceManager())),
      isBefore(*SM) {
    insideLoop = false;
    lastKernel = NULL;
    lastLoop = NULL;
    firstPrivate = false;
}

map<int, vector<Kernel*>> DataReuseAnalysisVisitor::getKernelMap() {
    return kernel_map;
}

map<int, vector<Loop*>> DataReuseAnalysisVisitor::getLoopMap() {
    return loop_map;
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
