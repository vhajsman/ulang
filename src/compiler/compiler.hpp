#ifndef __ULANG_COMPILER_H
#define __ULANG_COMPILER_H

#include <cstdint>
#include <string>
#include <vector>

#ifndef THROW_AWAY
#define THROW_AWAY (void)
#endif

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

    enum class TokenType {
        IntKeyword,
        Identifier,
        Number,
        Plus,
        Minus,
        Mul,
        Div,
        Assign,
        Semicolon,
        EndOfFile
    };

    struct Token {
        TokenType type;
        std::string text;
    };

    class Lexer {
        private:
        const std::string& src;
        size_t pos = 0;

        char peek() const;
        char get();

        public:
        std::vector<Token> tokenize();

        Lexer(const std::string& input);
    };

    std::vector<ASTNode*> buildAST(const std::vector<Token>& tokens);
};

#endif
