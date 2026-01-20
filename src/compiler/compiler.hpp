#ifndef __ULANG_COMPILER_H
#define __ULANG_COMPILER_H

#include "types.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#ifndef THROW_AWAY
#define THROW_AWAY (void)
#endif

namespace ULang {
    struct Symbol;

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

        Symbol* symbol = nullptr;

        ASTNode(int64_t val);
        ASTNode(const std::string& varname);
        ASTNode(ASTNodeType t);
    };

    using ASTPtr = std::unique_ptr<ASTNode>;

    enum class TokenType {
        TypeKeyword,
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
        std::string src;
        size_t pos = 0;

        char peek() const;
        char get();

        public:
        std::vector<Token> tokenize();

        Lexer(const std::string& input);
    };

    // std::vector<ASTNode*> buildAST(const std::vector<Token>& tokens);
    
    // ASTNode* parsePrimary(const std::vector<Token>& tokens, size_t& pos);
    // ASTNode* parseExpression(const std::vector<Token>& tokens, size_t& pos);
    // ASTNode* parseVarDecl(const std::vector<Token>& tokens, size_t& pos);
    struct SourceLocation {
        void* loc_parent;
        std::string loc_file;
        int loc_line;
        int loc_col;
    };

    struct Symbol {
        std::string name;
        unsigned int symbolId;

        const DataType* type;
        size_t stackOffset;

        SourceLocation where;
    };

    struct Scope {
        std::string _name;

        std::unordered_map<std::string, Symbol> symbols;
        Scope* parent = nullptr;
        size_t nextOffset;

        Symbol* decl(   const std::string& name, 
                        const DataType* type, 
                        SourceLocation* where = nullptr, 
                        size_t align_head = 0, 
                        size_t align_tail = 0);
        
        const Symbol* lookup(const std::string& name) const;
        const Symbol* lookup(unsigned int symbolId) const;
    };

    class SymbolTable {
        private:
        Scope* scope_global;
        Scope* scope_current;

        unsigned int nextSymbolId;

        public:
        SymbolTable();
        ~SymbolTable();

        Scope* enter(const std::string& name);
        Scope* leave();

        Symbol* decl(   const std::string& name, 
                        const DataType* type, 
                        SourceLocation* loc = nullptr, 
                        size_t align_head = 0, 
                        size_t align_tail = 0);
        
        const Symbol* lookup(const std::string& name) const;
        const Symbol* lookup(unsigned int symbolId) const;
        
        Scope* getCurrentScope() const;
        Scope* getGlobalScope() const;
    };

    class CompilerInstance {
        private:
        std::vector<Token> tokens;
        SymbolTable symbols;
        Lexer lexer;

        std::vector<ASTPtr> ast_owned;

        std::string filename;
        size_t pos = 0;

        private:
        /**
         * @brief Throws an expection if unexcepted token, returns the token if else.
         * @exception std::runtime_error
         * @param type token type
         * @return const Token& 
         */
        const Token& expectToken(TokenType type);

        /**
         * @brief Throws an expection if unexcepted token, returns the token if else.
         * @exception std::runtime_error
         * @param type token text
         * @return const Token& 
         */
        const Token& expectToken(const std::string& token);

        bool matchToken(TokenType type);

        int precedence(TokenType type);

        ASTNode* parsePrimary();
        ASTNode* parseVarDecl();
        ASTNode* parseExpression(int prec_min = 0);

        void /*std::vector<ASTNode*>*/ buildAST();

        public:
        CompilerInstance(const std::string& source, const std::string& filename = "unnamed");
        void compile();
    };
};

#endif
