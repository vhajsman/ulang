#include "bytecode.hpp"
#include "vm/VirtualMachine.hpp"
#include "vm/vmparams.hpp"
#include <boost/program_options/value_semantic.hpp>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace ULang;

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

int main(int argc, char** argv) {
    VMParams vmparams;

    po::options_description desc("ULang Bytecode Dump");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&vmparams.fileName), "Binary bytecode file")
        ("verbose,V", po::bool_switch(&vmparams.verbose_en)->default_value(false), "Enable verbose debug outputs")
        ("heapsize-start", po::value(&vmparams.heapsize_start_kb)->default_value(256), "Starting virtual memory size to allocate (in kB, default: 256)")
        ("heapsize-limit", po::value(&vmparams.heapsize_limit_kb)->default_value(0), "Maximal virtual memory size to allocate (in kB, 0 for unlimited, default: 0)");

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

    std::ifstream f(vmparams.fileName, std::ios::binary);
    if(!f) { 
        std::cerr << "Cannot open file: " << vmparams.fileName << "\n"; 
        return 1; 
    }

    VirtualMachine vmachine(vmparams);
    
    try {
        vmachine.init();
        
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

        std::vector<Instruction> instructions;

        size_t pc = hdr.code_offset;
        while(pc < hdr.code_offset + hdr.code_size && pc < buf.size()) {
            Instruction instr = readInstruction(buf, pc);
            instructions.push_back(instr);
        }

        if(vmparams.verbose_en)
            std::cout << "BOOT: Instructions read: " << instructions.size() << std::endl;

        vmachine.run(instructions);

    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;

        f.close();
        return 1;
    };

    f.close();
    return 0;
}