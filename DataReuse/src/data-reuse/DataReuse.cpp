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
#include <algorithm>

using namespace std;
using namespace clang;
using namespace llvm;

Rewriter rewriter;

class Kernel {
    const int id;
    Stmt *st;
    bool inLoop;
    Stmt *loop;
    FunctionDecl *FD;

    int link;

    set<VarDecl*> privList;
    set<ValueDecl*> valIn;
    set<ValueDecl*> valOut;
    set<ValueDecl*> valInOut;

    set<ValueDecl*> sharedValIn;
    set<ValueDecl*> sharedValOut;
    set<ValueDecl*> sharedValInOut;

    SourceLocation startLoc;
    SourceLocation endLoc;

public:
    Kernel(int ID, Stmt *stmt, FunctionDecl *F) : id(ID), st(stmt), FD(F) {
        inLoop = false;
        loop = NULL;
        link = 0;
    };

    int getID() const { return id; }
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
    void removeValueInOut(ValueDecl *d) { valInOut.erase(d); }
    set<ValueDecl*> getValInOut() { return valInOut; }

    void addSharedValueIn(ValueDecl *d) { sharedValIn.insert(d); }
    void removeSharedValueIn(ValueDecl *d) { sharedValIn.erase(d); }
    set<ValueDecl*> getSharedValIn() { return sharedValIn; }
    void addSharedValueOut(ValueDecl *d) { sharedValOut.insert(d); }
    void removeSharedValueOut(ValueDecl *d) { sharedValOut.erase(d); }
    set<ValueDecl*> getSharedValOut() { return sharedValOut; }
    void addSharedValueInOut(ValueDecl *d) { sharedValInOut.insert(d); }
    void removeSharedValueInOut(ValueDecl *d) { sharedValInOut.erase(d); }
    set<ValueDecl*> getSharedValInOut() { return sharedValInOut; }

    void setStartLoc(SourceLocation start) { startLoc = start; }
    SourceLocation getStartLoc() { return startLoc; }
    void setEndLoc(SourceLocation end) { endLoc = end; }
    SourceLocation getEndLoc() { return endLoc; }

    void setLink(int id) { link = id; }
    int isLinkedTo() { return link; }

    // Overloading operator < for sorting of keys in map
    bool operator< (const Kernel& k) const {
        return this->id < k.id;
    }
    bool operator> (const Kernel& k) const {
        return this->id > k.id;
    }
};

//map<Kernel*, vector<Kernel*>> kernel_map;
map<int, vector<Kernel*>> kernel_map;

class DataReuseAnalysisVisitor : public RecursiveASTVisitor<DataReuseAnalysisVisitor> {
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

    explicit DataReuseAnalysisVisitor(CompilerInstance *CI)
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
            vec.push_back(k);
            kernel_map[id] = vec;
            for(auto m = kernel_map.begin(); m != kernel_map.end(); m++) {
                int mid = m->first;
                llvm::errs() << "--- " << mid << "\n";
            }
            llvm::errs() << "***\n";
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

class DataReuseAnalysisASTConsumer : public ASTConsumer {
    DataReuseAnalysisVisitor *visitor;
    void setInOut(Kernel *k) {
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

    string getSharedCode(map<int, vector<Kernel*>>::iterator m) {
        string code = "";
        Kernel *k = m->second.at(0);
        vector<Kernel*> vec = m->second;
        if(k->getSharedValIn().size() > 0 || k->getSharedValOut().size() > 0 || k->getSharedValInOut().size() > 0) {
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

            OMPExecutableDirective *Dir = dyn_cast<OMPExecutableDirective>(vec.back()->getStmt());
            CapturedStmt *cs;
            for(auto c : Dir->children()) {
                cs = dyn_cast<CapturedStmt>(c);
                break;
            }
            k->setEndLoc(cs->getEndLoc());
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

    void getKernelInfo() {
        // Print kernel information
        llvm::errs() << "Kernel Data Information\n";
        llvm::errs() << "Total " << kernel_map.size() << " kernels:\n";
        for(auto m = kernel_map.begin(); m != kernel_map.end(); m++) {
            int id = m->first;
            Kernel *k = m->second.at(0);
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

    void getLoopInfo() {
        llvm::errs() << "Loop Information\n";
        for(auto m : kernel_map) {
            Kernel *k = m.second.at(0);
            if(k->isInLoop()) {
                m.second.push_back(k);
                k->setLink(k->getID());
                k->setStartLoc(k->getLoop()->getBeginLoc());
                k->setEndLoc(k->getLoop()->getEndLoc());
            } else {
                k->setStartLoc(k->getStmt()->getBeginLoc());
                OMPExecutableDirective *Dir = dyn_cast<OMPExecutableDirective>(k->getStmt());
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
    }

#if 0
    void getProximityInfo() {
        llvm::errs() << "Proximity Check\n";
        for(map<Kernel*, vector<Kernel*>>::iterator m1 = kernel_map.begin(); m1 != kernel_map.end(); m1++) {
            Kernel *k1 = m1->first;
            for(map<Kernel*, vector<Kernel*>>::iterator m2 = kernel_map.find(k1); m2 != kernel_map.end(); m2++) {
                Kernel *k2 = m2->first;
                if(k1 == k2) continue;

                if(k1->getFunction() == k2->getFunction()) {
                    if(k1->isLinkedTo()) {
                        map<Kernel*, vector<Kernel*>>::iterator kLink = kernel_map.find(k1->isLinkedTo());
                        kLink->second.push_back(k2);
                        k2->setLink(kLink->first);
                    } else {
                        m1->second.push_back(k2);
                        k2->setLink(k1);
                    }
                }
            }
        }
        for(map<Kernel*, vector<Kernel*>>::iterator m = kernel_map.begin(); m != kernel_map.end(); m++) {
            Kernel *k = m->first;
            /*llvm::errs() << k->getID() << " is linked to ";
            if(k->isLinkedTo())
                llvm::errs() << k->isLinkedTo()->getID() << "\n";
            else
                llvm::errs() << "None\n";*/
            if(k->isLinkedTo() && k->isLinkedTo() != k)
                kernel_map.erase(m);
        }
        for(auto m : kernel_map) {
            Kernel *k = m.first;
            vector<Kernel*> vec = m.second;
            llvm::errs() << k->getFunction()->getNameInfo().getAsString() << "\n";
            llvm::errs() << k->getID() << " ";

            for(Kernel* v : vec) {
                llvm::errs() << v->getID() << " ";
            }
            llvm::errs() << "\n";
        }
    }
#endif

    void getProximityInfo() {
        llvm::errs() << "Proximity Check\n";
        for(map<int, vector<Kernel*>>::iterator m1 = kernel_map.begin(); m1 != kernel_map.end(); m1++) {
            vector<Kernel*> vec1 = m1->second;
            Kernel *k1 = vec1.at(0);
            for(map<int, vector<Kernel*>>::iterator m2 = kernel_map.find(m1->first); m2 != kernel_map.end(); m2++) {
                if(m1->first == m2->first) continue;

                vector<Kernel*> vec2 = m2->second;
                Kernel *k2 = vec2.at(0);
                //llvm::errs() << k1->getFunction()->getNameInfo().getAsString() << " " << k2->getFunction()->getNameInfo().getAsString() << "\n";
                if(k1->getFunction() == k2->getFunction()) {
                    //llvm::errs() << k1->isLinkedTo() << "\n";
                    if(k1->isLinkedTo()) {
                        map<int, vector<Kernel*>>::iterator kLink = kernel_map.find(k1->isLinkedTo());
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

    void getDataReuseInfo() {
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
            //for(vector<Kernel*>::iterator k = m.second.begin(); k != m.second.end(); k++) {
            for(int i=1; i< m.second.size(); i++) {
                Kernel *k2 = m.second.at(i);
                llvm::errs() << k1->getID() << " " << k2->getID() << "\n";
                if(k1==k2) continue;
                set<ValueDecl*> k2In = k2->getValIn();
                set<ValueDecl*> intersect;
                set_intersection(k1Out.begin(), k1Out.end(), 
                                          k2In.begin(), k2In.end(), 
                                          inserter(intersect, intersect.begin()));
                for(auto i : intersect) {
                    llvm::errs() << i->getNameAsString() << "\n";
                    k1->removeValueOut(i);
                    k1->removeValueIn(i);
                    k1->addSharedValueIn(i);
                    k2->removeValueIn(i);
                }
            }
        }
    }

    void codeTransformation() {
        for(map<int, vector<Kernel*>>::iterator m = kernel_map.begin(); m != kernel_map.end(); m++) {
            Kernel *k = m->second.at(0);
            string code = getCode(k);
            string shared_code = getSharedCode(m);
            //llvm::errs() << "Shared: " << shared_code << "\n";
            //llvm::errs() << "Code: " << code << "\n";
            if(code != "")
                rewriter.InsertTextBefore(k->getStmt()->getBeginLoc().getLocWithOffset(-8), code+"\n");
            if(shared_code != "") {
                rewriter.InsertTextBefore(k->getStartLoc().getLocWithOffset(-8), shared_code+"\n{\n");
                rewriter.InsertTextAfter(k->getEndLoc().getLocWithOffset(2), "\n}\n");
            }

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

public:
    explicit DataReuseAnalysisASTConsumer(CompilerInstance *CI)
        : visitor(new DataReuseAnalysisVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());

        // Kernel Information
        getKernelInfo();
        getchar();

        // Check loop
        getLoopInfo();
        getchar();

        // Proximity Check
        getProximityInfo();
        getchar();

        // Data Reuse Information
        getDataReuseInfo();

        for(auto m : kernel_map) {
            setInOut(m.second.at(0));
        }
        getKernelInfo();

        // Get code
        codeTransformation();
    }
};

class DataReuseAnalysisPluginAction : public PluginASTAction {
    protected:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                StringRef file) {
            return make_unique<DataReuseAnalysisASTConsumer>(&CI);
        }

		bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
			return true;
		}
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<DataReuseAnalysisPluginAction> 
X("-data-reuse", "Data Reuse Analysis");
