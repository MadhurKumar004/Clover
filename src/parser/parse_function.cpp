#include "parse.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace Parse{
AST::Function Parse::parse_function(){
    AST::Type ret = parse_type();
    Tok::Token& name_tok = consume(Tok::TokenKind::IDENT, "expected function name");
    std::string name(name_tok.val);
    consume(Tok::TokenKind::LPAREN, "expected '('");
    std::vector<AST::Param> params;
    if(!check(Tok::TokenKind::RPAREN)){
        do{
            Tok::Token& pname = consume(Tok::TokenKind::IDENT, "expected param");
            consume(Tok::TokenKind::COLON, "expected ':'");
            AST::Type ptype = parse_type();
            params.push_back(AST::Param{
                std::string(pname.val),
                std::move(ptype)
            });
        } while(match(Tok::TokenKind::COMMA));
    }

    consume(Tok::TokenKind::RPAREN, "expected ')'");
    AST::Block body = parse_block();

    return AST::Function{
        std::optional<AST::Type>(std::move(ret)),
        std::move(name),
        std::move(params),
        std::make_unique<AST::Block>(std::move(body))
    };
}
}
