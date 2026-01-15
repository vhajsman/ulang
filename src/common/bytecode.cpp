#include "bytecode.hpp"

namespace ULang {
    Instruction parseInstruction(BytecodeStream& stream) {
        Instruction ins;

        ins.opcode = static_cast<Opcode>(stream.readByte());

        ins.opA.raw_meta = stream.readByte();
        ins.opA.value = const_cast<uint8_t*>(stream.readBytes(ins.opA.getDataSz()));

        ins.opB.raw_meta = stream.readByte();
        ins.opB.value = const_cast<uint8_t*>(stream.readBytes(ins.opB.getDataSz()));

        return ins;
    }
};