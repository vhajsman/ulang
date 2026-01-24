#include "compiler.hpp"
#include "types.hpp"

#include <cctype>
#include <cstddef>
#include <stdexcept>

namespace ULang {
    Lexer::Lexer(const std::string& input)
    : src(input), pos(0) {}

    char Lexer::peek() const {
        return this->pos < this->src.size() ? this->src[this->pos] : '\0';
    }

    char Lexer::get() {
        //return this->pos < this->src.size() ? this->src[this->pos++] : '\0';
        if(this->pos >= this->src.size())
            return '\0';

        char c = this->src[pos++];
        if(c == '\n') {
            line++; col = 1;
        } else col++;

        return c;
    }

    std::vector<Token> Lexer::tokenize(const std::string& filename) {
        std::vector<Token> tokens;
        
        this->pos = 0;
        this->line = 1;
        this->col = 1;

        while(this->pos < this->src.size()) {
            char c = this->peek();

            // whitespaces
            if(std::isspace(c)) {
                THROW_AWAY this->get();
                continue;
            }

            // numbers
            if(std::isdigit(c)) {
                std::string numstr;
                size_t tok_col = this->col;

                while(std::isdigit(this->peek()) || (numstr.size() > 0 && this->peek() == '_'))
                    numstr += this->get();

                tokens.push_back({TokenType::Number, numstr, {
                    nullptr,
                    filename,
                    this->line,
                    tok_col
                }});

                continue;
            }

            // identifiers, keywords
            if(std::isalpha(c)) {
                std::string id;
                size_t tok_col = this->col;

                while(std::isalnum(this->peek()) || this->peek() == '_') {
                    id += this->get();
                }

                try {
                    const DataType* type = resolveDataType(id);
                    tokens.push_back({TokenType::TypeKeyword, type->name, {
                        nullptr,
                        filename,
                        this->line,
                        tok_col
                    }});
                } catch(const std::exception& e) {
                    THROW_AWAY e;
                    tokens.push_back({TokenType::Identifier, id});
                };

                continue;
            }

            size_t tok_col = this->col;

            // operators
            switch(c) {
                case '+': tokens.push_back({TokenType::Plus,     std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '-': tokens.push_back({TokenType::Minus,    std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '*': tokens.push_back({TokenType::Mul,      std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '/': tokens.push_back({TokenType::Div,      std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '=': tokens.push_back({TokenType::Assign,   std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case ';': tokens.push_back({TokenType::Semicolon,std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                
                default: 
                    throw std::runtime_error(std::string("Unknown character: ") + c);
            }
        }

        // EOF
        tokens.push_back({TokenType::EndOfFile, "", {
            nullptr,
            filename,
            this->line,
            this->col
        }});
        
        return tokens;
    }
};