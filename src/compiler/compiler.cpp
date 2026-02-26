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
    : lexer(source), cparams(cparams) {
        this->symbols.getGlobalScope()->ci_ptr = this;
        this->subconstructor_builtin();
    }

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

    //bool isBinop(TokenType tt);

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
