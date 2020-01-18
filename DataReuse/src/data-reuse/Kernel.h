#ifndef DATAREUSE_KERNEL_H
#define DATAREUSE_KERNEL_H
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "Loop.h"

class Kernel {
    const int id;
    clang::Stmt *st;
    bool inLoop;
    Loop *loop;
    clang::FunctionDecl *FD;

    int link;

    std::set<clang::VarDecl*> privList;
    std::set<clang::ValueDecl*> valIn;
    std::set<clang::ValueDecl*> valOut;
    std::set<clang::ValueDecl*> valInOut;

    std::set<clang::ValueDecl*> sharedValIn;
    std::set<clang::ValueDecl*> sharedValOut;
    std::set<clang::ValueDecl*> sharedValInOut;

    clang::SourceLocation startLoc;
    clang::SourceLocation endLoc;

public:
    Kernel(int ID, clang::Stmt *stmt, clang::FunctionDecl *F) 
        : id(ID), 
          st(stmt), 
          FD(F) {
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
    Loop *getLoop();
    void setLoop(Loop *l);

    /* Getter/Setter for the function from which this kernel is called */
    clang::FunctionDecl *getFunction();
    void setFuction(clang::FunctionDecl *F);

    /* Get the Stmt of the kernel */
    clang::Stmt *getStmt();

    /* Add/Get/Remove private variables for this kernel */
    void addPrivate(clang::VarDecl *d);
    std::set<clang::VarDecl*> getPrivate();
    void removePrivate(clang::VarDecl *d);

    /* Add/Get/Remove variables which need to be transferred to the device 
     * for this kernel */
    void addValueIn(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValIn();
    void removeValueIn(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * for this kernel */
    void addValueOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValOut();
    void removeValueOut(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device for this kernel */
    void addValueInOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValInOut();
    void removeValueInOut(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred to the device 
     * and is shared between this kernel and some other kernel */
    void addSharedValueIn(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getSharedValIn();
    void removeSharedValueIn(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * and is shared between this kernel and some other kernel */
    void addSharedValueOut(clang::ValueDecl *d);
    void removeSharedValueOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getSharedValOut();
 
    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device and is shared between this kernel and some other kernel */
    void addSharedValueInOut(clang::ValueDecl *d);
    void removeSharedValueInOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getSharedValInOut();

    /* Getter/Setter for start and end source location of the kernel */
    void setStartLoc(clang::SourceLocation start);
    clang::SourceLocation getStartLoc();
    void setEndLoc(clang::SourceLocation end);
    clang::SourceLocation getEndLoc();

    void setLink(int id);
    int isLinkedTo();

    // Overloading operator < and > for sorting of keys in map
    bool operator< (const Kernel& k) const;
    bool operator> (const Kernel& k) const;
};

#endif
