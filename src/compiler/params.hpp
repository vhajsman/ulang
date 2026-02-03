#ifndef __ULANG_COMPILER_PARAMS_H
#define __ULANG_COMPILER_PARAMS_H

#include <string>
namespace ULang {
    struct CompilerParameters {
        bool verbose;
        std::string sourceFile;
        std::string outFile;

        // --- optimalization ---
        bool OExplicitZero; ///< Whether declaration without assignment should explicitely assign zero
    };
};

#endif
