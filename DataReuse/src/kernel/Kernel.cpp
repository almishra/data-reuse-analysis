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

class KernelVisitor : public RecursiveASTVisitor<KernelVisitor> {
private:
    ASTContext *astContext; //provides AST context info
    SourceManager *SM;

    template <typename T>
    bool kernel_found(Stmt *st)
    {
        auto cast_value = dyn_cast<T>(st);
        if (cast_value != nullptr) {
            /*llvm::errs() << "Kernel found at - ";
            cast_value->getBeginLoc().print(llvm::errs(), *SM);
            llvm::errs() << "\n";*/
            return true;
        }
        return false;
    }

public:
    explicit KernelVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())), 
          SM(&(astContext->getSourceManager())) { // initialize private members
            rewriter.setSourceMgr(*SM, astContext->getLangOpts());
            /*llvm::outs() << "Original File\n";
            rewriter.getEditBuffer(SM->getMainFileID()).write(llvm::outs());
            llvm::outs() << "******************************\n";*/
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
            llvm::errs() << "Kernel found at ";
            st->getBeginLoc().print(llvm::errs(), *SM);
            llvm::errs() << "\n";
        }

        return true;
    }
};

class KernelASTConsumer : public ASTConsumer {
    KernelVisitor *visitor;
public:
    explicit KernelASTConsumer(CompilerInstance *CI)
        : visitor(new KernelVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());
    }
};

class KernelPluginAction : public PluginASTAction {
    protected:
        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                StringRef file) {
            return make_unique<KernelASTConsumer>(&CI);
        }

		bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
			return true;
		}
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<KernelPluginAction> 
X("-find-kernel", "Locate all omp target kernels");
