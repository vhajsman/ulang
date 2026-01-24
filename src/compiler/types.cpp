#include "types.hpp"
#include "compiler.hpp"
#include "errno.h"

namespace ULang {
    const DataType* getType(ASTNode* node, SourceLocation loc) {
        if(!node)
            return nullptr;

        switch(node->type) {
            case ASTNodeType::DECLARATION: 
                return node->symbol ? node->symbol->type : nullptr;

            case ASTNodeType::ASSIGNMENT:
                return node->righthand ? getType(node->righthand, loc) : nullptr;
                
            case ASTNodeType::NUMBER:
                return &TYPE_INT32;

            case ASTNodeType::VARIABLE:
                if(node->symbol) 
                    return node->symbol->type;

                throw CompilerSyntaxException(
                    CompilerSyntaxException::Severity::Error,
                    "Could not determine type for '" + node->name + "'",
                    loc,
                    ULANG_SYNT_ERR_TYPE_DETERMINE_FAIL
                );

            case ASTNodeType::BINOP: {
                // fallback loc pro levý a pravý operand
                SourceLocation loc_left  = node->lefthand && node->lefthand->symbol ? node->lefthand->symbol->where : loc;
                SourceLocation loc_right = node->righthand && node->righthand->symbol ? node->righthand->symbol->where : loc;

                const DataType* left  = getType(node->lefthand, loc_left);
                const DataType* right = getType(node->righthand, loc_right);

                if((left->flags & SIGN) != (right->flags & SIGN)) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Operand types '" + left->name + "' and '" + right->name + "' differ in signedness",
                        loc,
                        ULANG_SYNT_WARN_TYPES_SIGN_DIFF
                    );
                }

                if(left->size != right->size) {
                    throw CompilerSyntaxException(
                        CompilerSyntaxException::Severity::Warning,
                        "Operand types '" + left->name + "' and '" + right->name + "' differ in sizes",
                        loc,
                        ULANG_SYNT_WARN_TYPES_SIZE_DIFF
                    );
                }

                return left;
            }
        }

        return nullptr;
    }

    const DataType* determineBinopType(const DataType* left, const DataType* right) {
        if(left->size > right->size) return left;
        if(right->size > left->size) return right;

        if((left->flags & SIGN) != (right->flags & SIGN)) {
            return (left->flags & SIGN) ? left : right;
        }
        
        return left;
    }
}