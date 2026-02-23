#include "bytecode.hpp"
#include "compiler/compiler.hpp"
#include "compiler/errno.h"
#include "types.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

#define cout_verbose        \
    if(this->cparams.verbose)    \
        std::cout

namespace ULang {
    /*
    void CompilerInstance::registerFunctions() {
        for(const auto& node: this->ast_owned) {
            ASTNode* n = node.get();
            if(n->type != ASTNodeType::FN_DEF)
                continue;

            if(this->symbols.lookup(n->name)) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "redefinition of '" + n->name + "' (function)",
                    n->symbol ? n->symbol->where : ULANG_LOCATION_NULL,
                    ULANG_SYNT_ERR_FN_REDEFINE
                );
            }

            Symbol* sym = this->symbols.decl(n->name, &TYPE_VOID, nullptr);
            sym->kind = SymbolKind::FUNCTION;
            sym->entry_ip = static_cast<uint32_t>(-1); // TODO: entry ip
            
            n->symbol = sym;
        }
    }
    */

    void CompilerInstance::compileFunction(ASTNode* node, std::vector<Instruction>& out) {
        if(!node->symbol)
            throw std::runtime_error("function symbol not found");

        node->symbol->entry_ip = out.size();
        this->verbose_nl("Compile function '" + node->symbol->name + "', entry_ip=");
        this->verbose_print(node->symbol->entry_ip);
        this->verbose_ascend();

        std::string scope_name = this->symbols.getCurrentScope()->_name + "::" + node->name + "@fn_decl";
        this->symbols.enter(scope_name);
        this->verbose_nl("Enter scope: " + scope_name);

        for(ASTNode* stmt: node->body)
            this->compileNode(stmt, out);
    
        bool termRet = !node->body.empty() && node->body.back()->type == ASTNodeType::FN_RET;
        if(node->symbol->type != &TYPE_VOID && !termRet) {
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error,
                "non-void function must return a value: '" + node->name + "'",
                node->symbol->where,
                ULANG_SYNT_ERR_FN_NO_RET
            );
        }

        this->verbose_descend();
    }
};

#undef cout_verbose