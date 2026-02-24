#include "compiler.hpp"

namespace ULang {
    std::vector<ASTNode*> CompilerInstance::parseBlock() {
        this->expectToken(TokenType::LCurly);
        
        std::vector<ASTNode *> body;
        while(this->tokens[this->pos].type != TokenType::RCurly && this->tokens[this->pos].type != TokenType::EndOfFile) {
            ASTNode* stmt_curr = this->parseStatement();
            if(stmt_curr)
                body.push_back(stmt_curr);
        }
        
        this->expectToken(TokenType::RCurly);
        return body;
    }
};