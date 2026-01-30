#include "bytecode.hpp"
#include "compiler.hpp"
#include <cstdint>

#define cout_verbose        \
    if(this->en_verbose)    \
        std::cout

namespace ULang {
    Instruction CompilerInstance::makeInstruction(Opcode opcode, Operand a, Operand b) {
        Instruction instruction {opcode, a, b};
        return instruction;
    }

    Operand CompilerInstance::makeIMM(uint64_t val, uint8_t sz) {
        Operand o{};
        
        o.setType(OperandType::OP_IMMEDIATE);
        o.setDataSz(sz);
        o.data = val;

        return o;
    }

    Operand CompilerInstance::makeIMMu32(uint32_t val) {
        Operand o{};
        
        o.setType(OperandType::OP_IMMEDIATE);
        o.setDataSz(4);
        o.data = val;

        return o;
    }

    Operand CompilerInstance::makeRef(uint32_t offset, uint8_t sz) {
        Operand o{};

        o.raw_meta = (static_cast<uint8_t>(OperandType::OP_REFERENCE) << 4) | (sz & 0x0F);
        o.data = offset;

        return o;
    }
};

#undef cout_verbose