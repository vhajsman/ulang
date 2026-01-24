#ifndef __ULANG_COMPILER_H
#define __ULANG_COMPILER_H

#include "types.hpp"

#include <cstdint>
#include <exception>
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
        NUMBER,         ///< integral value
        VARIABLE,       ///< variable reference
        BINOP,          ///< binary operator
        DECLARATION,    ///< declaration (int myVar = ...)
        ASSIGNMENT      ///< value assignment (myVar = ...)
    };

    enum class BinopType {
        ADDITION,       ///< addition
        SUBSTRACTION,   ///< substraction
        MULTIPLICATION, ///< multiplication
        DIVISION        ///< division
    };

    /**
     * @brief AST nodes
     */
    struct ASTNode {
        ASTNodeType type;               ///< node type
        int64_t val;                    ///< value (if ASTNodeType::NUMBER)

        std::string name;               ///< name

        BinopType op;                   ///< operator (if ASTNodeType::BINOP)
        ASTNode* lefthand = nullptr;    ///< lefthand operand
        ASTNode* righthand = nullptr;   ///< righthand operand
        ASTNode* initial = nullptr;     ///< initial value (if ASTNodeType::DECLARATION)

        Symbol* symbol = nullptr;       ///< symbol

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

    /**
     * @brief converts source to tokens
     */
    class Lexer {
        private:
        std::string src;    ///< source code
        size_t pos = 0;     ///< current position

        char peek() const;
        char get();

        public:

        /**
         * @brief converts source code to tokens
         * @exception std::runtime_error
         * @return std::vector<Token> token vector
         */
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

#ifndef ULANG_LOCATION_NULL
#define ULANG_LOCATION_NULL (ULang::SourceLocation) {nullptr, "(unknown)", 0, 0}
#endif

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

        /**
         * @brief Declares a symbol in current scope
         * 
         * @exception std::runtime_error
         * 
         * @param name symbol name
         * @param type data type
         * @param where position in source code (default: nullptr)
         * @param align_head alignment before (default: 0)
         * @param align_tail alignment after (default: 0)
         * @return Symbol* symbol pointer
         */
        Symbol* decl(   const std::string& name, 
                        const DataType* type, 
                        SourceLocation* where = nullptr, 
                        size_t align_head = 0, 
                        size_t align_tail = 0);
        
        const Symbol* lookup(const std::string& name) const;
        const Symbol* lookup(unsigned int symbolId) const;

        ~Scope();
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


        /**
         * @brief Declares a symbol in current scope
         * 
         * @exception std::runtime_error
         * 
         * @param name symbol name
         * @param type data type
         * @param loc position in source code (default: nullptr)
         * @param align_head alignment before (default: 0)
         * @param align_tail alignment after (default: 0)
         * @return Symbol* symbol pointer
         */
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

    class CompilerSyntaxException: public std::exception {
        public:
        enum class Severity {Error, Warning, Note};

        private:
        Severity severity;
        SourceLocation loc;
        std::string msg;
        unsigned int errnum;

        public:
        CompilerSyntaxException(Severity severity, const std::string& m, SourceLocation loc, unsigned int errnum = 0);

        const char* what() const noexcept override {return this->msg.c_str();};

        /**
         * @brief formatted compiler exception output (human readable)
         * 
         * @param en_color whether to enable ANSII escapes for colors
         * @param line source code (optional)
         * @return std::string formatted string
         */
        std::string fmt(bool en_color, std::string line = "") const;


        /**
         * @brief formatted compiler exception output (JSON)
         * 
         * @return std::string formatted string
         */
        std::string fmt_json() const;
        
        Severity getSeverity() const;
        SourceLocation getLocation() const;
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
         * @param loc location in source (optional)
         * @return const Token& 
         */
        const Token& expectToken(TokenType type, SourceLocation loc = ULANG_LOCATION_NULL);

        /**
         * @brief Throws an expection if unexcepted token, returns the token if else.
         * @exception std::runtime_error
         * @param type token text
         * @param loc source location (optional)
         * @return const Token& 
         */
        const Token& expectToken(const std::string& token, SourceLocation loc = ULANG_LOCATION_NULL);

        bool matchToken(TokenType type);

        int precedence(TokenType type);

        /**
         * @brief parses primary expression
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parsePrimary();


        /**
         * @brief parses variable declaration
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parseVarDecl();


        /**
         * @brief parses arithmetical expression
         * @param prec_min minimal precedence (default: 0)
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parseExpression(int prec_min = 0);

        /**
         * @brief generates AST based on tokens
         * @throws std::runtime_error
         */
        void buildAST();

        public:
        CompilerInstance(const std::string& source, const std::string& filename = "unnamed");

        /**
         * @brief does the compilation
         * @exception std::runtime_error
         */
        void compile();
    };
};

#endif
