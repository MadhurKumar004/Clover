#include "parse.h"
#include "ast.h"

namespace Parse{
    AST::Block Parse::parse_block(){
        consume(Tok::TokenKind::LBRACE, "expected '{'");

        AST::Block block;
        while(!check(Tok::TokenKind::RBRACE) && !is_at_end())
            block.body.push_back(
                std::make_unique<AST::Statement>(parse_statement())
            );
        consume(Tok::TokenKind::RBRACE, "expected '}'");

        return block;
    }
}
