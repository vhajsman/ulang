#include "compiler.hpp"
#include "types.hpp"

#include <cctype>
#include <cstddef>
#include <iostream>
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
                std::string str;
                size_t tok_col = this->col;

                while(std::isalnum(this->peek()) || this->peek() == '_') {
                    str += this->get();
                }

                TokenType tt = TokenType::Identifier;

                if(str == "fn")             tt = TokenType::Function;
                else if(str == "return")    tt = TokenType::Return;
                else try {
                    const DataType* type = resolveDataType(str);
                    tt = TokenType::TypeKeyword;
                    str = type->name;
                } catch(const std::exception& e) {
                    THROW_AWAY e;
                    //TokenType tt = TokenType::Identifier;
                    //tokens.push_back({tt, str});
                };

                tokens.push_back({tt, str, {
                        nullptr,
                        filename,
                        this->line,
                        tok_col
                    }});

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

                case '(': tokens.push_back({TokenType::LParen,std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case ')': tokens.push_back({TokenType::RParen,std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '{': tokens.push_back({TokenType::LCurly,std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                case '}': tokens.push_back({TokenType::RCurly,std::string(1, this->get()), {nullptr, filename, this->line, tok_col}}); break;
                
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

        for(auto& tok: tokens) {
            std::cout << "Token: " << int(tok.type) << ", text: '" << tok.text << "'" << std::endl;
        }
        
        return tokens;
    }
};