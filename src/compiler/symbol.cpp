#include "compiler.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace ULang {
    Symbol* Scope::decl(const std::string& name, const DataType* type, SourceLocation* where, size_t align_head, size_t align_tail) {
        if(this->symbols.count(name) > 0)
            throw std::runtime_error("Variable already declared in '" + this->_name + "' scope: " + name);

        // TODO: implement symbol IDs

        Symbol sym {
            name,
            0,
            type,
            this->nextOffset + align_head
        };

        if(where) {
            // std::memcpy((void*) &sym.where, where, sizeof(SourceLocation));
            sym.where = *where;
        }

        auto it = this->symbols.emplace(name, sym);
        this->nextOffset += type->size + align_tail;
        
        return &it.first->second;
    }

    const Symbol* Scope::lookup(const std::string& name) const {
        auto it = this->symbols.find(name);

        if(it != this->symbols.end()) 
            return &it->second;
        if(this->parent)
            return this->parent->lookup(name);
        
        return nullptr;
    }

    const Symbol* Scope::lookup(unsigned int symbolId) const {
        for(const auto& [K, V] : this->symbols) {
            THROW_AWAY K;
            if(V.symbolId == symbolId)
                return &V;
        }

        if(this->parent)
            return this->parent->lookup(symbolId);

        return nullptr;
    }
};