#include "bytecode.hpp"
#include "compiler.hpp"
#include "compiler/errno.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "types.hpp"
#include "vmreg_defines.hpp"

#define cout_verbose        \
    if(this->cparams.verbose)    \
        std::cout

#ifndef OP_GET_NULL
#define OP_GET_NULL \
    {OperandType::OP_NULL, 0}
#endif

namespace ULang {
    Operand CompilerInstance::compileNode(ASTNode* node, std::vector<Instruction>& out) {
        if(!node) {
            std::cout << "warning: null node" << std::endl;
            return OP_GET_NULL;
        }

        // TODO: locations in exceptions
        // TODO: allocTmp()

        Instruction instruction;

        switch(node->type) {
            case ASTNodeType::FN_ARG:
                break;

            case ASTNodeType::VARIABLE: {
                if(!node->symbol) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "Undefined variable: " + node->name,
                        ULANG_LOCATION_NULL,
                        ULANG_SYNT_ERR_VAR_UNDEFINED
                    );
                }

                return this->makeRef(node->symbol->stackOffset);
            }

            case ASTNodeType::NUMBER: {
                return {OperandType::OP_IMMEDIATE, node->val};
            }

            case ASTNodeType::BINOP: {
                Operand L = this->compileNode(node->lefthand, out);
                Operand R = this->compileNode(node->righthand, out);

                if(L.type != OperandType::OP_REGISTER && R.type != OperandType::OP_REGISTER) {
                    Operand tmp = {
                        OperandType::OP_REGISTER, 
                        vmreg_defines[19].reg_no // TMP3
                    };

                    Instruction i_mov;
                    i_mov.opcode = Opcode::MOV;
                    i_mov.operands.push_back(tmp);
                    i_mov.operands.push_back(L);

                    out.push_back(i_mov);
                    L = tmp;
                }

                switch(node->op) {
                    case BinopType::ADDITION:       instruction.opcode = Opcode::ADD; break;
                    case BinopType::SUBSTRACTION:   instruction.opcode = Opcode::SUB; break;
                    case BinopType::MULTIPLICATION: instruction.opcode = Opcode::MUL; break;
                    case BinopType::DIVISION:       instruction.opcode = Opcode::DIV; break;
                }

                cout_verbose << "   --> BINOP lefthand: " << node->lefthand->val << " righthand: " << node->righthand->val << std::endl;

                instruction.operands.push_back(L);
                instruction.operands.push_back(R);
                out.push_back(instruction);

                if( node->op == BinopType::DIVISION && 
                    (R.type == OperandType::OP_IMMEDIATE || R.type == OperandType::OP_CONSTANT) &&
                    R.data == 0) {
                        this->friendlyException(CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Warning,
                            "Division by zero", ULANG_LOCATION_NULL,
                            ULANG_SYNT_WARN_DIVISION_ZERO
                        ));
                }
                
                return L;
            }

            case ASTNodeType::ASSIGNMENT: {
                Operand R = this->compileNode(node->righthand, out);
                if(!node->lefthand || !node->lefthand->symbol)
                    throw std::runtime_error("Assignment target missing");

                instruction.opcode = Opcode::ST;
                instruction.operands.push_back(this->makeRef(node->lefthand->symbol->stackOffset));
                instruction.operands.push_back(R);
                out.push_back(instruction);
                
                return OP_GET_NULL;
            }

            case ASTNodeType::DECLARATION: {
                if(node->initial) {
                    Operand R = this->compileNode(node->initial, out);

                    instruction.opcode = Opcode::ST;
                    instruction.operands.push_back(this->makeRef(node->symbol->stackOffset));
                    instruction.operands.push_back(R);
                    out.push_back(instruction);
                }

                return OP_GET_NULL;
            }

            case ASTNodeType::FN_DEF: {
                ASTNode* prev = this->currentFunction;
                this->currentFunction = node;

                this->compileFunction(node, out);

                this->currentFunction = prev;
                return OP_GET_NULL;
            }

            case ASTNodeType::FN_RET: {
                if(!this->currentFunction) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "return statement not excepted",
                        node->symbol->where,
                        ULANG_SYNT_ERR_UNEXCEPTED_RET
                    );
                }

                const DataType* ret_type = currentFunction->symbol->type;

                if(node->initial) {
                    if(ret_type == &TYPE_VOID) {
                        throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                            "void function can't return a value",
                            node->symbol->where,
                            ULANG_SYNT_ERR_FN_RET_VOID
                        );
                    }

                    const DataType* retExpr_type = getType(node->initial, node->initial->symbol ? node->initial->symbol->where : ULANG_LOCATION_NULL);
                    if(retExpr_type != ret_type) {
                        // TODO: implicit casts

                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "return type mismatch",
                            currentFunction->symbol->where,
                            ULANG_SYNT_ERR_INVALID_RET
                        );
                    }

                    Operand op = this->compileNode(node->initial, out);
                    this->emit(this->ctx, Opcode::RET, op, OP_GET_NULL);
                } else {
                    if(ret_type != &TYPE_VOID) {
                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "non-void function must return a value: '" + node->name + "'",
                            node->symbol->where,
                            ULANG_SYNT_ERR_FN_NO_RET
                        );
                    }

                    this->emit(this->ctx, Opcode::RET, OP_GET_NULL, OP_GET_NULL);
                }
                
                return OP_GET_NULL;
            }

            case ASTNodeType::FN_CALL: {
                for(ASTNode* arg: node->args) {
                    // TODO: calling convention
                    this->compileNode(arg, out);
                }

                if(!node->symbol)
                    throw std::runtime_error("function symbol not set for FN_CALL: '" + node->name + "'");

                this->emit(this->ctx, Opcode::CALL, {OperandType::OP_REFERENCE, node->symbol->entry_ip}, OP_GET_NULL);

                if(node->target_symbol)
                    this->emit(this->ctx, Opcode::MOV, {OperandType::OP_REFERENCE, node->target_symbol->stackOffset}, {OperandType::OP_REGISTER, R_FNR.reg_no});

                return OP_GET_NULL;
            }

            default:
                throw std::runtime_error("invalid AST node type");
        }

        return OP_GET_NULL;
    }

    void CompilerInstance::serializeInstruction(const Instruction& instr, std::vector<uint8_t>& out) {
        out.push_back(static_cast<uint8_t>(instr.opcode));

        for(size_t i = 0; i < 2; i++) {
            Operand op{};

            if(i < instr.operands.size())
                op = instr.operands[i];

            out.push_back(static_cast<uint8_t>(op.type));

            for(int b = 0; b < 4; b++)
                out.push_back(uint8_t((op.data >> (8 * b)) & 0xFF));
        }
    }
};

#undef cout_verbose