#include "vm/VirtualMachine.hpp"
#include <boost/program_options/value_semantic.hpp>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    std::string fileName;
    bool verbose_en;

    size_t heapsize_start_kb;
    size_t heapsize_limit_kb;

    po::options_description desc("ULang Bytecode Dump");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&fileName), "Binary bytecode file")
        ("verbose,V", po::bool_switch(&verbose_en)->default_value(false), "Enable verbose debug outputs")
        ("heapsize-start", po::value(&heapsize_start_kb)->default_value(256), "Starting virtual memory size to allocate (in kB, default: 256)")
        ("heapsize-limit", po::value(&heapsize_limit_kb)->default_value(0), "Maximal virtual memory size to allocate (in kB, 0 for unlimited, default: 0)");

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

    ULang::VirtualMachine vmachine(verbose_en, heapsize_start_kb, heapsize_limit_kb);
    
    try {
        vmachine.init();
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;

        f.close();
        return 1;
    };

    return 0;
}