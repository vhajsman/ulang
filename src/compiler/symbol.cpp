#include "compiler.hpp"
#include <cstdint>
#include <cstring>
#include <stdexcept>

// TODO: implement symbol IDs

namespace ULang {
    Symbol* Scope::decl(const std::string& name, const DataType* type, SourceLocation* where, size_t align_head, size_t align_tail) {
        if(this->symbols.count(name) > 0)
            throw std::runtime_error("Variable already declared in '" + this->_name + "' scope: " + name);

        if(this->ci_ptr)
            this->ci_ptr->checkBuiltinRedecl(name, where);

        Symbol sym;
        sym.name = name;
        sym.symbolId = 0;
        sym.kind = SymbolKind::VARIABLE;
        sym.type = type;
        sym.stackOffset = this->nextOffset + align_head;
        sym.entry_ip = SIZE_MAX;

        if(where) 
            sym.where = *where;

        auto it = this->symbols.emplace(name, sym);
        this->nextOffset += type->size + align_tail;
        
        return &it.first->second;
    }

    Symbol* Scope::decl_fn(const std::string& name, const DataType* ret_type, SourceLocation* where, size_t align_head, size_t align_tail) {
        if(this->symbols.count(name) > 0)
            throw std::runtime_error("Function already declared in '" + this->_name + "' scope: " + name);

        if(this->ci_ptr)
            this->ci_ptr->checkBuiltinRedecl(name, where);

        Symbol sym;
        sym.name = name;
        sym.kind = SymbolKind::FUNCTION;
        sym.type = ret_type;
        sym.stackOffset = 0;
        sym.entry_ip = SIZE_MAX;

        if(where) 
            sym.where = *where;

        auto it = this->symbols.emplace(name, sym);
        this->nextOffset += ret_type->size + align_tail;
        
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