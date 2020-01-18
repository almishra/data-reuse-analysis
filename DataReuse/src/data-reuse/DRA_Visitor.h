#ifndef DRA_VISITOR_H
#define DRA_VISITOR_H
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <map>
#include <vector>

#include "Kernel.h"
#include "Loop.h"

#define DEBUG false

/* RecursiveASTVisitor helps us visit any type of AST Node */
class DataReuseAnalysisVisitor :
    public clang::RecursiveASTVisitor<DataReuseAnalysisVisitor> {
private:
    clang::SourceManager *SM;
    bool insideLoop;
    std::map<int, std::vector<Kernel*>> kernel_map;
    std::map<int, std::vector<Loop*>> loop_map;

    clang::SourceLocation loopEnd;
    clang::FunctionDecl *currentFunction;
    Kernel *lastKernel;
    Loop *lastLoop;
    clang::BeforeThanCompare<clang::SourceLocation> isBefore;
    bool firstPrivate;

    /* Check is the given SourceLocation is within the last known kernel */
    bool isWithin(clang::SourceLocation src);

    /* Check if the given variable is a private variable of the last
     * known kernel */
    bool isNotPrivate(clang::ValueDecl* v);

    /* Template function to check the type of the Stmt */
    template <typename T> bool kernel_found(clang::Stmt *st);

    /* Get the leftmost node of an Operator Stmt */
    clang::ValueDecl *getLeftmostNode(clang::Stmt *st);

public:
    explicit DataReuseAnalysisVisitor(clang::CompilerInstance *CI);

    std::map<int, std::vector<Kernel*>> getKernelMap();
    std::map<int, std::vector<Loop*>> getLoopMap();

    /* Visitor fucntions for RecursiveASTVisitor */
    virtual bool VisitFunctionDecl(clang::FunctionDecl *FD);
    virtual bool VisitDecl(clang::Decl *decl);
    virtual bool VisitStmt(clang::Stmt *st);
};

#endif
