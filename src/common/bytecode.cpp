#include "bytecode.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

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
};