#include "bytecode.hpp"
#include "compiler.hpp"
#include "types.hpp"
#include <stdexcept>
#include <string>

namespace ULang {
    void emit_uPutChar(CompilerInstance& ci, GenerationContext* ctx, ASTNode* node) {
        if(node->args.size() != 1)
            throw std::runtime_error("builtin:uPutChar() requires 1 argument");

        ci.compileNode(node->args[0], ctx->instructions);
        ci.emit(*ctx, PUTC, {
            OperandType::OP_IMMEDIATE, 
            node->args[0]->val}, 
        {OperandType::OP_NULL});
    }

    void emit_uGetChar(CompilerInstance& ci, GenerationContext* ctx, ASTNode* node) {
        if(node->args.size() != 1)
            throw std::runtime_error("builtin:uGetChar() requires 1 argument");

        Operand tmp = ci.allocTmpReg();
        ci.emit(*ctx, GETC, tmp, {OperandType::OP_NULL});
        ci.freeTmpReg(tmp);
    }

    // TODO: better name prefix generator
    std::vector<Builtin> builtin_list = {
        {"___uV0a042_builtin_uPutChar", "uPutChar", {&TYPE_CHAR}, nullptr},
        {"___uV0a042_builtin_uGetChar", "uGetChar", {}, nullptr}
    };

    ASTNode* CompilerInstance::insertBuiltinNode(Builtin& builtin) {
        ASTNode* node = new ASTNode(ASTNodeType::FN_DEF);
        node->name = builtin.alias;

        // TODO: better return types
        // Symbol* sym = this->symbols.decl_fn(builtin.name, &TYPE_UINT32);
        // Symbol* sym = builtin.symbol;
        // builtin.symbol = sym;

        node->symbol = builtin.symbol;

        // arguments
        for(const DataType* arg_type: builtin.args) {
            this->symbols.enter(builtin.alias);

            Symbol*  arg_sym  = this->symbols.decl("arg", arg_type);
            ASTNode* arg_node = new ASTNode(ASTNodeType::FN_ARG);
            arg_node->name = arg_sym->name;
            arg_node->symbol = arg_sym;

            node->args.push_back(arg_node);
            this->symbols.leave();
        }

        // body is leaved empty, emitter uses its callback
        node->body = {};

        this->ast_owned.insert(this->ast_owned.begin(), ASTPtr(node));
        return node;
    }

    bool isBuiltin(const std::string& str) { // FIXME
        for(std::string& curr: builtin_ids)
            if(str == curr) return true;

        return false;
    }

    /*
    void CompilerInstance::checkBuiltinRedecl(const std::string& str, SourceLocation* loc) {
        if(isBuiltin((std::string&) str) && !this->cparams.excludeBuiltin) {
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error,
                "'" + str + "' was already declared as builtin symbol",
                loc == nullptr ? ULANG_LOCATION_NULL : *loc,
                ULANG_SYNT_ERR_BUILTIN_REDECL
            );
        }
    }
    */

    void CompilerInstance::checkBuiltinRedecl(const std::string& str, SymbolOrigin origin, SourceLocation* loc) {
        if(origin != SymbolOrigin::USER)
            return;

        //this->checkBuiltinRedecl(str, loc);

        if(isBuiltin((std::string&) str) && !this->cparams.excludeBuiltin) {
            throw CompilerSyntaxException(
                CompilerSyntaxException::Severity::Error,
                "'" + str + "' was already declared as builtin symbol",
                loc == nullptr ? ULANG_LOCATION_NULL : *loc,
                ULANG_SYNT_ERR_BUILTIN_REDECL
            );
        }
    }

    Builtin* CompilerInstance::findBuiltin(const std::string& name) {
        if(this->cparams.excludeBuiltin)
            return nullptr;

        for(Builtin& curr: builtin_list) {
            if(curr.name == name)
                return &curr;
        }

        return nullptr;
    }

    void CompilerInstance::registerBuiltin(Builtin& builtin) {
        if(!builtin.enabled)
            return;

        Symbol* sym_existing = (Symbol*) this->symbols.lookup(builtin.alias);
        if(sym_existing) {
            if(sym_existing->origin == SymbolOrigin::USER)
                this->checkBuiltinRedecl(builtin.alias, SymbolOrigin::USER);
            
            builtin.symbol = sym_existing;
            return;
        }

        // TODO: better return types
        Symbol* sym_new = this->symbols.decl_fn(builtin.alias, &TYPE_UINT32, nullptr, SymbolOrigin::BUILTIN);
        builtin.symbol = sym_new;
    }

    void CompilerInstance::subconstructor_builtin() {
        if(this->cparams.excludeBuiltin)
            return;

        for(Builtin& b: builtin_list) {
            this->registerBuiltin(b);
            this->insertBuiltinNode(b);
        }
    }
};