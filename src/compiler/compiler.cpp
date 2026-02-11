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

#define cout_verbose        \
    if(this->cparams.verbose)    \
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
                // fallback loc pro levý a pravý operand
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

    bool CompilerInstance::matchToken(const std::string& token) {
        if(this->tokens[this->pos].text == token) {
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

        if(tok.type == TokenType::Return) {
            this->pos++;
            ASTNode* expr = nullptr;

            if(this->tokens[this->pos].type != TokenType::Semicolon)
                expr = this->parseExpression();

            this->expectToken(TokenType::Semicolon);

            ASTNode* node = new ASTNode(ASTNodeType::FN_RET);
            node->initial = expr;
            return node;
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

            // Is function call?
            if(this->matchToken(TokenType::LParen)) {
                cout_verbose << "Parsing function call: " << tok.text << "(...); ..." << std::endl;
                std::vector<ASTNode*> args;

                if(!this->matchToken(TokenType::RParen)) {
                    do {
                        ASTNode* arg_curr = parseExpression();
                        args.push_back(arg_curr);
                    } while(this->matchToken(TokenType::Comma));
                    this->expectToken(TokenType::RParen);
                    
                    node->args = std::move(args);
                    node->type = ASTNodeType::FN_CALL;
    
                    cout_verbose << " --> Arguments count: " << args.size() << std::endl;

                    // set symbol and return type
                    if(sym->kind != SymbolKind::FUNCTION) {
                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "'" + tok.text + "' is not a function",
                            tok.loc,
                            ULANG_SYNT_ERR_FN_NOT_FN
                        );
                    }

                    /*
                    node->symbol = new Symbol{
                        sym->name,
                        0,
                        SymbolKind::FUNCTION,
                        sym->type,
                        0,
                        0, // todo: resolve entry IP
                        tok.loc
                    };
                    */

                    cout_verbose << " --> Arguments count: " << node->args.size() << std::endl;
                }
            }

            return node;
        }

        SourceLocation loc_fail = {
            nullptr,
            this->cparams.sourceFile,
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

        } else node->initial = nullptr;

        this->expectToken(TokenType::Semicolon);
        return node;
    }

    ASTNode* CompilerInstance::parseFnDecl() {
        this->expectToken(TokenType::Function);
        const DataType* ret_type = resolveDataType(this->expectToken(TokenType::TypeKeyword).text);

        Token tok_name = this->expectToken(TokenType::Identifier);
        Symbol* sym = this->symbols.decl_fn(tok_name.text, ret_type, &tok_name.loc);

        cout_verbose << " --> Creating ASTNode for function: " << tok_name.text << "(...)" << std::endl;
        ASTNode* node = new ASTNode(ASTNodeType::FN_DEF);
        node->name = tok_name.text;
        node->symbol = sym;
        
        // parameters
        this->expectToken("("); 
        while(!this->matchToken(")")) {
            const DataType* arg_type = resolveDataType(this->expectToken(TokenType::TypeKeyword).text);
            Token arg_name = this->expectToken(TokenType::Identifier);
            Symbol* arg_sym = this->symbols.decl(arg_name.text, arg_type);

            cout_verbose << " --> Creating ASTNode for function parameter: " << tok_name.text << "(...)->" << arg_name.text << std::endl;
            ASTNode* arg_node = new ASTNode(ASTNodeType::FN_ARG);
            arg_node->name = arg_name.text;
            arg_node->symbol = arg_sym;
            
            node->args.push_back(arg_node);
        }

        if(this->matchToken(TokenType::Semicolon)) {
            this->friendlyException(CompilerSyntaxException(
                CompilerSyntaxException::Severity::Warning,
                "Function '" + tok_name.text + "' declaration doesn't define it's body",
                tok_name.loc,
                ULANG_SYNT_WARN_FN_NO_BODY
            ));

            return node;
        }

        std::string scopeName = this->symbols.getCurrentScope()->_name + "::" + tok_name.text + "@fn_decl";
        Scope* fn_scope = this->symbols.enter(scopeName);
        cout_verbose << "Enter new scope: " << fn_scope->_name << std::endl;
        
        node->body = this->parseBlock();

        this->symbols.leave();
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

            /*
            throw new CompilerSyntaxException(
                CompilerSyntaxException::Severity::Note,
                "Selected type: '" + result_type->name + "'",
                lefthand->symbol ? lefthand->symbol->where : ULANG_LOCATION_NULL
            );
            */

            node->symbol = new Symbol{"<binop>", 0, SymbolKind::VARIABLE, result_type, 0, {}};
            // -----------------------------------------

            lefthand = node;
        }

        return lefthand;
    }

    ASTNode* CompilerInstance::parseStatement() {
        if(this->matchToken(TokenType::Return)) {
            ASTNode* ret_expr = nullptr;

            if(this->tokens[this->pos].type != TokenType::Semicolon)
                ret_expr = this->parseExpression();

            this->expectToken(TokenType::Semicolon);

            ASTNode* node = new ASTNode(ASTNodeType::FN_RET);
            node->initial = ret_expr;
            return node;
        }

        if(this->tokens[this->pos].type == TokenType::TypeKeyword)
            return this->parseVarDecl();

        // assignment out of declaration
        Token& tok = this->tokens[this->pos];
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

            node->lefthand = new ASTNode(ASTNodeType::VARIABLE);
            node->lefthand = new ASTNode(tok.text);
            node->lefthand->symbol = (Symbol*) sym;

            cout_verbose << " --> Assignment LHS: " << tok.text << ", following: " << this->tokens[this->pos+1].text << std::endl;
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

            this->expectToken(TokenType::Semicolon);
            return node;
        }

        ASTNode* expr = this->parseExpression();
        this->expectToken(TokenType::Semicolon);
        return expr;
    }

    std::vector<ASTNode*> CompilerInstance::parseBlock() {
        this->expectToken(TokenType::LCurly);
        
        std::vector<ASTNode*> stmt;
        while(this->tokens[this->pos].type != TokenType::RCurly) {
            ASTNode* stmt_curr = this->parseStatement();
            stmt.push_back(stmt_curr);
        }

        this->expectToken(TokenType::RCurly);
        return stmt;
    }

    std::vector<uint8_t> CompilerInstance::serializeProgram(const std::vector<Instruction>& program) {
        std::vector<uint8_t> bytecode;
        bytecode.reserve(256);

        for(const Instruction& instr: program) {
            this->serializeInstruction(instr, bytecode);
        }

        return bytecode;
    }

    void CompilerInstance::buildAST() {
        cout_verbose << "Building AST tree..." << std::endl;

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
            //this->registerFunctions();
        } catch(const CompilerSyntaxException& e) {
            std::cerr << e.fmt(true) << std::endl;

            if(e.getSeverity() == CompilerSyntaxException::Severity::Error) {
                std::cout << "Compilation terminated" << std::endl;
                exit(1);
            }
        } catch(const std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        cout_verbose<< " -> " << this->tokens.size() << " tokens, " << this->ast_owned.size() << " AST nodes" << std::endl;

        GenerationContext ctx;
        ctx.symtab = &this->symbols;
        ctx.stack_top = 0x00;

        // functions first
        for(const auto& node: this->ast_owned) {
            if(node->type != ASTNodeType::FN_DEF)
                continue;

            ASTNode* nodeg = node.get();
            cout_verbose<< " --> AST (f) node type: " << static_cast<int>(node->type) << std::endl;
            cout_verbose<< "   --> node.get() = " << std::hex << &nodeg << std::endl;
            cout_verbose<< "   --> node.get()->symbol = " << std::hex << nodeg->symbol << std::endl;
            this->compileNode(nodeg, ctx.instructions);
        }

        // upstream then
        for(const auto& node: this->ast_owned) {
            ASTNode* nodeg = node.get();
            cout_verbose<< " --> AST node type: " << static_cast<int>(node->type) << std::endl;
            cout_verbose<< "   --> node.get() = " << std::hex << &nodeg << std::endl;
            cout_verbose<< "   --> node.get()->symbol = " << std::hex << nodeg->symbol << std::endl;
            this->compileNode(nodeg, ctx.instructions);
        }

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
