#include "compiler.hpp"

namespace ULang {
    bool isBuiltin(const std::string& str) {
        for(std::string& curr: builtin_ids)
            if(str == curr) return true;

        return false;
    }

    void CompilerInstance::checkBuiltinRedecl(const std::string& str, SourceLocation* loc) {
            if(isBuiltin((std::string&) str) && !this->cparams.excludeBuiltin) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "'" + str + "' was already declared as builtin symbol",
                    loc == nullptr ? ULANG_LOCATION_NULL : *loc,
                    ULANG_SYNT_ERR_BUILTIN_REDECL
                );
            }
        }
};