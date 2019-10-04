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

class LoopVisitor : public RecursiveASTVisitor<LoopVisitor> {
private:
    ASTContext *astContext; //provides AST context info
    SourceManager *SM;
    
    bool insideLoop;
    SourceLocation loopEnd;
    BeforeThanCompare<SourceLocation> isBefore;

    template <typename T>
    bool kernel_found(Stmt *st)
    {
        auto cast_value = dyn_cast<T>(st);
        if (cast_value != nullptr) {
            return true;
        }
        return false;
    }

public:
    explicit LoopVisitor(CompilerInstance *CI)
        : astContext(&(CI->getASTContext())), 
          SM(&(astContext->getSourceManager())),
          isBefore(*SM) { // initialize private members
            rewriter.setSourceMgr(*SM, astContext->getLangOpts());
            insideLoop = false;
        }

    virtual bool VisitStmt(Stmt *st) {
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
            llvm::errs() << "Kernel found at ";
            st->getBeginLoc().print(llvm::errs(), *SM);
            if(insideLoop) {
                llvm::errs() << "  -- It is inside a loop";
            }
            llvm::errs() << "\n";
        } else {
            if(ForStmt *f = dyn_cast<ForStmt>(st)) {
                llvm::errs() << "For ends at ";
                f->getEndLoc().print(llvm::errs(), *SM);
                llvm::errs() << "\n";
                if(!insideLoop)
                    loopEnd = f->getEndLoc();
                insideLoop = true;
            } else if(WhileStmt *w = dyn_cast<WhileStmt>(st)) {
                llvm::errs() << "While ends at ";
                w->getEndLoc().print(llvm::errs(), *SM);
                llvm::errs() << "\n";
                if(!insideLoop)
                    loopEnd = w->getEndLoc();
                insideLoop = true;
            }
        }

        return true;
    }
};

class LoopASTConsumer : public ASTConsumer {
    LoopVisitor *visitor;
public:
    explicit LoopASTConsumer(CompilerInstance *CI)
        : visitor(new LoopVisitor(CI)) // initialize the visitor
    { }

    virtual void HandleTranslationUnit(ASTContext &Context) {
        visitor->TraverseDecl(Context.getTranslationUnitDecl());
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
X("-find-loop", "Find whether a kernel is called from a loop");
