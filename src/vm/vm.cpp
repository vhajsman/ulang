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
            return "null";

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
            return "r" + std::to_string(operand.data) + ":" + vmreg_defines[operand.data].reg_name;
    }

    return "???";
}

namespace ULang {
    void VirtualMachine::init() {
        if(this->vmparams.verbose_en) {
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
        if(this->vmparams.verbose_en) {
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

    uint64_t VirtualMachine::readOpCast(const Operand& op) {
        uint64_t res = 0;
        switch(op.type) { // TODO: constants
            case OperandType::OP_CONSTANT:
            case OperandType::OP_IMMEDIATE: res = static_cast<uint32_t>(op.data); break;
            case OperandType::OP_REGISTER:  res = this->regs[op.data]; break;
            case OperandType::OP_REFERENCE: res = *(uint64_t*) this->castHeapReference(op.data); break;
            case OperandType::OP_NULL:      res = 0; break;
            default:
                throw std::runtime_error("Invalid operand");
        }

        if(this->vmparams.verbose_en) {
            std::cout << "\033[35m\t--> ";
            std::cout << "read: " << fmtOperand(op) << " -> " << static_cast<uint32_t>(res);
            std::cout << "\033[0m" << std::endl;
        }

        return res;
    }

    void VirtualMachine::writeOpCast(const Operand& op, uint64_t val) {
        switch (op.type) {
            case OperandType::OP_REGISTER:
                this->regs[op.data] = val;
                break;

            case OperandType::OP_REFERENCE:
                *(uint64_t*) this->castHeapReference(op.data) = val;
                break;

            case OperandType::OP_IMMEDIATE:
            case OperandType::OP_CONSTANT:
            case OperandType::OP_NULL:
                throw std::runtime_error("Operand not writeable");

            default:
                throw std::runtime_error("Invalid operand");
        }

        if(this->vmparams.verbose_en) {
            std::cout << "\033[35m\t--> ";
            std::cout << "write: " << fmtOperand(op) << " <- " << static_cast<uint32_t>(val);
            std::cout << "\033[0m" << std::endl;
        }
    }

    void VirtualMachine::execute(const Instruction& instr) {
        if(this->vmparams.verbose_en) {
            std::cout << "EXEC: DISASSEMBLY: ";
            std::cout << std::setw(8) << std::setfill('0') << std::hex << instr.offset;
            std::cout << ": " << opcodeToStr(instr.opcode);

            for(const auto& op : instr.operands)
                std::cout << " " << fmtOperand(op);

            std::cout << std::endl;
        }

        switch(instr.opcode) {
            //TODO: flags

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

            case Opcode::ADD: {
                //
                // [DST] = ([DST] + [SRC])
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                uint64_t a = readOpCast(dst);
                uint64_t b = readOpCast(src);
                uint64_t result = a + b;

                writeOpCast(dst, result);

                break;
            }

            case Opcode::SUB: {
                //
                // [DST] = ([DST] - [SRC])
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                uint64_t a = readOpCast(dst);
                uint64_t b = readOpCast(src);
                uint64_t result = a - b;

                writeOpCast(dst, result);

                break;
            }

            case Opcode::MUL: {
                //
                // [DST] = ([DST] * [SRC])
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                uint64_t a = readOpCast(dst);
                uint64_t b = readOpCast(src);
                uint64_t result = a * b;

                writeOpCast(dst, result);

                break;
            }

            case Opcode::DIV: {
                //
                // [DST]  = ([DST] / [SRC])
                // [TMP0] = ([DST] % [SRC])
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                uint64_t a = readOpCast(dst);
                uint64_t b = readOpCast(src);

                if(b == 0)
                    throw std::runtime_error("Division by zero");

                uint64_t q = a / b;
                uint64_t r = a % b;

                writeOpCast(dst, q);
                this->regs[R_TMP0.reg_no] = r;

                break;
            }

            case Opcode::MOV: {
                //
                // r:[DST] = [SRC]
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                if(dst.type != OperandType::OP_REGISTER)
                    throw std::runtime_error("Excepted register reference");

                uint64_t val = this->readOpCast(src);
                this->regs[dst.data] = val;
                break;
            }

            case Opcode::ST: {
                //
                // [REF] = [VAL]
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                if(dst.type != OperandType::OP_REFERENCE)
                    throw std::runtime_error("Excepted heap reference");

                uint64_t val = readOpCast(src);
                writeOpCast(dst, val);
                break;
            }

            case Opcode::LD: {
                //
                // [VAL] = [REF]
                //

                const Operand& dst = instr.operands[0];
                const Operand& src = instr.operands[1];

                writeOpCast(dst, readOpCast(src));
                break;
            }
        }
    }
};
