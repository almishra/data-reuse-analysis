#include "Kernel.h"

int Kernel::getID() const {
    return id;
}

void Kernel::setInLoop(bool in) {
    inLoop = in;
}

bool Kernel::isInLoop() {
    return inLoop;
}

Stmt* Kernel::getStmt() {
    return st;
}

Stmt* Kernel::getLoop() {
    return loop;
}

void Kernel::setLoop(Stmt *l) {
    loop = l;
}

FunctionDecl* Kernel::getFunction() {
    return FD;
}

void Kernel::setFuction(FunctionDecl *F) {
    FD = F;
}

void Kernel::addPrivate(VarDecl *d) {
    privList.insert(d);
}

set<VarDecl*> Kernel::getPrivate() {
    return privList;
}

void Kernel::addValueIn(ValueDecl *d) {
    valIn.insert(d);
}

void Kernel::removeValueIn(ValueDecl *d) {
    valIn.erase(d);
}

set<ValueDecl*> Kernel::getValIn() {
    return valIn;
}

void Kernel::addValueOut(ValueDecl *d) {
    valOut.insert(d);
}

void Kernel::removeValueOut(ValueDecl *d) {
    valOut.erase(d);
}

set<ValueDecl*> Kernel::getValOut() {
    return valOut;
}

void Kernel::addValueInOut(ValueDecl *d) {
    valInOut.insert(d);
}

void Kernel::removeValueInOut(ValueDecl *d) {
    valInOut.erase(d);
}

set<ValueDecl*> Kernel::getValInOut() {
    return valInOut;
}

void Kernel::addSharedValueIn(ValueDecl *d) {
    sharedValIn.insert(d);
}

void Kernel::removeSharedValueIn(ValueDecl *d) {
    sharedValIn.erase(d);
}

set<ValueDecl*> Kernel::getSharedValIn() {
    return sharedValIn;
}

void Kernel::addSharedValueOut(ValueDecl *d) {
    sharedValOut.insert(d);
}

void Kernel::removeSharedValueOut(ValueDecl *d) {
    sharedValOut.erase(d);
}

set<ValueDecl*> Kernel::getSharedValOut() {
    return sharedValOut;
}

void Kernel::addSharedValueInOut(ValueDecl *d) {
    sharedValInOut.insert(d);
}

void Kernel::removeSharedValueInOut(ValueDecl *d) {
    sharedValInOut.erase(d);
}

set<ValueDecl*> Kernel::getSharedValInOut() {
    return sharedValInOut;
}

void Kernel::setStartLoc(SourceLocation start) {
    startLoc = start;
}

SourceLocation Kernel::getStartLoc() {
    return startLoc;
}

void Kernel::setEndLoc(SourceLocation end) {
    endLoc = end;
}

SourceLocation Kernel::getEndLoc() {
    return endLoc;
}

void Kernel::setLink(int id) {
    link = id;
}

int Kernel::isLinkedTo() {
    return link;
}

bool Kernel::operator< (const Kernel& k) const {
    return this->id < k.id;
}
bool Kernel::operator> (const Kernel& k) const {
    return this->id > k.id;
}

