#ifndef LEX_H
#define LEX_H

#include <cstdint>
#include <vector>
#include <optional>
#include <string_view>
#include "util.h"
#include "tok.h"

namespace Lex {
    class Lexer {
    private:
        std::string_view content;
        std::size_t index;
        std::size_t prev_index;
        std::size_t line;
        std::size_t column;
    public :
        Lexer(std::string_view src) :
            content(src),
            index(0),
            prev_index(0),
            line(0),
            column(0) {}

        std::optional<char> next_byte();
        std::optional<char> peek_byte();

        char current_byte() const;
        char previous_byte();

        std::vector<Tok::Token> tokenize();
    private :
        std::optional<Tok::Token> tokenize_symbol(char c);
        std::optional<Tok::Token> tokenize_identifier();
        std::optional<Tok::Token> tokenize_number();
        std::optional<Tok::Token> tokenize_string();
        std::optional<Tok::Token> skip_comment();
        std::optional<Tok::TokenKind> check_multi_char_type();
        std::optional<Tok::Token> lexer_error(char c, const std::string& msg, size_t line, size_t column);
    };

};

#endif // LEX_H
