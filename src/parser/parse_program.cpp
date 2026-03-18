#include "parse.h"

namespace Parse {
    AST::Program Parse::parse_program(){
        AST::Program prog;
        while(!is_at_end()) {
            prog.items.push_back(parse_top_level());
        }

        return prog;
    }

    AST::TopLevel Parse::parse_top_level(){
        if(match(Tok::TokenKind::STRUCT))
            return parse_struct();
        if(match(Tok::TokenKind::CONST))
            return parse_const();
        if(match(Tok::TokenKind::EXTERN))
            return parse_extern();
        return parse_function();
    }

}
