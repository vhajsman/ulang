#include "bytecode.hpp"
#include <boost/program_options.hpp>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace po = boost::program_options;
using namespace ULang;

std::unordered_map<Opcode, int> operand_count = { 
    {Opcode::NOP, 0}, 
    {Opcode::PUSH, 1}, 
    {Opcode::POP, 1}, 
    {Opcode::ADD, 2}, 
    {Opcode::SUB, 2}, 
    {Opcode::MUL, 2}, 
    {Opcode::DIV, 2}, 
    {Opcode::LD, 1}, 
    {Opcode::ST, 1}, 
    {Opcode::JMP, 1}, 
    {Opcode::JZ, 1},
    {Opcode::CALL, 1},
    {Opcode::RET, 0},
    {Opcode::HALT, 0}
};

// HEX helper
static inline std::string HEX(uint64_t no, std::string suffix = "h") {
    std::stringstream stream;
    stream << std::hex << no << suffix;
    return stream.str();
}

Operand readOperand(const std::vector<uint8_t>& buf, size_t& pc) {
    Operand op {};
    if(pc + 2 > buf.size())
        return op;

    op.type = static_cast<OperandType>(buf[pc++]);
    op.data = 0;

    for(int i = 0; pc < buf.size(); i++)
        op.data |= uint32_t(buf[pc++]) << (8 * i);

    return op;
}

Instruction readInstruction(const std::vector<uint8_t>& buf, size_t& pc) {
    Instruction instr {};
    instr.offset = pc;

    if(pc + 11 >= buf.size())
        return instr;

    instr.opcode = static_cast<Opcode>(buf[pc++]);

    for(uint8_t i = 0; i < 2; ++i) {
        Operand op{};
        op.type = static_cast<OperandType>(buf[pc++]);
        op.data = 0;

        for(int b = 0; b < 4; b++)
            op.data |= uint32_t(buf[pc++]) << (8 * b);

        instr.operands.push_back(op);
    }

    return instr;
}

MetaData readMeta(const std::vector<uint8_t>& buf, const BytecodeHeader& hdr) {
    MetaData meta {};
    if(hdr.meta_size == 0)
        return meta;

    if(buf.size() < hdr.meta_offset + hdr.meta_size)
        throw std::runtime_error("Bytecode truncated: meta section incomplete");

    size_t pos = hdr.meta_offset;

    BytecodeMetaHeader meta_hdr {};
    std::memcpy(&meta_hdr, buf.data() + pos, sizeof(BytecodeMetaHeader));
    pos += sizeof(BytecodeMetaHeader);

    meta.types.resize(meta_hdr.type_count);
    if(meta_hdr.type_count > 0) {
        std::memcpy(meta.types.data(), buf.data() + pos, meta_hdr.type_count * sizeof(MetaType));
        pos += meta_hdr.type_count * sizeof(MetaType);
    }

    meta.symbols.resize(meta_hdr.symbol_count);
    if(meta_hdr.symbol_count > 0) {
        std::memcpy(meta.symbols.data(), buf.data() + pos, meta_hdr.symbol_count * sizeof(MetaSymbol));
        pos += meta_hdr.symbol_count * sizeof(MetaSymbol);
    }

    if(meta_hdr.string_pool_size > 0) {
        if(pos + meta_hdr.string_pool_size > buf.size())
            throw std::runtime_error("Bytecode truncated: string pool incomplete");

        meta.string_pool.assign(reinterpret_cast<const char*>(buf.data() + pos), meta_hdr.string_pool_size);
    }

    return meta;
}

std::string fmtOperand(const Operand& operand, const MetaData& meta, bool do_sym) {
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

            if(do_sym && !meta.symbols.empty()) {
                for(const auto& sym : meta.symbols) {
                    if(val == sym.stack_offset && sym.name_offset < meta.string_pool.size()) {
                        return "&" + std::string(meta.string_pool.data() + sym.name_offset);
                    }
                }
            }

            return "&" + HEX(val);
        }

        case OperandType::OP_REGISTER:
            return "R" + std::to_string(operand.data); // TODO
    }

    return "???";
}

int main(int argc, char** argv) {
    std::string fileName;
    bool do_bin = false;
    bool do_sym = false;

    po::options_description desc("ULang Disassembler Options");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&fileName), "Binary bytecode file")
        ("bin,b", po::bool_switch(&do_bin), "Show binary")
        ("symbols,s", po::bool_switch(&do_sym), "Decode symbol table");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help") || !vm.count("file")) {
        std::cout << desc << "\n";
        return 0;
    }

    std::ifstream f(fileName, std::ios::binary);
    if(!f) {
        std::cerr << "Cannot open file: " << fileName << "\n";
        return 1;
    }

    f.seekg(0, std::ios::end);
    size_t size = f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    f.close();

    if(buf.size() < sizeof(BytecodeHeader)) {
        std::cerr << "File smaller than header structure\n";
        return 1;
    }

    BytecodeHeader hdr {};
    std::memcpy(&hdr, buf.data(), sizeof(BytecodeHeader));

    if(!validateHeader(hdr, buf.size())) {
        std::cerr << "Invalid header\n";
        return 1;
    }

    MetaData meta = readMeta(buf, hdr);

    std::vector<Instruction> instructions;

    size_t pc = hdr.code_offset;
    while(pc < hdr.code_offset + hdr.code_size && pc < buf.size()) {
        Instruction instr = readInstruction(buf, pc);
        instructions.push_back(instr);
    }

    std::cout << "Instructions read: " << instructions.size() << std::endl;

    for(const auto& instr: instructions) {
        std::cout << std::setw(8) << std::setfill('0') << std::hex << instr.offset << " | ";

        if(do_bin) {
            size_t instr_end = instr.offset + 1 + /*opA.size + opB.size*/ sizeof(uint32_t) * 2;
            for(size_t i = instr.offset; i < instr_end && i < buf.size(); ++i)
                std::cout << std::setw(2) << std::setfill('0') << std::hex << (int) buf[i] << " ";
            std::cout << " | ";
        }

        std::cout << opcodeToStr(instr.opcode);
        for(const auto& op : instr.operands) {
            std::cout << " " << fmtOperand(op, meta, do_sym);
        }

        std::cout << std::endl;
    }

    return 0;
}
