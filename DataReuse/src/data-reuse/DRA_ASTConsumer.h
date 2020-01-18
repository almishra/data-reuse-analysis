#ifndef DRA_ASTCONSUMER_H
#define DRA_ASTCONSUMER_H

#include "DRA_Visitor.h"

/* The ASTConsumer "consumes" (reads) the AST produced by the Clang parser */
class DataReuseAnalysisASTConsumer : public clang::ASTConsumer {
    /* Path separator for Unix like filesystem */
    struct MatchPathSeparator {
        bool operator()(char ch) const { return ch == '/'; }
    };

    DataReuseAnalysisVisitor *visitor;
    clang::Rewriter rewriter;
    clang::SourceManager *SM;

    std::map<int, std::vector<Kernel*>> kernel_map;
    std::map<int, std::vector<Loop*>> loop_map;

    /* For each kernel mark all values which need to be transferred both to and
     * from the device */
    void setInOut(Kernel *k);

    /* Get code generate to transfer data for a kernel */
    std::string getCode(Kernel* k);

    /* Get code generate to transfer data shared between kernel */
    std::string getSharedCode(std::map<int, std::vector<Kernel*>>::iterator m);
    
    /* Get code generate to transfer data for a kernel inside a loop */
    std::string getLoopCode(Loop* l);

    /* Get the basename of a file */
    std::string basename(std::string path);
    
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
    explicit DataReuseAnalysisASTConsumer(clang::CompilerInstance *CI);
    virtual void HandleTranslationUnit(clang::ASTContext &Context);
};

#endif
