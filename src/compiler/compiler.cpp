#include "compiler.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "errno.h"

#define cout_verbose        \
    if(this->en_verbose)    \
        std::cout

namespace ULang {
    CompilerInstance::CompilerInstance(const std::string& source, const std::string& filename, bool en_verbose)
    : lexer(source), filename(filename), en_verbose(en_verbose) {}

    const Token& CompilerInstance::expectToken(TokenType type) {
        if(this->tokens[this->pos].type != type) {            
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error, 
                "Unexcepted token: '" + this->tokens[this->pos].text + "'",
                this->tokens[this->pos].loc,
                ULANG_SYNT_ERR_UNEXCEPT_TOK
            );
        }

        return this->tokens[this->pos++];
    }

    const Token& CompilerInstance::expectToken(const std::string& token) {
        if(this->tokens[this->pos].text != token) {
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error,
                "Unexcepted token: '" + this->tokens[this->pos].text + "', excepted '" + token + "'",
                this->tokens[this->pos].loc,
                ULANG_SYNT_ERR_UNEXCEPT_TOK
            );
        }

        return this->tokens[this->pos++];
    }

    bool CompilerInstance::matchToken(TokenType type) {
        if(this->tokens[this->pos].type == type) {
            this->pos++;
            return true;
        }

        return false;
    }

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

            ASTNode* node = new ASTNode(tok.text);
            node->symbol = const_cast<Symbol*>(sym);
            return node;

            //return new ASTNode(tok.text);
        }

        SourceLocation loc_fail = {
            nullptr,
            this->filename,
            static_cast<size_t>(tok.loc.loc_line),
            static_cast<size_t>(tok.loc.loc_col)
        };

        throw CompilerSyntaxException(
            CompilerSyntaxException::Severity::Error,
            "Excepted primary expression",
            loc_fail,
            ULANG_SYNT_ERR_EXCEPTED_PRIMARY
        );
    }

    ASTNode* CompilerInstance::parseVarDecl() {
        // type
        Token tok_type = this->expectToken(TokenType::TypeKeyword);
        const DataType* type = resolveDataType(tok_type.text);

        // name (identifier)
        Token tok_name = this->expectToken(TokenType::Identifier);

        // symbol
        Symbol* sym = this->symbols.decl(tok_name.text, type, &tok_name.loc);

        cout_verbose << " --> Creating ASTNode for " << tok_name.text << std::endl;

        ASTNode* node = new ASTNode(ASTNodeType::DECLARATION);
        node->name = tok_name.text;
        node->symbol = sym;

        if(this->tokens[this->pos].type == TokenType::Assign) {
            this->pos++;
            node->initial = this->parseExpression();

            if(node->initial) {
                const DataType* init_type = getType(node->initial, sym->where);

                if((init_type->flags & SIGN) != (type->flags & SIGN)) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Types '" + init_type->name + "' and '" + type->name + "' differ in signedness",
                        sym->where,
                        ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                    );
                }

                if(init_type->size != type->size) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Types '" + init_type->name + "' and '" + type->name + "' differ in sizes",
                        sym->where,
                        ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                    );
                }
            }

        } else node->initial = nullptr;

        this->expectToken(TokenType::Semicolon);
        return node;
    }

    ASTNode* CompilerInstance::parseExpression(int prec_min) {
        ASTNode* lefthand = this->parsePrimary();

        while(true) {
            TokenType op = this->tokens[this->pos].type;
            if(op == TokenType::Semicolon || op == TokenType::EndOfFile)
                break;

            int prec = this->precedence(op);
            if(prec < prec_min)
                break;

            this->pos++;

            ASTNode* righthand = this->parseExpression(prec + 1);

            ASTNode* node = new ASTNode(ASTNodeType::BINOP);
            node->lefthand = lefthand;
            node->righthand = righthand;

            switch (op) {
                case TokenType::Plus:  node->op = BinopType::ADDITION;          break;
                case TokenType::Minus: node->op = BinopType::SUBSTRACTION;      break;
                case TokenType::Mul:   node->op = BinopType::MULTIPLICATION;    break;
                case TokenType::Div:   node->op = BinopType::DIVISION;          break;
                
                default:
                    break;
            }

            // ---- type checking & BINOP result type ----
            const DataType* left_type  = getType(lefthand, lefthand->symbol ? lefthand->symbol->where : SourceLocation{});
            const DataType* right_type = getType(righthand, righthand->symbol ? righthand->symbol->where : SourceLocation{});

            if((left_type->flags & SIGN) != (right_type->flags & SIGN)) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Warning,
                    "Operand types '" + left_type->name + "' and '" + right_type->name + "' differ in signedness",
                    lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL,
                    ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                );
            }

            if(left_type->size != right_type->size) {
                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Warning,
                    "Operand types '" + left_type->name + "' and '" + right_type->name + "' differ in sizes",
                    lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL,
                    ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                );
            }

            // determine result type (pick larger size, signedness preference)
            const DataType* result_type = left_type;
            if(right_type->size > left_type->size) {
                result_type = right_type;
            } else if((left_type->flags & SIGN) != (right_type->flags & SIGN)) {
                result_type = (left_type->flags & SIGN) ? left_type : right_type;
            }

            /*
            throw new CompilerSyntaxException(
                CompilerSyntaxException::Severity::Note,
                "Selected type: '" + result_type->name + "'",
                lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL
            );
            */

            node->symbol = new Symbol{"<binop>", 0, result_type, 0, {}};
            // -----------------------------------------

            lefthand = node;
        }

        return lefthand;
    }

    void CompilerInstance::buildAST() {
        this->ast_owned.clear();
        this->pos = 0;

        while(this->tokens[this->pos].type != TokenType::EndOfFile) {
            ASTNode* node_raw = nullptr;

            if(this->tokens[this->pos].type == TokenType::TypeKeyword) {
                node_raw = this->parseVarDecl();
            } else {
                node_raw = this->parseExpression();
                this->expectToken(TokenType::Semicolon);
            }
            
            if(!node_raw)
                throw std::runtime_error("Parser returned null AST node");

            this->ast_owned.push_back(ASTPtr(node_raw));
        }
    }

    void CompilerInstance::compile() {
        std::cout << "Compile: " << this->filename << std::endl;

        try {
            this->tokens = this->lexer.tokenize();
            this->buildAST();
        } catch(const CompilerSyntaxException& e) {
            std::cerr << e.fmt(true);

            if(e.getSeverity() == CompilerSyntaxException::Severity::Error) {
                std::cout << "Compilation terminated" << std::endl;
                exit(1);
            }
        } catch(const std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        cout_verbose<< " -> " 
                    << this->tokens.size() 
                    << " tokens, " 
                    << this->ast_owned.size() 
                    << " AST nodes" 
                    << std::endl;

        for(const auto& node: this->ast_owned) {
            cout_verbose<< " --> AST node type: "
                        << static_cast<int>(node->type)
                        << std::endl;
        }

        // TODO: type checking, bytecode, IR

        /*
        for(auto& node: this->ast_owned)
            node.reset();

        this->ast_owned.clear();
        */
    }
};

#undef cout_verbose
