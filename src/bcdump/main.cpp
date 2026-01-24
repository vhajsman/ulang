#include "bytecode.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace ULang;

void dumpHeader(const BytecodeHeader& hdr) {
    std::cout << "------ BYTECODE HEADER ------\n";
    std::cout << "      Magic : " << std::string(hdr.magic, 6) << "\n";
    std::cout << "    Version : " << int(hdr.version_major) << "." << int(hdr.version_minor) << "\n";
    std::cout << "     Endian : " << int(hdr.endian) << "\n";
    std::cout << "  Word size : " << int(hdr.word_size) << "\n";
    std::cout << "Code offset : " << hdr.code_offset << "\n";
    std::cout << "  Code size : " << hdr.code_size << "\n";
    std::cout << "Meta offset : " << hdr.meta_offset << "\n";
    std::cout << "  Meta size : " << hdr.meta_size << "\n";
    std::cout << "      Flags : " << hdr.flags << "\n";
    std::cout << "   Checksum : " << hdr.checksum << "\n";
}

void dumpMeta(std::ifstream& f, const BytecodeHeader& hdr) {
    f.seekg(hdr.meta_offset, std::ios::beg);

    BytecodeMetaHeader meta_hdr{};
    f.read(reinterpret_cast<char*>(&meta_hdr), sizeof(meta_hdr));

    std::vector<MetaType> types(meta_hdr.type_count);
    f.read(reinterpret_cast<char*>(types.data()), types.size() * sizeof(MetaType));

    std::vector<MetaSymbol> symbols(meta_hdr.symbol_count);
    f.read(reinterpret_cast<char*>(symbols.data()), symbols.size() * sizeof(MetaSymbol));

    std::vector<char> string_pool(meta_hdr.string_pool_size);
    f.read(string_pool.data(), string_pool.size());

    for(auto& t : types)
        if(t.name_offset >= string_pool.size()) throw std::runtime_error("Type name offset out of bounds");

    for(auto& s : symbols) {
        if(s.name_offset >= string_pool.size() || s.type_id >= types.size())
            throw std::runtime_error("Symbol offset/type_id out of bounds");
    }

    std::cout << "\n------ TYPES (" << types.size() << ") ------\n";
    for(auto& t : types) {
        std::cout   << "  " << &string_pool[t.name_offset]
                    << ", size=" << int(t.size)
                    << ", flags=0x" << std::hex << int(t.flags) << std::dec << "\n";
    }

    std::cout << "\n------ SYMBOLS (" << symbols.size() << ") ------\n";
    for(auto& s : symbols) {
        std::cout   << "  " << &string_pool[s.name_offset]
                    << ", type=" << &string_pool[types[s.type_id].name_offset]
                    << ", offset=" << s.stack_offset
                    << "\n";
    }
}

int main(int argc, char** argv) {
    std::string fileName;

    po::options_description desc("ULang Bytecode Dump");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&fileName), "Binary bytecode file");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch(const po::error &e) {
        std::cerr << "Error parsing options: " << e.what() << "\n";
        return 1;
    }

    if(vm.count("help") || !vm.count("file")) {
        std::cout << desc << "\n";
        return 0;
    }

    std::ifstream f(fileName, std::ios::binary);
    if(!f) { std::cerr << "Cannot open file: " << fileName << "\n"; return 1; }

    try {
        BytecodeHeader hdr{};
        f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        dumpHeader(hdr);
        dumpMeta(f, hdr);
    } catch(const std::exception& e) {
        std::cerr << "Error reading meta: " << e.what() << "\n";
        return 1;
    }

    return 0;
}