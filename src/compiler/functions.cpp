#include "compiler/compiler.hpp"
#include "compiler/errno.h"
#include "types.hpp"
#include <cstddef>
#include <cstdint>

#define cout_verbose        \
    if(this->cparams.verbose)    \
        std::cout

namespace ULang {
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
};

#undef cout_verbose