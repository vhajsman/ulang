#include "bytecode.hpp"
#include "compiler.hpp"
#include <cstdint>

#define cout_verbose        \
    if(this->en_verbose)    \
        std::cout

namespace ULang {
    Instruction CompilerInstance::makeInstruction(Opcode opcode, Operand a, Operand b) {
        Instruction instruction {};

        instruction.opcode = opcode;
        instruction.operands.push_back(a);
        instruction.operands.push_back(b);

        return instruction;
    }

    Operand CompilerInstance::makeIMM(uint32_t val) {
        Operand o{};
        o.type = OperandType::OP_IMMEDIATE;
        
        o.data = val;

        return o;
    }

    Operand CompilerInstance::makeIMMu32(uint32_t val) {
        return this->makeIMM(val);
    }

    Operand CompilerInstance::makeRef(uint32_t offset) {
        Operand o{};

        o.type = OperandType::OP_REFERENCE;
        o.data = offset;

        return o;
    }
};

#undef cout_verbose