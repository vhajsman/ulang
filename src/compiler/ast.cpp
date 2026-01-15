#include "compiler.hpp"

namespace ULang {
    ASTNode::ASTNode(int64_t val)
    : type(ASTNodeType::NUMBER), val(val) {}

    ASTNode::ASTNode(const std::string& varname)
    : type(ASTNodeType::VARIABLE), name(varname) {}

    ASTNode::ASTNode(ASTNodeType t)
    : type(t) {}
};