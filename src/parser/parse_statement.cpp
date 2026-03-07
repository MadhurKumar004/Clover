#include "parse.h"
#include <memory>
#include <string>
#include <utility>

namespace Parse {
    AST::Statement Parse::parse_statement(){
        if(match(Tok::TokenKind::IF))
            return AST::Statement{parse_if()};
        if(match(Tok::TokenKind::LOOP))
            return AST::Statement{parse_loop()};
        if(match(Tok::TokenKind::DO))
            return AST::Statement{parse_do()};
        if(match(Tok::TokenKind::RET))
            return AST::Statement{parse_return()};
        if(match(Tok::TokenKind::BREAK)){
            consume(Tok::TokenKind::SEMICOLON, "expected ';'");
            return AST::Statement{AST::BreakStmt{}};
        }
        if(match(Tok::TokenKind::CONTINUE)){
            consume(Tok::TokenKind::SEMICOLON, "expected ';'");
            return AST::Statement{AST::ContinueStmt{}};
        }
        if(match(Tok::TokenKind::UNSAFE)){
            return AST::Statement{AST::UnsafeStmt{
                std::make_unique<AST::Block>(parse_block())
            }};
        }
        // const var decl
        if(check(Tok::TokenKind::CONST)){
            return AST::Statement{parse_var_decl()};
        }
        // var decl: ident ':'
        if(check(Tok::TokenKind::IDENT) &&
           index + 1 < tokens.size() &&
           tokens[index + 1].kind == Tok::TokenKind::COLON){
            return AST::Statement{parse_var_decl()};
        }

        auto expr = parse_expression();
        consume(Tok::TokenKind::SEMICOLON, "expected ';'");
        return AST::Statement{AST::ExprStmt{
            std::make_unique<AST::Expression>(std::move(expr))
        }};
    }

    AST::IfStmt Parse::parse_if(){
        AST::IfStmt node;
        auto cond = parse_expression();
        auto body = parse_block();
        node.branches.push_back({
            std::make_unique<AST::Expression>(std::move(cond)),
            std::make_unique<AST::Block>(std::move(body))
        });

        while(match(Tok::TokenKind::ELIF)){
            auto elif_cond = parse_expression();
            auto elif_body = parse_block();
            node.branches.push_back({
                std::make_unique<AST::Expression>(std::move(elif_cond)),
                std::make_unique<AST::Block>(std::move(elif_body))
            });
        }

        if(match(Tok::TokenKind::ELSE))
            node.else_body = std::make_unique<AST::Block>(parse_block());
        return node;
    }

    AST::LoopStmt Parse::parse_loop() {
        AST::Block body = parse_block();
        return {std::make_unique<AST::Block>(std::move(body))};
    }

    AST::DoStmt Parse::parse_do(){
        AST::Expression cond = parse_expression();
        AST::Block body = parse_block();
        return {
            std::make_unique<AST::Expression>(std::move(cond)),
            std::make_unique<AST::Block>(std::move(body))
        };
    }

    AST::ReturnStmt Parse::parse_return(){
        AST::ExprPtr value;
        if(!check(Tok::TokenKind::SEMICOLON)){
            value = std::make_unique<AST::Expression>(parse_expression());
        }
        consume(Tok::TokenKind::SEMICOLON, "expected ';'");
        return {std::move(value)};
    }

    AST::VarDecl Parse::parse_var_decl(){
        bool is_const = false;
        if(match(Tok::TokenKind::CONST))
            is_const = true;
        auto& name = consume(Tok::TokenKind::IDENT, "expected name");
        consume(Tok::TokenKind::COLON, "expected ':'");
        AST::Type type = parse_type();
        AST::ExprPtr init;
        if(match(Tok::TokenKind::ASSIGN))
            init = std::make_unique<AST::Expression>(parse_expression());
        consume(Tok::TokenKind::SEMICOLON, "expected ';'");
        return {
            is_const,
            std::string(name.val),
            std::move(type),
            std::move(init)
        };
    }

    AST::ConstDecl Parse::parse_const(){
        auto& name = consume(Tok::TokenKind::IDENT, "expected identifier");
        consume(Tok::TokenKind::COLON, "expected ':'");
        AST::Type type = parse_type();
        consume(Tok::TokenKind::ASSIGN, "expected '='");
        AST::Expression expr = parse_expression();
        consume(Tok::TokenKind::SEMICOLON, "expected ';'");
        return {
            std::string(name.val),
            std::move(type),
            std::make_unique<AST::Expression>(std::move(expr))
        };
    }
}
