#include "compiler.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include "params.hpp"

namespace po = boost::program_options;

int main(int argc, char** argv) {
    ULang::CompilerParameters cparams;


    po::options_description desc("ULang Compiler Options");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&cparams.sourceFile), "Source file")
        ("output,o", po::value<std::string>(&cparams.outFile)->default_value("a.out"), "Output file")
        ("verbose", po::bool_switch(&cparams.verbose)->default_value(false), "Generate verbose compilation log");

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

    std::ifstream file(cparams.sourceFile);
    if(!file.is_open()) {
        std::cerr << "Cannot open file: " << cparams.sourceFile << "\n";
        return 1;
    }

    std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::cout << "Loaded " << sourceCode.size() << " bytes from" << cparams.sourceFile << ", output: " << cparams.outFile << "\n";

    ULang::Lexer lexer(sourceCode);
    THROW_AWAY lexer.tokenize();

    ULang::CompilerInstance* ci = new ULang::CompilerInstance(sourceCode, cparams);
    THROW_AWAY ci->compile();
    std::cout << "Compile OK" << std::endl;

    delete ci;
    return 0;
}