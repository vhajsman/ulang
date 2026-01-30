#include "bytecode.hpp"
#include "compiler.hpp"
#include "compiler/errno.h"
#include <cstdint>
#include <stdexcept>

namespace ULang {
    void CompilerInstance::compileNode(ASTNode* node, std::vector<Instruction>& out) {
        if(!node)
            return;

        // TODO: locations in exceptions

        Instruction instruction;

        switch(node->type) {
            case ASTNodeType::VARIABLE: {
                if(!node->symbol) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Error,
                        "Undefined variable: " + node->name,
                        ULANG_LOCATION_NULL,
                        ULANG_SYNT_ERR_VAR_UNDEFINED
                    );
                }
                
                instruction = {
                    Opcode::LOAD,
                    this->makeRef(node->symbol->stackOffset)
                };
                
                out.push_back(instruction);
                break;
            }

            case ASTNodeType::NUMBER: {
                instruction = {
                    Opcode::PUSH, 
                    this->makeIMM(node->val)
                };

                out.push_back(instruction);
                break;
            }

            case ASTNodeType::BINOP: {
                switch(node->op) {
                    case BinopType::ADDITION:       instruction.opcode = Opcode::ADD; break;
                    case BinopType::SUBSTRACTION:   instruction.opcode = Opcode::SUB; break;
                    case BinopType::MULTIPLICATION: instruction.opcode = Opcode::MUL; break;
                    case BinopType::DIVISION:       instruction.opcode = Opcode::DIV; break;
                }

                this->compileNode(node->lefthand, out);
                this->compileNode(node->righthand, out);

                out.push_back(instruction);
                break;
            }

            case ASTNodeType::ASSIGNMENT: {
                this->compileNode(node->righthand, out);
                if(!node->lefthand || !node->lefthand->symbol)
                    throw std::runtime_error("Assignment target missing");

                instruction.opcode = Opcode::STORE;
                instruction.opA = this->makeRef(node->lefthand->symbol->stackOffset);

                out.push_back(instruction);
                break;
            }

            case ASTNodeType::DECLARATION: {
                if(node->initial) {
                    this->compileNode(node->initial, out);

                    instruction.opcode = Opcode::STORE;
                    instruction.opA = this->makeRef(node->symbol->stackOffset);

                    out.push_back(instruction);
                }

                break;
            }

            default:
                throw std::runtime_error("invalid AST node type");
        }
    }

    void CompilerInstance::serializeInstruction(const Instruction& instr, std::vector<uint8_t>& out) {
        out.push_back(static_cast<uint8_t>(instr.opcode));
        
        out.push_back(instr.opA.raw_meta);
        out.push_back(instr.opB.raw_meta);

        size_t a_sz = instr.opA.getDataSz();
        if(a_sz > 0 && instr.opA.getType() != OperandType::OP_NULL) {
            if(instr.opA.getType() == OperandType::OP_NULL)
                throw std::runtime_error("Operand A has size but NULL type");
            if(!instr.opA.data)
                throw std::runtime_error("Operand A has size but no data");

            write_bytes(out, (const void*) instr.opA.data, a_sz);
        }

        size_t b_sz = instr.opB.getDataSz();
        if(b_sz > 0 && instr.opB.getType() != OperandType::OP_NULL) {
            if(instr.opB.getType() == OperandType::OP_NULL)
                throw std::runtime_error("Operand B has size but NULL type");
            if(!instr.opB.data)
                throw std::runtime_error("Operand B has size but no data");

            write_bytes(out, (const void*) instr.opB.data, b_sz);
        }
    }
};