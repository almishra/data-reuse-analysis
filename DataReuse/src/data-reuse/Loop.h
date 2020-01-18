#ifndef DATAREUSE_LOOP_H
#define DATAREUSE_LOOP_H
#include "clang/Frontend/FrontendPluginRegistry.h"

class Loop {
    const int id;
    clang::Stmt *st;
    std::set<int> kernels;

    std::set<clang::VarDecl*> privList;
    std::set<clang::ValueDecl*> valIn;
    std::set<clang::ValueDecl*> valOut;
    std::set<clang::ValueDecl*> valInOut;

    clang::SourceLocation startLoc;
    clang::SourceLocation endLoc;

public:
    Loop(int ID, clang::Stmt *stmt) : id(ID), st(stmt) {};

    /* Get ID for this loop. Every loop has a unique ID number which is
     * set in the constructor */
    int getID() const;

    /* Get the Stmt of the loop*/
    clang::Stmt *getStmt();

    /* Add kernel ID for kernels inside the loop */
    void addKernel(int k_id);

    /* Get list of all kernels inside the loop */
    std::set<int> getKernels();

    /* Add/Get/Remove private variables for this loop */
    void addPrivate(clang::VarDecl *d);
    std::set<clang::VarDecl*> getPrivate();
    void removePrivate(clang::VarDecl *d);

    /* Add/Get/Remove variables which need to be transferred to the device 
     * for this loop */
    void addValueIn(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValIn();
    void removeValueIn(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred from the device 
     * for this loop */
    void addValueOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValOut();
    void removeValueOut(clang::ValueDecl *d);

    /* Add/Get/Remove variables which need to be transferred both to and from 
     * the device for this loop */
    void addValueInOut(clang::ValueDecl *d);
    std::set<clang::ValueDecl*> getValInOut();
    void removeValueInOut(clang::ValueDecl *d);

    /* Getter/Setter for start and end source location of the loop */
    void setStartLoc(clang::SourceLocation start);
    clang::SourceLocation getStartLoc();
    void setEndLoc(clang::SourceLocation end);
    clang::SourceLocation getEndLoc();

    // Overloading operator < and > for sorting of keys in map
    bool operator< (const Loop& l) const;
    bool operator> (const Loop& l) const;
};

#endif
