#include <cstdint>
#include <string>
#include <optional>
#include <cctype>

#include <vector>
#include "lex.h"
#include "tok.h"

namespace Lex {
    std::optional<char> Lexer::next_byte(){
        if(index < content.size())
            return content[index++];
        return std::nullopt;
    }
    std::optional<char> Lexer::peek_byte(){
        if(index < content.size())
            return content[index];
        return std::nullopt;
    }
    char Lexer::current_byte() const {
        return content[index-1];
    }
    char Lexer::previous_byte() {
            index--;
        return content[index-1];
    }
    std::vector<Tok::Token> Lexer::tokenize(){
        std::vector<Tok::Token> tokens;
        while(auto ch = next_byte()){
            char c = *ch;
            column += index - prev_index;
            prev_index = index;

            std::optional<Tok::Token> token;
            if(std::isdigit(c)){
                token = tokenize_number();
            }
            else if(std::isalnum(c) || c == '_'){
                token = tokenize_identifier();
            }
            else if(c == '"' || c == '\''){
                token = tokenize_string();
            }
            else if (std::isspace(c)) {
                if(c == '\n'){
                    line++;
                    column = 0;
                }
                token = std::nullopt;
            }
            else{
                token = tokenize_symbol(c);
            }

            if(token){
                tokens.push_back(*token);
            }
        }
        tokens.push_back(Tok::Token{Tok::TokenKind::Eof, "", line, column});
        return tokens;
    }
    std::optional<Tok::Token> Lexer::tokenize_symbol(char c) {

        Tok::TokenKind type;

        switch (c) {
            case '+': type = Tok::TokenKind::PLUS; break;
            case '*': type = Tok::TokenKind::MULTIPLY; break;
            case ',': type = Tok::TokenKind::COMMA; break;
            case ';': type = Tok::TokenKind::SEMICOLON; break;
            case '(': type = Tok::TokenKind::LPAREN; break;
            case ')': type = Tok::TokenKind::RPAREN; break;
            case '[': type = Tok::TokenKind::LBRACKET; break;
            case ']': type = Tok::TokenKind::RBRACKET; break;
            case '{': type = Tok::TokenKind::LBRACE; break;
            case '}': type = Tok::TokenKind::RBRACE; break;
            case '.': type = Tok::TokenKind::DOT; break;
            case '^': type = Tok::TokenKind::BITWISEXOR; break;
            case '&': type = Tok::TokenKind::BITWISEAND; break;
            case '|': type = Tok::TokenKind::BITWISEOR; break;
            case '/':
                return skip_comment();
            case '=':
            case '<':
            case '>':
            case '!':
            case '-':
            case ':': {
                auto t = check_multi_char_type();
                if (!t)
                    return std::nullopt;
                type = *t;
                break;
            }

            default:
                return lexer_error(c, "invalid character", line, column);
        }

        return Tok::Token{
            type,
            "",
            line,
            column
        };
    }
    std::optional<Tok::Token> Lexer::skip_comment(){
        if(auto c = peek_byte(); c && *c == '/'){
            next_byte(); // consume second '/'
            while(auto p = peek_byte()){
                if(*p == '\n')
                    break;
                next_byte();
            }
            return std::nullopt;
        }
        return Tok::Token{Tok::TokenKind::DIVIDE, "", line, column};
    }
    std::optional<Tok::TokenKind> Lexer::check_multi_char_type(){
        char first_char = current_byte();
        auto second = next_byte();

        if(!second)
            return std::nullopt;

        char second_char = *second;

        if(first_char == '=' && second_char == '=') return Tok::TokenKind::EQUAL;
        if(first_char == '!' && second_char == '=') return Tok::TokenKind::NOT_EQUAL;
        if(first_char == '<' && second_char == '=') return Tok::TokenKind::LESSER_EQUAL;
        if(first_char == '>' && second_char == '=') return Tok::TokenKind::GREATER_EQUAL;
        if(first_char == '>' && second_char == '>') return Tok::TokenKind::RSHIFT;
        if(first_char == '<' && second_char == '<') return Tok::TokenKind::LSHIFT;

        previous_byte();

        switch(first_char){
            case '!' : return Tok::TokenKind::NOT;
            case '=' : return Tok::TokenKind::ASSIGN;
            case '<' : return Tok::TokenKind::LESSER;
            case '>' : return Tok::TokenKind::GREATER;
            case '-' : return Tok::TokenKind::MINUS;
            case ':' : return Tok::TokenKind::COLON;
            default : lexer_error(first_char, "invalid token", line, column);
        }
    }
    std::optional<Tok::Token> Lexer::tokenize_identifier(){
        size_t size = 0;
        char c = current_byte();
        while(std::isalnum(c)|| c == '_'){
            size++;
            auto ch = next_byte();
            if(!ch)
                break;
            c = *ch;
        }

        //rewind because we read one char too far;
        previous_byte();
        std::string_view text = content.substr(index-size, size);
        Tok::TokenKind kind = Tok::ident_or_keyword(text);

        return Tok::Token{
            kind,
            text,
            line,
            column
        };
    }
    std::optional<Tok::Token> Lexer::tokenize_number(){
        size_t size = 0;
        bool is_float = false;
        char c = current_byte();
        while(std::isdigit(c) || c == '.'){
            if(c == '.')
                is_float = true;
            size++;
            auto ch = next_byte();
            if(!ch)
                break;
            c = *ch;
        }
        previous_byte();

        std::string_view result = content.substr(index-size, size);

        return Tok::Token{
            is_float ? Tok::TokenKind::LITFLOAT : Tok::TokenKind::LITINT, // change later if float
            result,
            line,
            column
        };
    }
    std::optional<Tok::Token> Lexer::tokenize_string(){
        const size_t start = index;
        size_t end = start;
        char quote = current_byte();
        auto ch = next_byte();
        if(!ch) return std::nullopt;
        char c = *ch;

        while(c != quote){
            end++;
            auto next = next_byte();
            if(!next) return std::nullopt;
            c = *next;
        }

        std::string_view result = content.substr(start, end-start);
        return Tok::Token{
            Tok::TokenKind::STRING,
            result,
            line,
            column
        };

    }
    [[noreturn]]
    std::optional<Tok::Token> Lexer::lexer_error(char c, const std::string& msg, size_t line, size_t column){
        std::cerr << "LexerError\n";
        std::cerr << "Error at line: " << line + 1 << ", column: " << column + 1 << "\n";
        std::cerr << "Error type: " << msg << " => '" << c << "' code: " << static_cast<int>(c) << "\n";
        std::exit(1);
};
