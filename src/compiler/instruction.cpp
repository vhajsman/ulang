#include "bytecode.hpp"
#include "compiler.hpp"
#include <cstdint>
#include <cstring>

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
        
        std::memcpy(o.data.data(), &val, sz);

        return o;
    }

    Operand CompilerInstance::makeIMMu32(uint32_t val) {
        /*
        Operand o{};
        
        o.setType(OperandType::OP_IMMEDIATE);
        o.setDataSz(4);
        
        std::memcpy(o.data.data(), &val, 4);

        return o;
        */

        return this->makeIMM(val, 4);
    }

    Operand CompilerInstance::makeRef(uint32_t offset, uint8_t sz) {
        Operand o{};

        o.raw_meta = (static_cast<uint8_t>(OperandType::OP_REFERENCE) << 4) | (sz & 0x0F);
        std::memcpy(o.data.data(), &offset, sz);

        return o;
    }
};

#undef cout_verbose