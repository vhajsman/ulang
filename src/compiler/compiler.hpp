#ifndef __ULANG_COMPILER_H
#define __ULANG_COMPILER_H

#include <cstdint>
#include <string>

namespace ULang {
    enum class ASTNodeType {
        NUMBER,         // integral value
        VARIABLE,       // variable reference
        BINOP,          // binary operator
        DECLARATION,    // int myVar = ...
        ASSIGNMENT      // myVar = expr
    };

    enum class BinopType {
        ADDITION,
        SUBSTRACTION,
        MULTIPLICATION,
        DIVISION
    };

    struct ASTNode {
        ASTNodeType type;
        int64_t val;

        std::string name;

        BinopType op;
        ASTNode* lefthand = nullptr; ASTNode* righthand = nullptr;
        ASTNode* initial = nullptr;

        ASTNode(int64_t val);
        ASTNode(const std::string& varname);
        ASTNode(ASTNodeType t);
    };
};

#endif
