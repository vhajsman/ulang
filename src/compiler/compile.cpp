#include "bytecode.hpp"
#include "compiler.hpp"
#include "compiler/errno.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "types.hpp"
#include "vmreg_defines.hpp"

#ifndef OP_GET_NULL
#define OP_GET_NULL \
    {OperandType::OP_NULL, 0}
#endif

namespace ULang {
    Operand CompilerInstance::compileNode(ASTNode* node, std::vector<Instruction>& out) {
        // TODO: locations in exceptions

        if(!node) {
            std::cout << "warning: null node" << std::endl;
            return OP_GET_NULL;
        }

        this->verbose_nl("Compile AST node '" + (node->name.empty() ? "unnamed" : node->name) + "'");
        this->verbose_ascend();

        std::vector<bool> tmp_snapshot = this->tmp_used;
        Operand result = OP_GET_NULL;
        Instruction instruction;

        try {
            switch(node->type) {
                case ASTNodeType::NUMBER: {
                    this->verbose_descend();
                    return {OperandType::OP_IMMEDIATE, node->val};
                }
    
                case ASTNodeType::VARIABLE: {
                    if(!node->symbol) {
                        throw CompilerSyntaxException(
                            CompilerSyntaxException::Severity::Error,
                            "Undefined variable: " + node->name,
                            ULANG_LOCATION_NULL,
                            ULANG_SYNT_ERR_VAR_UNDEFINED
                        );
                    }
    
                    Operand reg = this->allocTmpReg();
                    this->emit(this->ctx, Opcode::LD, reg, {OperandType::OP_REFERENCE, node->symbol->stackOffset});

                    this->verbose_descend();
                    return reg;
                }
    
                case ASTNodeType::ASSIGNMENT: {
                    Operand R = this->compileNode(node->righthand, out);
                    if(!node->lefthand || !node->lefthand->symbol)
                        throw std::runtime_error("Assignment target missing");
    
                    this->emit(this->ctx, Opcode::ST, {OperandType::OP_REFERENCE, node->lefthand->symbol->stackOffset}, R);
    
                    if(R.type == OperandType::OP_REGISTER && R.data >= R_TMP0.reg_no && R.data < R_TMP0.reg_no + this->tmp_used.size())
                        this->freeTmpReg(R, true);
                    
                    this->verbose_descend();
                    return OP_GET_NULL;
                }
    
                case ASTNodeType::DECLARATION: {
                    if(node->initial) {
                        Operand R = this->compileNode(node->initial, out);
                        this->emit(this->ctx, Opcode::ST, {OperandType::OP_REFERENCE, node->symbol->stackOffset}, R);
    
                        if(R.type == OperandType::OP_REGISTER && R.data >= R_TMP0.reg_no && R.data < R_TMP0.reg_no + this->tmp_used.size())
                            this->freeTmpReg(R, true);
                    }
    
                    this->verbose_descend();
                    return OP_GET_NULL;
                }
    
                case ASTNodeType::BINOP: {
                    Operand L = this->compileNode(node->lefthand, out);
                    Operand R = this->compileNode(node->righthand, out);
    
                    if(L.type != OperandType::OP_REGISTER && R.type != OperandType::OP_REGISTER) {
                        L = this->allocTmpReg();
                        this->emit(this->ctx, Opcode::MOV, L, OP_GET_NULL);
                    }
    
                    switch(node->op) {
                        case BinopType::ADDITION:       instruction.opcode = Opcode::ADD; break;
                        case BinopType::SUBSTRACTION:   instruction.opcode = Opcode::SUB; break;
                        case BinopType::MULTIPLICATION: instruction.opcode = Opcode::MUL; break;
                        case BinopType::DIVISION:       instruction.opcode = Opcode::DIV; break;
                    }
    
                    this->verbose_nl("BINOP: ");
                    this->verbose_print("L: "); this->verbose_print(node->lefthand->val);
                    this->verbose_print("R: "); this->verbose_print(node->righthand->val);
    
                    instruction.operands.push_back(L);
                    instruction.operands.push_back(R);
                    out.push_back(instruction);
    
                    // handle division by zero
                    if( node->op == BinopType::DIVISION && 
                        (R.type == OperandType::OP_IMMEDIATE || R.type == OperandType::OP_CONSTANT) &&
                        R.data == 0) {
                            this->friendlyException(CompilerSyntaxException(
                                CompilerSyntaxException::Severity::Warning,
                                "Division by zero", ULANG_LOCATION_NULL,
                                ULANG_SYNT_WARN_DIVISION_ZERO
                            ));
                    }
    
                    if(R.type == OperandType::OP_REGISTER && R.data >= R_TMP0.reg_no && R.data < R_TMP0.reg_no + this->tmp_used.size())
                        this->freeTmpReg(R, true);
                    
                    this->verbose_descend();
                    return L;
                }
    
                case ASTNodeType::FN_ARG:
                    break;
    
                case ASTNodeType::FN_DEF: {
                    
                    ASTNode* prev = this->currentFunction;
                    this->currentFunction = node;
    
                    this->compileFunction(node, out);
    
                    this->currentFunction = prev;

                    this->verbose_descend();
                    
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
    
                        if(op.type == OperandType::OP_REGISTER && op.data >= R_TMP0.reg_no && op.data < R_TMP0.reg_no + this->tmp_used.size())
                            this->freeTmpReg(op, true);
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
                    
                    this->verbose_descend();
                    return OP_GET_NULL;
                }
    
                case ASTNodeType::FN_CALL: {
                    // TODO: calling convention
    
                    if(!node->symbol)
                        throw std::runtime_error("function symbol not set for FN_CALL: '" + node->name + "'");
                    if(node->symbol->kind != SymbolKind::FUNCTION)
                        throw std::runtime_error("invalid symbol in FN_CALL: '" + node->name + "'");
    
                    for(ASTNode* arg: node->args)
                        this->compileNode(arg, out);
    
                    this->emit(this->ctx, Opcode::CALL, {
                        OperandType::OP_REFERENCE, 
                        node->symbol->entry_ip
                    }, OP_GET_NULL);
    
                    if(node->target_symbol) {
                        this->emit(this->ctx, Opcode::MOV, {
                            OperandType::OP_REFERENCE, 
                            node->target_symbol->stackOffset}, 
                        {
                            OperandType::OP_REGISTER, 
                            R_FNR.reg_no
                        });
                    }

                    this->verbose_descend();
                    return {
                        OperandType::OP_REGISTER,
                        R_FNR.reg_no
                    };
                }
    
                default:
                    throw std::runtime_error("invalid AST node type");
            }
        } catch(std::exception& e) {
            this->verbose_descend();
            throw e;
        }


        // free all the tmp registers allocated in this scope, except the result
        for(uint32_t i = 0; i < this->tmp_used.size(); i++) { // FIXME
            if(this->tmp_used[i] && !tmp_snapshot[i]) {
                Operand tmp{OperandType::OP_REGISTER, R_TMP0.reg_no + i};
                if(!(result.type == OperandType::OP_REGISTER && result.data == tmp.data))
                    this->freeTmpReg(tmp);
            }
        }

        this->verbose_descend();
        return result;
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
