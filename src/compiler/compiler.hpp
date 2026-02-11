#ifndef __ULANG_COMPILER_H
#define __ULANG_COMPILER_H

#include "bytecode.hpp"
#include "compiler/params.hpp"
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

    static inline void write_bytes(std::vector<uint8_t>& out, const void* src, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(src);
        out.insert(out.end(), p, p + size);
    }
    struct SourceLocation {
        void* loc_parent;
        std::string loc_file;
        size_t loc_line;
        size_t loc_col;
    };
#ifndef ULANG_LOCATION_NULL
    #define ULANG_LOCATION_NULL (ULang::SourceLocation) {nullptr, "(unknown)", 0, 0}
#endif

    // ==================================================================
    // ======== AST NODES
    // ==================================================================

    enum class ASTNodeType {
        NUMBER,         ///< integral value
        VARIABLE,       ///< variable reference
        BINOP,          ///< binary operator
        DECLARATION,    ///< declaration (int myVar = ...)
        ASSIGNMENT,     ///< value assignment (myVar = ...)
        FN_DEF,         ///< function definition
        FN_CALL,        ///< function call
        FN_ARG,         ///< function argument
        FN_RET,         ///< function return
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
        uint32_t val;                    ///< value (if ASTNodeType::NUMBER)

        std::string name;               ///< name

        BinopType op;                   ///< operator (if ASTNodeType::BINOP)
        ASTNode* lefthand = nullptr;    ///< lefthand operand
        ASTNode* righthand = nullptr;   ///< righthand operand
        ASTNode* initial = nullptr;     ///< initial value (if ASTNodeType::DECLARATION)

        Symbol* symbol = nullptr;       ///< symbol

        std::vector<ASTNode*> body;     ///< function body (if ASTNodeType::FN_DEF)
        std::vector<ASTNode*> args;     ///< function args (if ASTNodeType::FN_CALL)
        const DataType* ret_type = nullptr;

        ASTNode(int64_t val);
        ASTNode(const std::string& varname);
        ASTNode(ASTNodeType t);
    };

    using ASTPtr = std::unique_ptr<ASTNode>;

    // ==================================================================
    // ======== TOKENS AND LEXER
    // ==================================================================

    enum class TokenType {
        LParen,
        RParen,
        LCurly,
        RCurly,
        Comma,
        TypeKeyword,
        Identifier,
        Number,
        Plus,
        Minus,
        Mul,
        Div,
        Assign,
        Semicolon,
        Function,
        Return,
        EndOfFile
    };

    inline std::string toktype2str(TokenType tt){
        switch (tt) {
            case TokenType::LParen:         return "'('";
            case TokenType::RParen:         return "')'";
            case TokenType::LCurly:         return "'{'";
            case TokenType::RCurly:         return "'}'";
            case TokenType::Comma:          return "','";
            case TokenType::TypeKeyword:    return "type keyword";
            case TokenType::Identifier:     return "identifier";
            case TokenType::Number:         return "number";
            case TokenType::Plus:           return "addition";
            case TokenType::Minus:          return "substraction";
            case TokenType::Mul:            return "multiplication";
            case TokenType::Div:            return "division";
            case TokenType::Assign:         return "assignment";
            case TokenType::Return:         return "return statement";
            case TokenType::Semicolon:      return "','";
            case TokenType::Function:       return "function";
            case TokenType::EndOfFile:      return "EOF";
        }
    }

    struct Token {
        TokenType type;
        std::string text;
        SourceLocation loc;
    };

    /**
     * @brief converts source to tokens
     */
    class Lexer {
        private:
        std::string src;    ///< source code

        size_t pos = 0;     ///< current position
        size_t line = 1;    ///< line number
        size_t col = 1;     ///< column number

        char peek() const;
        char get();

        public:

        /**
         * @brief converts source code to tokens
         * @exception std::runtime_error
         * @return std::vector<Token> token vector
         */
        std::vector<Token> tokenize(const std::string& filename = "(unknown)");

        Lexer(const std::string& input);
    };

    // std::vector<ASTNode*> buildAST(const std::vector<Token>& tokens);
    
    // ASTNode* parsePrimary(const std::vector<Token>& tokens, size_t& pos);
    // ASTNode* parseExpression(const std::vector<Token>& tokens, size_t& pos);
    // ASTNode* parseVarDecl(const std::vector<Token>& tokens, size_t& pos);

    // ==================================================================
    // ======== SYMBOLS AND SYMBOL TABLE
    // ==================================================================

    enum class SymbolKind {
        VARIABLE,
        FUNCTION
    };

    struct Symbol {
        std::string name;
        unsigned int symbolId;

        SymbolKind kind = SymbolKind::VARIABLE;
        const DataType* type;

        size_t stackOffset;
        uint32_t entry_ip; ///< functions only

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

        /**
         * @brief Declares a function symbol in current scope
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
        Symbol* decl_fn(const std::string& name,
                        const DataType* ret_type,
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

        /**
         * @brief Declares a function symbol in current scope
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
        Symbol* decl_fn(const std::string& name,
                        const DataType* ret_type,
                        SourceLocation* where = nullptr,
                        size_t align_head = 0,
                        size_t align_tail = 0);
        
        const Symbol* lookup(const std::string& name) const;
        const Symbol* lookup(unsigned int symbolId) const;
        
        Scope* getCurrentScope() const;
        Scope* getGlobalScope() const;
    };

    // ==================================================================
    // ======== COMPILER EXCEPTIONS
    // ==================================================================

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

    // ==================================================================
    // ======== COMPILATION, BYTECODE AND CONTEXT
    // ==================================================================

    BytecodeHeader buildBytecodeHeader(uint32_t code_size, uint32_t meta_size, uint8_t word_size);
    MetaData buildMeta(SymbolTable& symtable, const std::vector<const DataType*>& types0, bool verbose_en);
    void writeBytecode(const std::string& filename, const std::vector<uint8_t>& code, const MetaData& meta, uint8_t word_size);

    struct GenerationContext {
        std::vector<Instruction> instructions;
        SymbolTable* symtab;
        uint32_t stack_top;
    };

    // ==================================================================
    // ======== COMPILER INSTANCE
    // ==================================================================

    class CompilerInstance {
        private:
        std::vector<Token> tokens;  ///< List of all the tokens from the source
        SymbolTable symbols;        ///< Symbol table
        Lexer lexer;                ///< Lexer instance

        std::vector<ASTPtr> ast_owned;

        CompilerParameters cparams;

        size_t pos = 0;             ///< Current position

        /**
         * @brief CompilerSyntaxException exceptions not terminating the compilation
         * 
         */
        std::vector<CompilerSyntaxException> exceptions_friendly;

        private:

        /**
         * @brief Throws a friendly exception which does not terminate the compilation
         * 
         * @param e exception
         */
        void friendlyException(CompilerSyntaxException e);

        /**
         * @brief Get the data type
         * 
         * @param node node
         * @param loc source location for logging reference
         * @return const DataType* 
         */
        const DataType* getType(ASTNode* node, SourceLocation loc = ULANG_LOCATION_NULL);

        const DataType* determineBinopType(const DataType* left, const DataType* right);

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
        bool matchToken(const std::string& token);

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
         * @brief parses function declaration
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parseFnDecl();

        /**
         * @brief parses statement
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parseStatement();

        /**
         * @brief parses code block (a sequence of statements)
         * @exception std::runtime_error
         * @return std::vector<ASTNode*> a vector of pointers to new AST nodes
         */
        std::vector<ASTNode*> parseBlock();

        /**
         * @brief parses arithmetical expression
         * @param prec_min minimal precedence (default: 0)
         * @exception std::runtime_error
         * @return ASTNode* pointer to new AST node
         */
        ASTNode* parseExpression(int prec_min = 0);

        // void registerFunctions();

        /**
         * @brief generates AST based on tokens
         * @throws std::runtime_error
         */
        void buildAST();

        Instruction makeInstruction(Opcode opcode, Operand a = {}, Operand b = {});
        Operand makeIMM(uint32_t val = 0);
        Operand makeIMMu32(uint32_t val = 0);
        Operand makeRef(uint32_t offset);

        void serializeInstruction(const Instruction& instr, std::vector<uint8_t>& out);
        std::vector<uint8_t> serializeProgram(const std::vector<Instruction>& program);

        Operand compileNode(ASTNode* node, std::vector<Instruction>& out);

        void emit(GenerationContext& ctx, Opcode opcode, const Operand& op_a, const Operand& op_b);

        public:
        CompilerInstance(const std::string& source, CompilerParameters& cparams);

        /**
         * @brief does the compilation
         * @exception std::runtime_error
         */
        void compile();
    };
};

#endif
