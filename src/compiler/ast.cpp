#include "compiler.hpp"
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ULang {
    ASTNode::ASTNode(int64_t val)
    : type(ASTNodeType::NUMBER), val(val) {}

    ASTNode::ASTNode(const std::string& varname)
    : type(ASTNodeType::VARIABLE), name(varname) {}

    ASTNode::ASTNode(ASTNodeType t)
    : type(t) {}

    /*
    ASTNode* parsePrimary(const std::vector<Token>& tokens, size_t& pos) {
        if(tokens[pos].type == TokenType::Number) {
            ASTNode* n = new ASTNode(ASTNodeType::NUMBER);
            n->val = std::stoi(tokens[pos].text);
            pos++;

            return n;
        } else if(tokens[pos].type == TokenType::Identifier) {
            ASTNode* v = new ASTNode(ASTNodeType::VARIABLE);
            v->name = tokens[pos].text;
            pos++;

            return v;
        } else {
            throw std::runtime_error("Unexpected token in expression: " + tokens[pos].text);
        }
    }

    ASTNode* parseExpression(const std::vector<Token>& tokens, size_t& pos) {
        ASTNode* left = parsePrimary(tokens, pos);

        while(pos < tokens.size()) {
            BinopType op;
            if(tokens[pos].type == TokenType::Plus)         op = BinopType::ADDITION;
            else if(tokens[pos].type == TokenType::Minus)   op = BinopType::SUBSTRACTION;
            else if(tokens[pos].type == TokenType::Mul)     op = BinopType::MULTIPLICATION;
            else if(tokens[pos].type == TokenType::Div)     op = BinopType::DIVISION;
            else break;

            pos++;
            ASTNode* right = parsePrimary(tokens, pos);

            ASTNode* bin = new ASTNode(ASTNodeType::BINOP);
            bin->op         = op;
            bin->lefthand   = left;
            bin->righthand  = right;

            left = bin;
        }

        return left;
    }

    ASTNode* parseVarDecl(const std::vector<Token>& tokens, size_t& pos, Symbol* sym = nullptr) {
        if(tokens[pos].type != TokenType::TypeKeyword)
            throw std::runtime_error("Expected 'int'");

        pos++;

        if(tokens[pos].type != TokenType::Identifier)
            throw std::runtime_error("Expected identifier");
        std::string varName = tokens[pos].text;
        pos++;

        if(tokens[pos].type != TokenType::Assign)
            throw std::runtime_error("Expected '='");
        pos++;

        ASTNode* expr = parseExpression(tokens, pos);

        if(tokens[pos].type != TokenType::Semicolon)
            throw std::runtime_error("Expected ';'");
        pos++;

        ASTNode* node = new ASTNode(ASTNodeType::DECLARATION);
        node->name = varName;
        node->initial = expr;

        return node;
    }
    */
};