#include "compiler.hpp"

namespace ULang {
    ASTNode* CompilerInstance::parseStatement() {
        Token& tok = this->tokens[this->pos];

        if(tok.type == TokenType::Return) {
            this->pos++;
            ASTNode* ret_expr = nullptr;

            if(this->tokens[this->pos].type != TokenType::Semicolon && this->tokens[this->pos].type != TokenType::RCurly)
                ret_expr = this->parseExpression();

            // matchToken(TokenType::Semicolon);
            // this->expectToken(TokenType::Semicolon);

            if(this->tokens[this->pos].type == TokenType::Semicolon)
                this->pos++;

            ASTNode* node = new ASTNode(ASTNodeType::FN_RET);
            node->initial = ret_expr;
            return node;
        }

        if(tok.type == TokenType::TypeKeyword)
            return this->parseVarDecl();

        // assignment out of declaration
        if(tok.type == TokenType::Identifier && this->tokens[this->pos + 1].type == TokenType::Assign) {
            const Symbol* sym = this->symbols.lookup(tok.text);
            if(!sym) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "'" + tok.text + "' is not declared in this scope",
                    tok.loc,
                    ULANG_SYNT_ERR_VAR_UNDEFINED
                );
            }

            ASTNode* node = new ASTNode(ASTNodeType::ASSIGNMENT);
            ASTNode* lhs = new ASTNode(ASTNodeType::VARIABLE);
            lhs->name = tok.text;
            lhs->symbol = const_cast<Symbol *>(sym);
            node->lefthand = lhs;

            this->verbose_nl("Assignment LHS: " + tok.text + ", following: " + this->tokens[this->pos + 1].text);
            this->verbose_ascend();
            
            this->pos += 2;

            node->righthand = parseExpression();
            if(!node->righthand) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "assignment requires an expression",
                    tok.loc,
                    ULANG_SYNT_ERR_EXCEPTED_EXPR
                );
            }

            // is righthand a call?
            if(node->righthand->type == ASTNodeType::FN_CALL)
                node->righthand->target_symbol = lhs->symbol;

            this->expectToken(TokenType::Semicolon);
            this->verbose_descend();
            return node;
        }

        ASTNode* expr = this->parseExpression();
        this->expectToken(TokenType::Semicolon);
        return expr;
    }
};