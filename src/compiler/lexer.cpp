#include "compiler.hpp"
#include "types.hpp"

#include <cctype>
#include <stdexcept>

namespace ULang {
    Lexer::Lexer(const std::string& input)
    : src(input) {}

    char Lexer::peek() const {
        return this->pos < this->src.size() ? this->src[this->pos] : '\0';
    }

    char Lexer::get() {
        return this->pos < this->src.size() ? this->src[this->pos++] : '\0';
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;

        while(this->pos < this->src.size()) {
            char c = this->peek();

            if(std::isspace(c)) {
                THROW_AWAY this->get();
                continue;
            }

            if(std::isdigit(c)) {
                std::string numstr;
                while(std::isdigit(this->peek()) || (numstr.size() > 0 && this->peek() == '_'))
                    numstr += this->get();

                tokens.push_back({TokenType::Number, numstr});
                continue;
            }

            if(std::isalpha(c)) {
                std::string id;
                while(std::isalnum(this->peek()) || this->peek() == '_') {
                    id += this->get();
                }

                try {
                    const DataType* type = resolveDataType(id);
                    tokens.push_back({TokenType::TypeKeyword, type->name});
                } catch(const std::exception& e) {
                    THROW_AWAY e;
                    tokens.push_back({TokenType::Identifier, id});
                };

                continue;
            }

            switch(c) {
                case '+': tokens.push_back({TokenType::Plus, std::string(1, this->get())}); break;
                case '-': tokens.push_back({TokenType::Minus, std::string(1, this->get())}); break;
                case '*': tokens.push_back({TokenType::Mul, std::string(1, this->get())}); break;
                case '/': tokens.push_back({TokenType::Div, std::string(1, this->get())}); break;
                case '=': tokens.push_back({TokenType::Assign, std::string(1, this->get())}); break;
                case ';': tokens.push_back({TokenType::Semicolon, std::string(1, this->get())}); break;
                
                default: 
                    throw std::runtime_error(std::string("Unknown character: ") + c);
            }
        }

        tokens.push_back({TokenType::EndOfFile, ""});
        return tokens;
    }
};