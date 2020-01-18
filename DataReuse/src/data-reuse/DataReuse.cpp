#include "DRA_ASTConsumer.h"

using namespace std;
using namespace clang;
using namespace llvm;

class DataReuseAnalysisPluginAction : public PluginASTAction {
protected:
    /* Function called by clang when it invokes our plugin */
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
            StringRef file) {
        return make_unique<DataReuseAnalysisASTConsumer>(&CI);
    }

    /* Function needed to parse custom command line arguments */
    bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
        return true;
    }
};

/*register the plugin and its invocation command in the compilation pipeline*/
static FrontendPluginRegistry::Add<DataReuseAnalysisPluginAction>
X("-data-reuse", "Data Reuse Analysis");
