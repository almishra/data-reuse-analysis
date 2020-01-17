#ifndef DATAREUSE_LOOP_H
#define DATAREUSE_LOOP_H
#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace std;
using namespace clang;

class Loop {
    const int id;
    Stmt *st;
    set<int> kernels;

    set<VarDecl*> privList;
    set<ValueDecl*> valIn;
    set<ValueDecl*> valOut;
    set<ValueDecl*> valInOut;

    SourceLocation startLoc;
    SourceLocation endLoc;

public:
    Loop(int ID, Stmt *stmt) : id(ID), st(stmt) {};

    /* Get ID for this loop. Every loop has a unique ID number which is
     * set in the constructor */
    int getID() const;

    /* Get the Stmt of the loop*/
    Stmt *getStmt();

    /* Add kernel ID for kernels inside the loop */
    void addKernel(int k_id);

    /* Get list of all kernels inside the loop */
    set<int> getKernels();

    /* Add/Get/Remove private variables for this loop */
    void addPrivate(VarDecl *d);
    set<VarDecl*> getPrivate();
    void removePrivate(VarDecl *d);

    /* Add/Get/Remove variables which need to be transferred to the device 
     * for this loop */
    void addValueIn(ValueDecl *d);
    set<ValueDecl*> getValIn();
    void removeValueIn(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * for this loop */
    void addValueOut(ValueDecl *d);
    set<ValueDecl*> getValOut();
    void removeValueOut(ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device for this loop */
    void addValueInOut(ValueDecl *d);
    set<ValueDecl*> getValInOut();
    void removeValueInOut(ValueDecl *d);

    /* Getter/Setter for start and end source location of the loop */
    void setStartLoc(SourceLocation start);
    SourceLocation getStartLoc();
    void setEndLoc(SourceLocation end);
    SourceLocation getEndLoc();

    // Overloading operator < and > for sorting of keys in map
    bool operator< (const Loop& l) const;
    bool operator> (const Loop& l) const;
};

#endif
