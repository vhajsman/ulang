#include "compiler.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace ULang {
    SymbolTable::SymbolTable(): nextSymbolId(16) {
        this->scope_global = new Scope;
        this->scope_global->_name = "<global>";
        this->scope_global->parent = nullptr;
        this->scope_global->nextOffset = 16;

        this->scope_current = this->scope_global;
    }

    SymbolTable::~SymbolTable() {
        /*
        Scope* s = this->scope_current;
        while(s) {
            Scope* parent = s->parent;
            delete s;

            s = parent;
        }
        */

        delete this->scope_global;
    }

    Scope* SymbolTable::enter(const std::string& name) {
        Scope* s = new Scope;
        s->_name = name;
        s->parent = this->scope_current;
        s->nextOffset = this->scope_current->nextOffset;

        this->scope_current = s;
        return s;
    }

    Scope* SymbolTable::leave() {
        if(this->scope_current->parent == nullptr)
            throw std::runtime_error("Cannot exit global scope");

        Scope* old = this->scope_current;
        this->scope_current = this->scope_current->parent;
        
        delete old;

        return this->scope_current;
    }

    Symbol* SymbolTable::decl(const std::string& name, const DataType* type, SourceLocation* loc, size_t align_head, size_t align_tail) {
        Symbol* sym = this->scope_current->decl(name, type, loc, align_head, align_tail);
        sym->symbolId = this->nextSymbolId++;

        std::cout << "decl: " << type->name << " " << name << " in scope '" << this->scope_current->_name << "' @ vstack: " << sym->stackOffset << std::endl;

        return sym;
    }

    const Symbol* SymbolTable::lookup(const std::string& name) const {
        return this->scope_current->lookup(name);
    }

    const Symbol* SymbolTable::lookup(unsigned int symbolId) const {
        return this->scope_current->lookup(symbolId);
    }

    Scope::~Scope() {
        
    }
    
    Scope* SymbolTable::getCurrentScope() const {
        return this->scope_current;
    }

    Scope* SymbolTable::getGlobalScope() const {
        return this->scope_global;
    }
}