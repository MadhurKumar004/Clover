#include "parse.h"
#include <string>
#include <utility>

namespace Parse {
    AST::StructDecl Parse::parse_struct(){
        Tok::Token& name = consume(Tok::TokenKind::IDENT, "expected struct name");
        consume(Tok::TokenKind::LBRACE, "expected '{'");
        std::vector<AST::FieldDecl> fields;
        while(!check(Tok::TokenKind::RBRACE) && !is_at_end()){
            Tok::Token& field_name = consume(Tok::TokenKind::IDENT, "expected field name");
            consume(Tok::TokenKind::COLON, "expected ':'");
            AST::Type type = parse_type();
            consume(Tok::TokenKind::SEMICOLON, "expected ';'");
            fields.push_back(AST::FieldDecl{
                std::string(field_name.val),
                std::move(type)
            });
        }

        consume(Tok::TokenKind::RBRACE, "expected '}'");

        return AST::StructDecl{
            std::string(name.val),
            std::move(fields)
        };
    }
}
