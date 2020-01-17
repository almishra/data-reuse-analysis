#ifndef DATAREUSE_H
#define DATAREUSE_H
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include <map>
#include <vector>

#include "Kernel.h"
#include "Loop.h"

using namespace llvm;

#define DEBUG false

/* RecursiveASTVisitor helps us visit any type of AST Node */
class DataReuseAnalysisVisitor :
    public RecursiveASTVisitor<DataReuseAnalysisVisitor> {
private:
    ASTContext *astContext;
    SourceManager *SM;
    bool insideLoop;
  //  Stmt *loop;
    SourceLocation loopEnd;
    FunctionDecl *currentFunction;
    Kernel *lastKernel;
    Loop *lastLoop;
    BeforeThanCompare<SourceLocation> isBefore;
    bool firstPrivate;

    /* Check is the given SourceLocation is within the last known kernel */
    bool isWithin(SourceLocation src);

    /* Check if the given variable is a private variable of the last
     * known kernel */
    bool isNotPrivate(ValueDecl* v);

    /* Template function to check the type of the Stmt */
    template <typename T> bool kernel_found(Stmt *st);

    /* Get the leftmost node of an Operator Stmt */
    ValueDecl *getLeftmostNode(Stmt *st);

public:
    explicit DataReuseAnalysisVisitor(CompilerInstance *CI);

    /* Get the SourceManager used while printing a SourceLocation */
    SourceManager *getSourceManager();

    /* Visitor fucntions for RecursiveASTVisitor */
    virtual bool VisitFunctionDecl(FunctionDecl *FD);
    virtual bool VisitDecl(Decl *decl);
    virtual bool VisitStmt(Stmt *st);
};

/* The ASTConsumer "consumes" (reads) the AST produced by the Clang parser */
class DataReuseAnalysisASTConsumer : public ASTConsumer {
    /* Path separator for Unix like filesystem */
    struct MatchPathSeparator {
        bool operator()(char ch) const { return ch == '/'; }
    };

    DataReuseAnalysisVisitor *visitor;

    /* For each kernel mark all values which need to be transferred both to and
     * from the device */
    void setInOut(Kernel *k);

    /* Get code generate to transfer data for a kernel */
    string getCode(Kernel* k);

    /* Get code generate to transfer data shared between kernel */
    string getSharedCode(map<int, vector<Kernel*>>::iterator m);
    
    /* Get code generate to transfer data for a kernel inside a loop */
    string getLoopCode(Loop* l);

    /* Get the basename of a file */
    string basename(string path);
    
    /* Identify kernels in the given code.
     * Currently targeting
     *   "target",
     *   "target parallel",
     *   "target parallel for",
     *   "target teams",
     *   "target teams distribute" and
     *   "target teams distribute parallel for"
     * directives for kernel */
    void getKernelInfo();
    
    /* Identify all kernels which are called from within a loop */
    void getLoopInfo();

    /* Identify kernels which are in close proximity to each other.
     * Currently kernels called from the same function are considered to be in
     * close proximity to each other */
    void getProximityInfo();

    /* Identity all data which is reused between kernels in close proximity to
     * each other */
    void getDataReuseInfo();

    /* Generate code for data transfer */
    void codeTransformation();

public:
    explicit DataReuseAnalysisASTConsumer(CompilerInstance *CI);
    virtual void HandleTranslationUnit(ASTContext &Context);
};

class DataReuseAnalysisPluginAction : public PluginASTAction {
protected:
    /* Function called by clang when it invokes our plugin */
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, 
                                              StringRef file);

    /* Function needed to parse custom command line arguments */
    bool ParseArgs(const CompilerInstance &CI, const vector<string> &args);
};

#endif
