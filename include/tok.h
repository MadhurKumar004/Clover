#ifndef TOK_H
#define TOK_H

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>

namespace Tok {

    enum class TokenKind : uint32_t {
        //Special
        Eof, UNKNOWN,

        //Literal
        LITINT, LITFLOAT, LITSTRING, LITCHAR,

        //Identifier
        IDENT,

        //keyword
        EXTERN, RET, CONST, IF, ELIF, ELSE, LOOP, DO, STRUCT, AS, UNSAFE, TRUE, FALSE, BREAK, CONTINUE,

        //Delimeter
        COMMA, SEMICOLON, LPAREN /* () */ ,RPAREN, LBRACE /* {} */ , RBRACE, LBRACKET /* [] */, RBRACKET,

        //operator
        ASSIGN /* = */, PLUS_ASSIGN /* += */, MINUS_ASSIGN, STAR_ASSING, SLASH_ASSIGN, PERCENT_ASSIGN,
        AMP_ASSIGN, PIPE_ASSIGN, CARET_ASSIGN/* ^= */, SHL_ASSIGN, SHR_ASSIGN, NOT, DOT, PLUS, MINUS,
        MULTIPLY, DIVIDE, MODULO, AND, OR, EQUAL /* == */, LESSER, GREATER, LESSER_EQUAL, GREATER_EQUAL,
        NOT_EQUAL, COLON, TILDE, BITWISEAND, BITWISEOR, BITWISEXOR, LSHIFT, RSHIFT,

        //Datatype
        U8, U16, U32, U64, U128,
        I1, I8, I16, I32, I64, I128,
        F16, F32, F64, F128,
        STRING, CHAR, NONE,
    };

    struct Token {
        TokenKind kind;
        std::string_view val; // Zero copy-val
        unsigned int line;
        unsigned int col;
    };

    // small, constexpr keyword table and simple lookup
    static constexpr std::pair<std::string_view, TokenKind> KEYWORDS[] = {

        // control flow
        {"if", TokenKind::IF},
        {"elif", TokenKind::ELIF},
        {"else", TokenKind::ELSE},
        {"loop", TokenKind::LOOP},
        {"do", TokenKind::DO},
        {"break", TokenKind::BREAK},
        {"continue", TokenKind::CONTINUE},

        // declarations
        {"extern", TokenKind::EXTERN},
        {"struct", TokenKind::STRUCT},
        {"const", TokenKind::CONST},
        {"ret", TokenKind::RET},
        {"as", TokenKind::AS},
        {"unsafe", TokenKind::UNSAFE},

        // literals
        {"true", TokenKind::TRUE},
        {"false", TokenKind::FALSE},

        // datatypes
        {"u8", TokenKind::U8},
        {"u16", TokenKind::U16},
        {"u32", TokenKind::U32},
        {"u64", TokenKind::U64},
        {"u128", TokenKind::U128},

        {"i1", TokenKind::I1},
        {"i8", TokenKind::I8},
        {"i16", TokenKind::I16},
        {"i32", TokenKind::I32},
        {"i64", TokenKind::I64},
        {"i128", TokenKind::I128},

        {"f16", TokenKind::F16},
        {"f32", TokenKind::F32},
        {"f64", TokenKind::F64},
        {"f128", TokenKind::F128},

        {"string", TokenKind::STRING},
        {"char", TokenKind::CHAR}
    };

    inline std::optional<TokenKind> keyword_kind(std::string_view s){
        for(auto &p : KEYWORDS) if (p.first == s) return p.second;
        return  std::nullopt;
    }

    inline TokenKind ident_or_keyword(std::string_view s){
        if(auto k = keyword_kind(s)) return *k;
        return TokenKind::IDENT;
    }

    inline std::optional<TokenKind> delimiter_kind(char c){
        switch(c){
            case ',' : return TokenKind::COMMA;
            case ';' : return TokenKind::SEMICOLON;
            case '(' : return TokenKind::LPAREN;
            case ')' : return TokenKind::RPAREN;
            case '{' : return TokenKind::LBRACE;
            case '}' : return TokenKind::RBRACE;
            case '[' : return TokenKind::LBRACKET;
            case ']' : return TokenKind::RBRACKET;
            default: return std::nullopt;
        }
    }

};

#endif // TOK_H
