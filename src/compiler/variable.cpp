#include "compiler.hpp"

namespace ULang {
    ASTNode* CompilerInstance::parseVarDecl() {
        // type
        Token tok_type = this->expectToken(TokenType::TypeKeyword);
        const DataType* type = resolveDataType(tok_type.text);

        // name (identifier)
        Token tok_name = this->expectToken(TokenType::Identifier);

        // symbol
        Symbol* sym = this->symbols.decl(tok_name.text, type, &tok_name.loc);

        this->verbose_nl("Creating ASTNode for decl: '" + tok_name.text + "'");
        this->verbose_ascend();

        ASTNode* node = new ASTNode(ASTNodeType::DECLARATION);
        node->name = tok_name.text;
        node->symbol = sym;

        // optional initializer
        if(this->tokens[this->pos].type == TokenType::Assign) {
            this->pos++;
            node->initial = this->parseExpression();

            if(node->initial) {
                const DataType* init_type = getType(node->initial, sym->where);
                if(init_type) {
                    if((init_type->flags & SIGN) != (type->flags & SIGN)) {
                        this->friendlyException(CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Warning,
                            "Types '" + init_type->name + "' and '" + type->name + "' differ in signedness",
                            sym->where,
                            ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                        ));
                    }
    
                    if(init_type->size != type->size) {
                        this->friendlyException(CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Warning,
                            "Types '" + init_type->name + "' and '" + type->name + "' differ in sizes",
                            sym->where,
                            ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                        ));
                    }
                }
            }
        } else node->initial = nullptr;

        this->expectToken(TokenType::Semicolon);
        this->verbose_descend();
        return node;
    }
};