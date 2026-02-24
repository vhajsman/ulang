#include "compiler.hpp"

namespace ULang {
    ASTNode* CompilerInstance::parseFnDecl() {
        this->expectToken(TokenType::Function);

        // return type
        const DataType* ret_type = resolveDataType(this->expectToken(TokenType::TypeKeyword).text);

        // identifier
        Token tok_name = this->expectToken(TokenType::Identifier);
        
        Symbol* sym = this->symbols.decl_fn(tok_name.text, ret_type, &tok_name.loc);

        this->verbose_nl("Creating ASTNode for function '" + tok_name.text + "(...)'");
        this->verbose_ascend();

        ASTNode* node = new ASTNode(ASTNodeType::FN_DEF);
        node->name = tok_name.text;
        node->symbol = sym;

        // parameters
        this->expectToken(TokenType::LParen); 
        while(this->tokens[this->pos].type != TokenType::RParen) {
            // type
            const DataType* arg_type = resolveDataType(this->expectToken(TokenType::TypeKeyword).text);
            
            // identifier
            Token arg_name = this->expectToken(TokenType::Identifier);

            Symbol* arg_sym = this->symbols.decl(arg_name.text, arg_type);

            this->verbose_nl("Creating ASTNode for function parameter '" + tok_name.text + "(...)->" + arg_sym->name + "'");
            
            ASTNode* arg_node = new ASTNode(ASTNodeType::FN_ARG);
            arg_node->name = arg_sym->name;
            arg_node->symbol = arg_sym;

            node->args.push_back(arg_node);

            // continue to next arg if comma, break otherwise
            if(!this->matchToken(TokenType::Comma))
                break;
        }

        this->expectToken(TokenType::RParen);

        // declaration withou body
        if(this->matchToken(TokenType::Semicolon)) {
            this->friendlyException(CompilerSyntaxException(
                CompilerSyntaxException::Severity::Warning,
                "Function '" + tok_name.text + "' declaration doesn't define it's body",
                tok_name.loc,
                ULANG_SYNT_WARN_FN_NO_BODY
            ));

            this->verbose_descend();
            return node;
        }

        std::string scopeName = this->symbols.getCurrentScope()->_name + "::" + tok_name.text + "@fn_decl";
        Scope* fn_scope = this->symbols.enter(scopeName);
        this->verbose_nl("Enter new scope: " + fn_scope->_name);

        // function body
        // ! function MUST have code block unlike if-else statements, etc
        node->body = this->parseBlock();

        this->symbols.leave();
        this->verbose_descend();
        return node;
    }

    ASTNode* CompilerInstance::parsePostfix() {
        ASTNode* node = this->parsePrimary();

        while(this->pos < this->tokens.size()) {
            /*
            if(this->tokens[this->pos].type == TokenType::LParen) {
                if(!node->symbol) {
                    const Symbol* sym = this->symbols.lookup(node->name);
                    if(!sym) {
                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "'" + node->name + "' was not declared in current scope",
                            node->symbol->where,
                            ULANG_SYNT_ERR_VAR_UNDEFINED
                        );
                    }

                    node->symbol = const_cast<Symbol*>(sym);
                    if(node->symbol->kind != SymbolKind::FUNCTION) {
                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "'" + node->name + "' is not a function",
                            node->symbol->where,
                            ULANG_SYNT_ERR_FN_NOT_FN
                        );
                    }
                }

                ASTNode* call = new ASTNode(ASTNodeType::FN_CALL);
                call->lefthand = node;

                this->pos++;

                if(this->tokens[this->pos].type != TokenType::RParen) {
                    do {
                        ASTNode* arg = parseExpression();
                        call->args.push_back(arg);

                        if(this->tokens[this->pos].type != TokenType::Comma) 
                            break;

                        this->pos++;
                    } while(true);
                }

                this->expectToken(TokenType::RParen);

                if(node->symbol->kind != SymbolKind::FUNCTION) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "'" + node->name + "' is not a function",
                        node->symbol->where,
                        ULANG_SYNT_ERR_FN_NOT_FN
                    );
                }

                node = call;
                continue;
            }

            break;
            */

            if(this->tokens[this->pos].type != TokenType::LParen)
                break;

            if(node->name.empty() || !node->symbol) {
                /*
                if(node->symbol->kind != SymbolKind::FUNCTION) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "'" + node->name + "' is not a function",
                        node->symbol->where,
                        ULANG_SYNT_ERR_FN_NOT_FN
                    );
                }*/

                break;
            }

            // TODO: support call by reference
            if(node->symbol->kind != SymbolKind::FUNCTION) {
                if(node->symbol->kind != SymbolKind::FUNCTION) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "'" + node->name + "' is not a function",
                        node->symbol->where,
                        ULANG_SYNT_ERR_FN_NOT_FN
                    );
                }
            }

            ASTNode* call_node = new ASTNode(ASTNodeType::FN_CALL);
            call_node->lefthand = node;
            call_node->symbol = node->symbol;
            this->pos++;

            if(this->tokens[this->pos].type != TokenType::RParen) {
                do {
                    //ASTNode* arg_node = new ASTNode(ASTNodeType::FN_ARG);
                    ASTNode* arg_node = this->parseExpression();
                    call_node->args.push_back(arg_node);

                    if(this->tokens[this->pos].type != TokenType::Comma)
                        break;

                    this->pos++;
                } while(true);
            }

            this->expectToken(TokenType::RParen);
            node = call_node;
        }

        return node;
    }
};