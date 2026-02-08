#include "VirtualMachine.hpp"
#include "bytecode.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace ULang;

// HEX helper
static inline std::string HEX(uint64_t no, std::string suffix = "h") {
    std::stringstream stream;
    stream << std::hex << no << suffix;
    return stream.str();
}

std::string fmtOperand(const Operand& operand) {
    std::string out;

    switch (operand.type) {
        case OperandType::OP_NULL:
            return "";

        case OperandType::OP_IMMEDIATE:
        case OperandType::OP_CONSTANT: {
            uint64_t val = operand.data;
            return HEX(val);
        }

        case OperandType::OP_REFERENCE: {
            uint64_t val = operand.data;
            return "&" + HEX(val);
        }

        case OperandType::OP_REGISTER:
            return "R" + std::to_string(operand.data); // TODO
    }

    return "???";
}

namespace ULang {
    void VirtualMachine::init() {
        if(this->verbose_en) {
            std::cout << "ULang VM v 1.0. - https://github.com/vhajsman/ulang" << std::endl;
            std::cout << "INIT: register size: " << sizeof(uint64_t) << ", count: " << this->REG_COUNT << std::endl;
            std::cout << "INIT: total register-occupied memory: " << sizeof(uint64_t) * this->REG_COUNT << std::endl;
            std::cout << "INIT: stack size: " << STACK_SIZE << std::endl;
        }

        memset(this->regs, 0x00, sizeof(uint64_t) * this->REG_COUNT);
        this->pc =    &regs[R_PC.reg_no];       // Program counter
        this->sp =    &regs[R_SP.reg_no];       // Stack pointer
        this->fp =    &regs[R_FP.reg_no];       // Frame pointer
        this->flags = &regs[R_FLAGS.reg_no];    // Flags

        this->stack = (uint8_t*) malloc(this->STACK_SIZE);
        *this->sp = reinterpret_cast<uint64_t>(this->stack + this->STACK_SIZE);

        this->heap_init();
    }

    void VirtualMachine::run(const std::vector<Instruction> program) {
        if(this->verbose_en) {
            std::cout << "EXEC: instruction count: " << program.size() << std::endl;
        }

        this->stat_exec_begin = std::chrono::steady_clock::now();

        bool running = true;
        *this->pc = 0;
        while(running && *this->pc < program.size()) {
            this->execute(program[*this->pc]);
            (*this->pc)++;
        }
    }

    /*
    uint64_t* VirtualMachine::castOpReference(const Operand& op) {
        switch(op.type) {
            case OperandType::OP_IMMEDIATE: return (uint64_t*) &op.data;
            case OperandType::OP_CONSTANT:  return (uint64_t*) &op.data;
            case OperandType::OP_REGISTER:  return &this->regs[op.data];
            case OperandType::OP_REFERENCE: return (uint64_t*) this->castHeapReference(op.data);
            case OperandType::OP_NULL:      return nullptr;
            default:
                std::runtime_error("Invalid operand");
        }
    }
    */

    uint64_t VirtualMachine::readOpCast(const Operand& op) {
        switch(op.type) { // TODO: constants
            case OperandType::OP_CONSTANT:
            case OperandType::OP_IMMEDIATE: return op.data;
            case OperandType::OP_REGISTER:  return this->regs[op.data];
            case OperandType::OP_REFERENCE: return *(uint64_t*) this->castHeapReference(op.data);
            case OperandType::OP_NULL:      return 0;
            default:
                throw std::runtime_error("Invalid operand");
        }
    }

    void VirtualMachine::writeOpCast(const Operand& op, uint64_t val) {
        switch (op.type) {
            case OperandType::OP_REGISTER:
                this->regs[op.data] = val;
                return;

            case OperandType::OP_REFERENCE:
                *(uint64_t*) this->castHeapReference(op.data) = val;
                return;

            case OperandType::OP_IMMEDIATE:
            case OperandType::OP_CONSTANT:
            case OperandType::OP_NULL:
                throw std::runtime_error("Operand not writeable");

            default:
                throw std::runtime_error("Invalid operand");
        }
    }

    void VirtualMachine::execute(const Instruction& instr) {
        if(this->verbose_en) {
            std::cout << "EXEC: DISASSEMBLY: ";
            std::cout << std::setw(8) << std::setfill('0') << std::hex << instr.offset;
            std::cout << ": " << opcodeToStr(instr.opcode);

            for(const auto& op : instr.operands)
                std::cout << " " << fmtOperand(op);

            std::cout << std::endl;
        }

        switch(instr.opcode) {
            case Opcode::NOP: break;

            case Opcode::PUSH: {
                const Operand& op = instr.operands[0];
                uint64_t val = 0;

                switch(op.type) {
                    case OperandType::OP_IMMEDIATE: 
                        val = op.data; break;
                    case OperandType::OP_REGISTER:
                        val = this->regs[op.data]; break;
                    default:
                        std::runtime_error("Invalid operand");
                }

                *this->sp -= sizeof(uint64_t);
                *(uint64_t*)(this->stack + *this->sp) = val;
                break;
            }

            case Opcode::POP: {
                const Operand& op = instr.operands[0];
                uint64_t val = *(uint64_t*)(this->stack + *this->sp);
                *this->sp += sizeof(uint64_t);

                if(op.type == OperandType::OP_REGISTER)
                    this->regs[op.data] = val;

                break;
            }
        }
    }
};
