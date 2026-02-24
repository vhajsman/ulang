#include "compiler.hpp"
#include "bytecode.hpp"
#include "compiler/params.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "errno.h"

#define cout_verbose                                                           \
if (this->cparams.verbose)                                                   \
std::cout

namespace ULang {
    void writeOperand(std::vector<uint8_t>& out, const Operand& op) {
        out.push_back(static_cast<uint8_t>(op.type));

        for(size_t i = 0; i < sizeof(Operand::data); ++i)
            out.push_back(static_cast<uint8_t>((op.data >> (8 * i)) & 0xFF));
    }

    CompilerInstance::CompilerInstance(const std::string& source, CompilerParameters& cparams)
    : lexer(source), cparams(cparams) {}

    void CompilerInstance::friendlyException(CompilerSyntaxException e) {
        this->exceptions_friendly.push_back(e);
        std::cout << e.fmt(true) << std::endl;
    }

    const DataType* CompilerInstance::getType(ASTNode* node, SourceLocation loc) {
        if(!node)
            return nullptr;

        switch(node->type) {
            case ASTNodeType::DECLARATION: 
                return node->symbol ? node->symbol->type : nullptr;

            case ASTNodeType::ASSIGNMENT:
                return node->righthand ? getType(node->righthand, loc) : nullptr;

            case ASTNodeType::NUMBER:
                return &TYPE_INT32;

            case ASTNodeType::FN_ARG:
            case ASTNodeType::VARIABLE:
                if(node->symbol) 
                    return node->symbol->type;

                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "Could not determine type for '" + node->name + "'",
                    loc,
                    ULANG_SYNT_ERR_TYPE_DETERMINE_FAIL
                );

            case ASTNodeType::BINOP: {
                SourceLocation loc_left  = node->lefthand && node->lefthand->symbol ? node->lefthand->symbol->where : loc;
                SourceLocation loc_right = node->righthand && node->righthand->symbol ? node->righthand->symbol->where : loc;

                const DataType* left  = getType(node->lefthand, loc_left);
                const DataType* right = getType(node->righthand, loc_right);

                if((left->flags & SIGN) != (right->flags & SIGN)) {
                    this->friendlyException(CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Operand types '" + left->name + "' and '" + right->name + "' differ in signedness",
                        loc,
                        ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                    ));
                }

                if(left->size != right->size) {
                    this->friendlyException(CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Operand types '" + left->name + "' and '" + right->name + "' differ in sizes",
                        loc,
                        ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                    ));
                }

                return left;
            }
        }

        return nullptr;
    }

    const DataType* CompilerInstance::determineBinopType(const DataType* left, const DataType* right) {
        if(left->size > right->size) return left;
        if(right->size > left->size) return right;

        if((left->flags & SIGN) != (right->flags & SIGN)) {
            return (left->flags & SIGN) ? left : right;
        }

        return left;
    }

    const Token& CompilerInstance::expectToken(TokenType type) {
        if(this->tokens[this->pos].type != type) {
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error, 
                "Unexcepted token: '" + this->tokens[this->pos].text + "', excepted " + toktype2str(type),
                this->tokens[this->pos].loc,
                ULANG_SYNT_ERR_UNEXCEPT_TOK
            );
        }

        return this->tokens[this->pos++];
    }

    const Token& CompilerInstance::expectToken(const std::string& token) {
        if(this->pos >= this->tokens.size() && this->tokens[this->pos].text != token) {
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
        std::cout << "matchToken? current=" << toktype2str(this->tokens[this->pos].type) << " expected=" << toktype2str(type) << std::endl;
        
        //if(this->pos >= this->tokens.size()) 
        // return false;

        if(this->pos >= this->tokens.size() && this->tokens[this->pos].type == type) {
            this->pos++;
            return true;
        }

        return false;
    }

    bool CompilerInstance::matchToken(const std::string &token) {
        std::cout << "matchToken? current=" << toktype2str(this->tokens[this->pos].type) << " expected=" << token << std::endl;

        if(this->tokens[this->pos].text == token) {
            this->pos++;
            return true;
        }
        
        return false;
    }

    inline static bool isBinop(TokenType tt) {
        return tt == TokenType::Plus || tt == TokenType::Minus || tt == TokenType::Div || tt == TokenType::Mul;
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
                result_type, 
                0, 
                {}
            };

            lefthand = node;
        }

        return lefthand;
    }

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

    std::vector<ASTNode*> CompilerInstance::parseBlock() {
        this->expectToken(TokenType::LCurly);
        
        std::vector<ASTNode *> body;
        while(this->tokens[this->pos].type != TokenType::RCurly && this->tokens[this->pos].type != TokenType::EndOfFile) {
            ASTNode* stmt_curr = this->parseStatement();
            if(stmt_curr)
                body.push_back(stmt_curr);
        }
        
        this->expectToken(TokenType::RCurly);
        return body;
    }

    std::vector<uint8_t> CompilerInstance::serializeProgram(const std::vector<Instruction>& program) {
        std::vector<uint8_t> bytecode;
        bytecode.reserve(256);

        for(const Instruction& instr : program) {
            this->serializeInstruction(instr, bytecode);
        }

        return bytecode;
    }

    void CompilerInstance::buildAST() {
        this->verbose_nl("Build AST tree");
        this->verbose_ascend();

        this->ast_owned.clear();
        this->pos = 0;

        while(this->tokens[this->pos].type != TokenType::EndOfFile) {
            ASTNode* node_raw = nullptr;

            switch(this->tokens[this->pos].type) {
                case TokenType::TypeKeyword:
                    node_raw = this->parseVarDecl();
                    break;
                case TokenType::Function:
                    node_raw = this->parseFnDecl();
                    break;

                default: {
                    node_raw = this->parseExpression();
                    this->expectToken(TokenType::Semicolon);
                    break;
                }
            }

            if(!node_raw)
                throw std::runtime_error("Parser returned null AST node");

            this->ast_owned.push_back(ASTPtr(node_raw));
        }

        this->verbose_descend();
    }

    void CompilerInstance::emit(GenerationContext& ctx, Opcode opcode, const Operand& op_a, const Operand& op_b) {
        Instruction instr {};

        instr.opcode = opcode;
        instr.operands.push_back(op_a);
        instr.operands.push_back(op_b);

        ctx.instructions.push_back(instr);
    }

    void CompilerInstance::compile() {
        std::cout << "Compile: " << this->cparams.sourceFile << std::endl;

        try {
            this->tokens = this->lexer.tokenize();
            this->buildAST();
        } catch (const CompilerSyntaxException &e) {
            std::cerr << e.fmt(true) << std::endl;

            if(e.getSeverity() == CompilerSyntaxException::Severity::Error) {
                std::cout << "Compilation terminated" << std::endl;
                exit(1);
            }
        } catch(const std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        this->verbose_nl("tokens: " + std::to_string(this->tokens.size()) + ", nodes: " + std::to_string(this->ast_owned.size()));

        // GenerationContext ctx;
        this->ctx.symtab = &this->symbols;
        this->ctx.stack_top = 0x00;

        // placeholder jump to main
        uint32_t jmp_pos = this->ctx.instructions.size();
        this->emit(this->ctx, Opcode::JMP, {OperandType::OP_NULL}, {OperandType::OP_NULL}); // patch below

        // functions first
        for(const auto& node: this->ast_owned) {
            if(node->type != ASTNodeType::FN_DEF)
                continue;

            ASTNode* nodeg = node.get();
            
            this->verbose_nl("AST function node type: ");
            this->verbose_print(static_cast<int>(node->type));
            this->verbose_ascend();

            this->verbose_nl("node.get() = "); this->verbose_print((uintptr_t) &nodeg);
            this->verbose_nl("node.get()->symbol = "); this->verbose_print((uintptr_t) &nodeg->symbol);

            this->compileNode(nodeg, ctx.instructions);
            this->verbose_descend();
        }

        // patch JMP operand
        this->ctx.instructions[jmp_pos].operands[0] = {
            OperandType::OP_IMMEDIATE, 
            static_cast<uint32_t>(this->ctx.instructions.size())
        };

        // global code then
        for(const auto& node: this->ast_owned) {
            ASTNode* nodeg = node.get();
            if(nodeg->type == ASTNodeType::FN_DEF)
                continue;

            this->verbose_nl("AST normal node type: ");
            this->verbose_print(static_cast<int>(node->type));
            this->verbose_ascend();

            this->verbose_nl("node.get() = "); this->verbose_print((uintptr_t) &nodeg);
            this->verbose_nl("node.get()->symbol = "); this->verbose_print((uintptr_t) &nodeg->symbol);

            this->compileNode(nodeg, ctx.instructions);
            this->verbose_descend();
        }

        this->emit(this->ctx, Opcode::HALT, {OperandType::OP_NULL}, {OperandType::OP_NULL});

        this->verbose_nl("\n");

        std::vector<const DataType*> types_vect = {
            &TYPE_INT8, &TYPE_INT16, &TYPE_INT32, &TYPE_INT64,
            &TYPE_UINT8, &TYPE_UINT16, &TYPE_UINT32, &TYPE_UINT64,
            &TYPE_CHAR
        };

        MetaData meta = buildMeta(this->symbols, types_vect, this->cparams.verbose);
        std::vector<uint8_t> code = this->serializeProgram(ctx.instructions);
        writeBytecode(this->cparams.outFile, code, meta, 4);
    }
};

#undef cout_verbose
