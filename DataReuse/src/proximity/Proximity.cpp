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

class FunctionInfo {
    FunctionDecl *funcDecl;
    unsigned ID;
    bool kernel;

    public:
    FunctionInfo(FunctionDecl *f, unsigned id): 
        funcDecl(f), ID(id), kernel(false) {}
    void setKernelFunction(bool k) { kernel = k; }
    bool isKernelFunction() { return kernel; }
    FunctionDecl *getFuncDecl() { return funcDecl; }
    unsigned getID() { return ID; }

    bool operator==(FunctionInfo &other)
    { 
        return (ID == other.getID());
    }

    std::size_t operator()(FunctionInfo& k)
    {
      using std::size_t;
      using std::hash;
      using std::string;

      // Compute individual hash values for FunctionDecl
      // and ID and combine them using XOR and bit shifting

      return (hash<unsigned>()(k.getFuncDecl()->getODRHash()))
               ^ (hash<unsigned>()(k.getID() << 1));
    }
};

class KernelInfo {
    Stmt *kernel;
    FunctionDecl *func;
    int id;
    
    public:
    KernelInfo(int i, Stmt *st, FunctionDecl *f) : id(i), kernel(st), func(f) {}
    FunctionDecl *getFunction() { return func; }
    Stmt *getKernel() { return kernel; }
    int getID() { return id; }
};

//std::vector<KernelInfo> kernelList;
std::map<FunctionInfo*, std::vector<KernelInfo>> proximityList;

class ProximityVisitor : public RecursiveASTVisitor<ProximityVisitor> {
private:
    ASTContext *astContext; //provides AST context info
    SourceManager *SM;

    FunctionInfo *currentFunction;
    unsigned funcID;

    template <typename T>
    bool kernel_found(Stmt *st)
    {
        auto cast_value = dyn_cast<T>(st);
        if (cast_value != nullptr) {
            return true;
        }
        return false;
    }

    int kernelID;

public:
    explicit ProximityVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())), 
          SM(&(astContext->getSourceManager())) { // initialize private members
            rewriter.setSourceMgr(*SM, astContext->getLangOpts());
            kernelID = 0;
            funcID = 0;
            /*llvm::outs() << "Original File\n";
            rewriter.getEditBuffer(SM->getMainFileID()).write(llvm::outs());
            llvm::outs() << "******************************\n";*/
        }

    virtual bool VisitFunctionDecl(FunctionDecl *FD) {
        /*llvm::errs() << "Function : " << FD->getNameAsString() << "\nStart - ";
        FD->getSourceRange().getBegin().print(llvm::errs(), *SM);
        llvm::errs() << "\nEnd - ";
        FD->getSourceRange().getEnd().print(llvm::errs(), *SM);
        llvm::errs() << "\n";
        llvm::errs() << "\n";*/

        currentFunction = new FunctionInfo(FD, funcID++);
        proximityList.insert(
                pair<FunctionInfo*, std::vector<KernelInfo>>
                (currentFunction, std::vector<KernelInfo>())
                );
        
        return true;
    }

    virtual bool VisitStmt(Stmt *st) {
        bool found = false;
        found |= kernel_found<OMPTargetDirective>(st);
        found |= kernel_found<OMPTargetParallelDirective>(st);
        found |= kernel_found<OMPTargetParallelForDirective>(st);
        found |= kernel_found<OMPTargetTeamsDirective>(st);
        found |= kernel_found<OMPTargetTeamsDistributeDirective>(st);
        found |= kernel_found<OMPTargetTeamsDistributeParallelForDirective>(st);

        if(found) {
            /*llvm::errs() << "Kernel found at ";
            st->getBeginLoc().print(llvm::errs(), *SM);
            llvm::errs() << " in function - " 
                         << currentFunction->getFuncDecl()->getNameAsString();
            llvm::errs() << "\n";*/

            KernelInfo k(kernelID++, st, currentFunction->getFuncDecl());
            //kernelList.push_back(k);

            proximityList[currentFunction].push_back(k);
        }

        return true;
    }
};

class ProximityASTConsumer : public ASTConsumer {
    ProximityVisitor *visitor;
public:
    explicit ProximityASTConsumer(CompilerInstance *CI)
        : visitor(new ProximityVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());

       /* for (std::vector<KernelInfo>::iterator it = kernelList.begin() ; 
                     it != kernelList.end(); ++it) {
            llvm::errs() << "Kernel ID: " << it->getID();
            llvm::errs() << " in function : " << it->getFunction()->getNameAsString() << "\n";
        }*/

        for (std::map<FunctionInfo*, std::vector<KernelInfo>>::iterator it = 
                proximityList.begin(); it != proximityList.end(); it++) {
            FunctionInfo *f = it->first;
            std::vector<KernelInfo> k = it->second;
            llvm::errs() << f->getFuncDecl()->getNameAsString() << ":\n";
            for (std::vector<KernelInfo>::iterator it = k.begin() ; 
                    it != k.end(); ++it) {
                llvm::errs() << "Kernel ID: " << it->getID();
                llvm::errs() << " in function : " 
                             << it->getFunction()->getNameAsString() << "\n";
            }
            llvm::errs() << "\n";
        }
        llvm::errs() << "\n";
    }
};

class ProximityPluginAction : public PluginASTAction {
    protected:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                StringRef file) {
            return make_unique<ProximityASTConsumer>(&CI);
        }

		bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
			return true;
		}
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<ProximityPluginAction> 
X("-find-near-kernel", "Locate kernels called from same function");
