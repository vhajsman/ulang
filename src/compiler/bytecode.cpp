#include "bytecode.hpp"
#include "compiler.hpp"
#include "types.hpp"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>

namespace ULang {
    BytecodeHeader buildBytecodeHeader(uint32_t code_size, uint32_t meta_size, uint8_t word_size) {
        BytecodeHeader hdr{};
        std::memcpy(hdr.magic, "ULANG0", 6);
        
        hdr.version_major = 1;
        hdr.version_minor = 0;

        hdr.endian = 0;             // little-endian
        hdr.word_size = word_size;  // 32-bit

        hdr.code_offset = sizeof(BytecodeHeader);
        hdr.code_size = code_size;
        hdr.meta_offset = hdr.code_offset + hdr.code_size;
        hdr.meta_size = meta_size;

        hdr.flags = 0;
        hdr.checksum = 0;    // TODO: CRC32

        return hdr;
    }

    MetaData buildMeta(SymbolTable& symtable, const std::vector<const DataType*>& types, bool verbose_en) {
#define verbose_cout if(verbose_en) std::cout
        MetaData meta;

        verbose_cout << "----- Building meta header -----" << std::endl;

        // 1. Data types
        for(size_t i = 0; i < types.size(); i++) {
            const DataType* t = types[i];

            MetaType mtype;
            mtype.name_offset = addStringToPool(meta.string_pool, t->name);
            mtype.size = t->size;
            mtype.flags = t->flags;

            meta.types.push_back(mtype);
            verbose_cout << "  --> Add type: " << t->name << std::endl;
        }

        // 2. Symbols
        Scope* scope = symtable.getGlobalScope();
        // TODO: iterate through all scopes

        for(auto& [name, sym] : scope->symbols) {
            MetaSymbol msym;
            msym.name_offset = addStringToPool(meta.string_pool, name);

            auto it = std::find(types.begin(), types.end(), sym.type);
            msym.type_id = (it != types.end()) ? std::distance(types.begin(), it) : 0;
            msym.stack_offset = sym.stackOffset;
            msym.flags = 0;

            meta.symbols.push_back(msym);
            verbose_cout << "  --> Add symbol: " << sym.name << std::endl;
        }

        return meta;
#undef verbose_cout
    }

    void writeBytecode(const std::string& filename, const std::vector<uint8_t>& code, const MetaData& meta, uint8_t word_size) {
        BytecodeHeader hdr = buildBytecodeHeader(code.size(), sizeof(BytecodeMetaHeader) + meta.symbols.size(), word_size);

        std::ofstream fout(filename, std::ios::binary);
        fout.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));

        // 1. Code
        fout.write(reinterpret_cast<const char*>(code.data()), code.size());

        // 2. Meta
        BytecodeMetaHeader meta_hdr{};
        meta_hdr.symbol_count = meta.symbols.size();
        meta_hdr.type_count = meta.types.size();
        meta_hdr.string_pool_size = meta.string_pool.size();
        fout.write(reinterpret_cast<char*>(&meta_hdr), sizeof(meta_hdr));

        // 2a. Types
        fout.write(reinterpret_cast<const char*>(meta.types.data()), meta.types.size() * sizeof(MetaType));

        // 2b. Symbols
        fout.write(reinterpret_cast<const char*>(meta.symbols.data()), meta.symbols.size() * sizeof(MetaSymbol));

        // 2c. String pool
        fout.write(meta.string_pool.data(), meta.string_pool.size());

        fout.close();
    }
};
