#include "compiler.hpp"

namespace ULang {
    bool isBinop(TokenType tt) {
        return tt == TokenType::Plus || tt == TokenType::Minus || tt == TokenType::Div || tt == TokenType::Mul;
    }

    const DataType* CompilerInstance::determineBinopType(const DataType* left, const DataType* right) {
        if(left->size > right->size) return left;
        if(right->size > left->size) return right;

        if((left->flags & SIGN) != (right->flags & SIGN)) {
            return (left->flags & SIGN) ? left : right;
        }

        return left;
    }
};