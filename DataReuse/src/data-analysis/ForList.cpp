#include "ForOffload.h"


SourceLocation ForList::getForLoc() {
    return st->getForLoc();
}

std::vector<ValueDecl> &ForList::getMapIn() {
    return valIn;
}

std::vector<ValueDecl> &ForList::getMapOut() {
    return valOut;
}

std::vector<ValueDecl> &ForList::getMapInOut() {
    return valInOut;
}

void ForList::set_collapse(bool b) {
    collapse = b;
}

bool ForList::get_collapse() {
    return collapse;
}

bool ForList::is_within(SourceLocation src) {
    /*if(isBefore(st->getLocStart(), src) &&
            isBefore(src, st->getLocEnd()))*/
    if(isBefore(st->getBeginLoc(), src) &&
            isBefore(src, st->getEndLoc()))
        return true;
    return false;
}

void ForList::addVar(VarDecl *v) {
    var.push_back(*v);
}

void ForList::addMapOut(ValueDecl *v) {
    if(v == NULL) return;
    for(std::vector<ValueDecl>::iterator it = valOut.begin();
            it != valOut.end(); it++) {
        if(it->getLocation().getRawEncoding() == 
                v->getLocation().getRawEncoding()) 
            return; 
    }
    for(std::vector<VarDecl>::iterator it = var.begin();
            it != var.end(); it++) {
        if(v->getLocation().getRawEncoding() == 
                it->getLocation().getRawEncoding())
            return;
    }
    for(std::vector<ValueDecl>::iterator it = valIn.begin();
            it != valIn.end(); it++) {
        if(it->getLocation().getRawEncoding() == 
                v->getLocation().getRawEncoding()) {
            valInOut.push_back(*v);
            it = valIn.erase(it);
            return;
        }
    }
    valOut.push_back(*v);
}

void ForList::addMapIn(ValueDecl *v) {
    if(v == NULL) return;
    for(std::vector<ValueDecl>::iterator it = valIn.begin();
            it != valIn.end(); it++) {
        if(it->getLocation().getRawEncoding() == 
                v->getLocation().getRawEncoding()) 
            return; 
    }
    for(std::vector<VarDecl>::iterator it = var.begin();
            it != var.end(); it++) {
        if(v->getLocation().getRawEncoding() == 
                it->getLocation().getRawEncoding())
            return;
    }
    for(std::vector<ValueDecl>::iterator it = valOut.begin();
            it != valOut.end(); it++) {
        if(it->getLocation().getRawEncoding() == 
                v->getLocation().getRawEncoding()) {
            valInOut.push_back(*v);
            it = valOut.erase(it);
            return;
        }
    }
    valIn.push_back(*v);
}

SourceRange ForList::getRange() {
    return st->getSourceRange();
}

void ForList::dump() {
    llvm::errs() << "For loop at ";
    st->getForLoc().dump(*SM);
    llvm::errs() << "\nValIn\n";
    for(std::vector<ValueDecl>::iterator it = valIn.begin();
            it != valIn.end(); it++) {
        llvm::errs() << "|-- " << it->getNameAsString() << " (";
        it->getLocation().dump(*SM);
        llvm::errs() << ")\n";
    }
    llvm::errs() << "ValOut\n";
    for(std::vector<ValueDecl>::iterator it = valOut.begin();
            it != valOut.end(); it++) {
        llvm::errs() << "|-- " << it->getNameAsString() << " (";
        it->getLocation().dump(*SM);
        llvm::errs() << ")\n";
    }
    llvm::errs() << "Var\n";
    for(std::vector<VarDecl>::iterator it = var.begin();
            it != var.end(); it++) {
        llvm::errs() << "|-- " << it->getNameAsString() << " (";
        it->getLocation().dump(*SM);
        llvm::errs() << ")\n";
    }
    llvm::errs() << "\n";
}
