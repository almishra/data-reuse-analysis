#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
//#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <stack>
#include <map>
#include <vector>
#include <fstream>

using namespace std;
using namespace llvm;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

class ForList {
private:
    SourceManager *SM;
    BeforeThanCompare<SourceLocation> isBefore;
    ForStmt *st;
    std::vector<VarDecl> var;
    std::vector<ValueDecl> valIn;
    std::vector<ValueDecl> valOut;
    std::vector<ValueDecl> valInOut;
    bool collapse = false;

public:
    ForList(SourceManager *sm, ForStmt *s): SM(sm), isBefore(*SM), st(s) {}

    SourceLocation getForLoc();
    std::vector<ValueDecl> &getMapIn();
    std::vector<ValueDecl> &getMapOut();
    std::vector<ValueDecl> &getMapInOut();
    void set_collapse(bool b) ;
    bool get_collapse();
    bool is_within(SourceLocation src);
    void addVar(VarDecl *v);
    void addMapOut(ValueDecl *v);
    void addMapIn(ValueDecl *v);
    SourceRange getRange();
    void dump();
};


