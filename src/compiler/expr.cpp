#include "compiler.hpp"
#include <stdexcept>

namespace ULang {
    int CompilerInstance::precedence(TokenType type) {
        switch(type) {
            case TokenType::Mul:
            case TokenType::Div:
                return 20;

            case TokenType::Plus:
            case TokenType::Minus:
                return 10;

            default:
                return -1;
        }
    }
    
    ASTNode* CompilerInstance::parsePrimary() {
        Token& tok = this->tokens[this->pos];
        this->verbose_ascend();

        if(tok.type == TokenType::LParen) {
            this->pos++;
            ASTNode* expr = this->parseExpression();

            this->expectToken(TokenType::RParen);
            return expr;
        }

        if(tok.type == TokenType::Number) {
            this->pos++;
            return new ASTNode(std::stoll(tok.text));
        }

        if(tok.type == TokenType::Identifier) {
            this->pos++;

            const Symbol* sym = this->symbols.lookup(tok.text);
            if(!sym) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "'" + tok.text + "' is not declared in this scope",
                    tok.loc,
                    ULANG_SYNT_ERR_VAR_UNDEFINED
                );
            }

            ASTNode* node = new ASTNode(ASTNodeType::VARIABLE);
            node->name = tok.text;
            node->symbol = const_cast<Symbol*>(sym);

            this->verbose_descend();
            return node;
        }

        SourceLocation loc_fail = {
            nullptr,
            this->cparams.sourceFile,
            static_cast<size_t>(tok.loc.loc_line),
            static_cast<size_t>(tok.loc.loc_col)
        };

        this->verbose_descend();
        throw CompilerSyntaxException(
            CompilerSyntaxException::Severity::Error,
            "Excepted primary expression",
            loc_fail,
            ULANG_SYNT_ERR_EXCEPTED_PRIMARY
        );
    }

    ASTNode* CompilerInstance::parseExpression(int prec_min) {
        //ASTNode* lefthand = this->parsePrimary();
        ASTNode* lefthand = this->parsePostfix();
        if(!lefthand)
            throw std::runtime_error("parsePostfix() returned nullptr");

        while(this->pos < this->tokens.size()) {
            TokenType op = this->tokens[this->pos].type;
            if(op == TokenType::Semicolon || op == TokenType::EndOfFile || !isBinop(op))
                break;

            int prec = this->precedence(op);
            if(prec < prec_min)
                break;

            this->pos++;

            ASTNode* righthand = this->parseExpression(prec + 1);
            if(!lefthand || !righthand) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "Invalid expression operands",
                    ULANG_LOCATION_NULL,
                    ULANG_SYNT_ERR_EXCEPTED_EXPR
                );
            }

            //ASTNode* node = new ASTNode(ASTNodeType::BINOP);
            //node->lefthand = lefthand;
            //node->righthand = righthand;

            BinopType op_type;

            switch(op) {
                case TokenType::Plus:   op_type = BinopType::ADDITION;         break;
                case TokenType::Minus:  op_type = BinopType::SUBSTRACTION;     break;
                case TokenType::Mul:    op_type = BinopType::MULTIPLICATION;   break;
                case TokenType::Div:    op_type = BinopType::DIVISION;         break;

                default:
                    break;
            }

            if(isBinop(op))
                this->verbose_nl("IS binop");

            // ---- type checking & BINOP result type ----
            const DataType* left_type =  getType(lefthand, lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL);
            const DataType* right_type = getType(righthand, righthand->symbol ? righthand->symbol->where : ULANG_LOCATION_NULL);
            if(!left_type || !right_type)
                throw std::runtime_error("Could not determine operand types");

            if((left_type->flags & SIGN) != (right_type->flags & SIGN)) {
                this->friendlyException(CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Warning,
                    "Operand types '" + left_type->name + "' and '" + right_type->name + "' differ in signedness",
                    lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL,
                    ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                ));
            }

            if(left_type->size != right_type->size) {
                this->friendlyException(CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Warning,
                    "Operand types '" + left_type->name + "' and '" + right_type->name + "' differ in sizes",
                    lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL,
                    ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                ));
            }

            // determine result type (pick larger size, signedness preference)
            const DataType* result_type = left_type;
            if(right_type->size > left_type->size) {
                result_type = right_type;
            } else if((left_type->flags & SIGN) != (right_type->flags & SIGN)) {
                result_type = (left_type->flags & SIGN) ? left_type : right_type;
            }

            ASTNode* node = new ASTNode(ASTNodeType::BINOP);
            node->lefthand = lefthand;
            node->righthand = righthand;
            node->op = op_type;

            node->symbol = new Symbol{
                "<binop>", 
                0, 
                SymbolKind::VARIABLE, 
                SymbolOrigin::USER,
                result_type, 
                0, 
                {}
            };

            lefthand = node;
        }

        return lefthand;
    }
};