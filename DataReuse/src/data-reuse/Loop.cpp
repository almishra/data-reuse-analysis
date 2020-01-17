#include "Loop.h"

int Loop::getID() const {
    return id;
}

Stmt* Loop::getStmt() {
    return st;
}

void Loop::addKernel(int k_id) {
    kernels.insert(k_id);
}

set<int> Loop::getKernels() {
    return kernels;
}

void Loop::addPrivate(VarDecl *d) {
    privList.insert(d);
}

set<VarDecl*> Loop::getPrivate() {
    return privList;
}

void Loop::removePrivate(VarDecl *d) {
    privList.erase(d);
}

void Loop::addValueIn(ValueDecl *d) {
    valIn.insert(d);
}

void Loop::removeValueIn(ValueDecl *d) {
    valIn.erase(d);
}

set<ValueDecl*> Loop::getValIn() {
    return valIn;
}

void Loop::addValueOut(ValueDecl *d) {
    valOut.insert(d);
}

void Loop::removeValueOut(ValueDecl *d) {
    valOut.erase(d);
}

set<ValueDecl*> Loop::getValOut() {
    return valOut;
}

void Loop::addValueInOut(ValueDecl *d) {
    valInOut.insert(d);
}

void Loop::removeValueInOut(ValueDecl *d) {
    valInOut.erase(d);
}

set<ValueDecl*> Loop::getValInOut() {
    return valInOut;
}

void Loop::setStartLoc(SourceLocation start) {
    startLoc = start;
}

SourceLocation Loop::getStartLoc() {
    return startLoc;
}

void Loop::setEndLoc(SourceLocation end) {
    endLoc = end;
}

SourceLocation Loop::getEndLoc() {
    return endLoc;
}

bool Loop::operator< (const Loop& l) const {
    return this->id < l.id;
}

bool Loop::operator> (const Loop& l) const {
    return this->id > l.id;
}
