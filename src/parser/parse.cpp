#include "parse.h"
#include <stdexcept>
#include <utility>

namespace Parse {
    Parse::Parse(std::vector<Tok::Token> tokens) :
        tokens(std::move(tokens)) {}

    AST::Program Parse::parse(){
        return parse_program();
    }

    Tok::Token* Parse::peek(){
        if(index < tokens.size())
            return &tokens[index];
        return nullptr;
    }

    Tok::Token* Parse::advance(){
        if(!is_at_end())
            index++;
        return previous();
    }

    Tok::Token* Parse::previous(){
        if(index == 0)
            return nullptr;
        return &tokens[index - 1];
    }

    bool Parse::check(Tok::TokenKind kind){
        if(is_at_end())
            return false;

        return peek()->kind == kind;
    }

    bool Parse::match(Tok::TokenKind kind){
        if(check(kind)){
            advance();
            return true;
        }
        return false;
    }

    Tok::Token& Parse::consume(Tok::TokenKind kind, const char* msg){
        if(check(kind))
            return *advance();

        throw std::runtime_error(msg);
    }

    bool Parse::is_at_end(){
        Tok::Token* t = peek();
        return t == nullptr || t->kind == Tok::TokenKind::Eof;
    }
}
