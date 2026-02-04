#include "bytecode.hpp"
#include <cstdint>
#include <cstring>

namespace ULang {
    Instruction parseInstruction(BytecodeStream& stream) {
        Instruction ins;

        ins.opcode = static_cast<Opcode>(stream.readByte());

        /*
        ins.opA.size = stream.readByte();
        ins.opA.type = static_cast<OperandType>(stream.readByte());
        if(ins.opA.size > 0) {
            std::memset(&ins.opA.data, 0, sizeof(ins.opA.data));
            std::memcpy(&ins.opA.data, stream.readBytes(ins.opA.size), ins.opA.size);
        }
        
        ins.opB.size = stream.readByte();
        ins.opB.type = static_cast<OperandType>(stream.readByte());
        if(ins.opB.size > 0) {
            std::memset(&ins.opB.data, 0, sizeof(ins.opB.data));
            std::memcpy(&ins.opB.data, stream.readBytes(ins.opB.size), ins.opB.size);
        }
        */

        for(int i = 0; i < 2; i++) {
            uint8_t size = stream.readByte();

            Operand op;
            op.type = static_cast<OperandType>(stream.readByte());

            std::memcpy(&op.data, stream.readBytes(size), size);
            
            ins.operands.push_back(op);
        }

        return ins;
    }

    bool validateHeader(const BytecodeHeader& hdr, size_t file_size) {
        if(std::memcmp(hdr.magic, "ULANG0", 6) != 0) return false;
        if(hdr.version_major != 1)                              return false;
        if(hdr.word_size != 4 && hdr.word_size != 8)            return false;
        if(hdr.endian > 1)                                      return false;
        if(hdr.code_offset + hdr.code_size > file_size)         return false;
        if(hdr.meta_offset + hdr.meta_size > file_size)         return false;

        return true;
    }

    bool validateMetaSection(const BytecodeHeader& hdr, const BytecodeMetaHeader& meta, size_t file_size) {
        if(hdr.meta_offset + sizeof(BytecodeMetaHeader) > file_size)
            return false;

        size_t symbols_size = meta.symbol_count * sizeof(MetaSymbol);
        size_t types_size   = meta.type_count   * sizeof(MetaType);

        if(hdr.meta_offset + sizeof(BytecodeMetaHeader) + symbols_size + types_size + meta.string_pool_size > file_size)
            return false;

        return true;
    }

    uint32_t addStringToPool(std::string& pool, const std::string& str) {
        uint32_t offset = pool.size();
        pool += str;
        pool += '\0';
        
        return offset;
    }

    const char* opcodeToStr(Opcode op) {
        switch(op) {
            case Opcode::NOP:   return "NOP";
            case Opcode::PUSH:  return "PUSH";
            case Opcode::POP:   return "POP";
            case Opcode::ADD:   return "ADD";
            case Opcode::SUB:   return "SUB";
            case Opcode::MUL:   return "MUL";
            case Opcode::DIV:   return "DIV";
            case Opcode::LD:    return "LD";
            case Opcode::ST:    return "ST";
            case Opcode::JMP:   return "JMP";
            case Opcode::JZ:    return "JZ";
            case Opcode::CALL:  return "CALL";
            case Opcode::RET:   return "RET";
            case Opcode::HALT:  return "HALT";
            case Opcode::MOV:   return "MOV";
        }
        return "???";
    }

    const char* operandTypeToStr(OperandType t) {
        switch(t) {
            case OperandType::OP_NULL:      return "null";
            case OperandType::OP_IMMEDIATE: return "imm";
            case OperandType::OP_REFERENCE: return "ref";
            case OperandType::OP_CONSTANT:  return "const";
            case OperandType::OP_REGISTER:  return "reg";
        }
        return "?";
    }
};