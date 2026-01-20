#include "compiler.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <string>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    std::string sourceFile;
    std::string outFile;

    po::options_description desc("ULang Compiler Options");
    desc.add_options()
        ("help,h", "Show help")
        ("file,f", po::value<std::string>(&sourceFile), "Source file")
        ("output,o", po::value<std::string>(&outFile)->default_value("a.out"), "Output file");

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

    std::ifstream file(sourceFile);
    if(!file.is_open()) {
        std::cerr << "Cannot open file: " << sourceFile << "\n";
        return 1;
    }

    std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::cout << "Loaded " << sourceCode.size() << " bytes from" << sourceFile << ", output: " << outFile << "\n";

    ULang::Lexer lexer(sourceCode);
    THROW_AWAY lexer.tokenize();

    ULang::CompilerInstance* ci = new ULang::CompilerInstance(sourceCode, sourceFile);
    THROW_AWAY ci->compile();
    std::cout << "Compile OK" << std::endl;

    delete ci;
}