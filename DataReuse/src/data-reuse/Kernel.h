#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace std;
using namespace clang;

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

    /* Get ID for this kernel. Every kernel has a unique ID number which is
     * set in the constructor */
    int getID() const;

    /* Getter/Setter that kernel is inside a loop */
    bool isInLoop();
    void setInLoop(bool in);

    /* Getter/Setter for the Stmt of the loop this kernel is inside */
    Stmt *getLoop();
    void setLoop(Stmt *l);

    /* Getter/Setter for the function from which this kernel is called */
    FunctionDecl *getFunction();
    void setFuction(FunctionDecl *F);

    /* Get the Stmt of the kernel */
    Stmt *getStmt();

    /* Add/Get/Remove private variables for this kernel */
    void addPrivate(VarDecl *d);
    set<VarDecl*> getPrivate();

    /* Add/Get/Remove variables which need to be transferred to the device 
     * for this kernel */
    void addValueIn(ValueDecl *d);
    set<ValueDecl*> getValIn();
    void removeValueIn(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * for this kernel */
    void addValueOut(ValueDecl *d);
    set<ValueDecl*> getValOut();
    void removeValueOut(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device for this kernel */
    void addValueInOut(ValueDecl *d);
    set<ValueDecl*> getValInOut();
    void removeValueInOut(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred to the device 
     * and is shared between this kernel and some other kernel */
    void addSharedValueIn(ValueDecl *d);
    set<ValueDecl*> getSharedValIn();
    void removeSharedValueIn(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * and is shared between this kernel and some other kernel */
    void addSharedValueOut(ValueDecl *d);
    void removeSharedValueOut(ValueDecl *d);
    set<ValueDecl*> getSharedValOut();
 
    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device and is shared between this kernel and some other kernel */
    void addSharedValueInOut(ValueDecl *d);
    void removeSharedValueInOut(ValueDecl *d);
    set<ValueDecl*> getSharedValInOut();

    /* Getter/Setter for start and end source location of the kernel */
    void setStartLoc(SourceLocation start);
    SourceLocation getStartLoc();
    void setEndLoc(SourceLocation end);
    SourceLocation getEndLoc();

    void setLink(int id);
    int isLinkedTo();

    // Overloading operator < and > for sorting of keys in map
    bool operator< (const Kernel& k) const;
    bool operator> (const Kernel& k) const;
};
