#include "bytecode.hpp"
#include "compiler.hpp"
#include "compiler/errno.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace ULang {
    void CompilerInstance::compileNode(ASTNode* node, std::vector<Instruction>& out) {
        if(!node) {
            std::cout << "warning: null node" << std::endl;
            return;
        }

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

                instruction.opcode = Opcode::LD;
                this->makeRef(node->symbol->stackOffset);
                
                out.push_back(instruction);
                break;
            }

            case ASTNodeType::NUMBER: {
                instruction.opcode = Opcode::PUSH;
                instruction.operands.push_back(this->makeIMM(node->val));
                out.push_back(instruction);
                break;
            }

            case ASTNodeType::BINOP: {
                // TODO what?

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

                instruction.opcode = Opcode::ST;
                instruction.operands.push_back(this->makeRef(node->lefthand->symbol->stackOffset));

                out.push_back(instruction);
                break;
            }

            case ASTNodeType::DECLARATION: {
                if(node->initial) {
                    this->compileNode(node->initial, out);

                    instruction.opcode = Opcode::ST;
                    instruction.operands.push_back(this->makeRef(node->symbol->stackOffset));

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