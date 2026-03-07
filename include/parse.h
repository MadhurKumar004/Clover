#ifndef PARSE_H
#define PARSE_H

#include <cstddef>
#include <vector>
#include "tok.h"
#include "ast.h"

namespace Parse {
    class Parse {
        std::vector<Tok::Token> tokens;
        std::size_t index = 0;
    public:
        Parse(std::vector<Tok::Token> tokens);
        AST::Program parse();
    public:
        Tok::Token* peek();
        Tok::Token* previous();
        Tok::Token* advance();

        bool match(Tok::TokenKind kind);
        bool check(Tok::TokenKind kind);

        Tok::Token& consume(Tok::TokenKind kind, const char* msg);

        bool is_at_end();

        //Top level
        AST::Program parse_program();
        AST::TopLevel parse_top_level();
        AST::Function parse_function();
        AST::StructDecl parse_struct();
        AST::ConstDecl parse_const();
        AST::ExternDecl parse_extern();

        //Statements
        AST::Statement parse_statement();
        AST::Block parse_block();

        AST::VarDecl parse_var_decl();
        AST::IfStmt parse_if();
        AST::LoopStmt parse_loop();
        AST::DoStmt parse_do();
        AST::ReturnStmt parse_return();

        //Expressions
        AST::Expression parse_expression();
        AST::Expression parse_assignment();
        AST::Expression parse_or();
        AST::Expression parse_and();
        AST::Expression parse_equality();
        AST::Expression parse_comparison();
        AST::Expression parse_bitwise_or();
        AST::Expression parse_bitwise_xor();
        AST::Expression parse_bitwise_and();
        AST::Expression parse_shift();
        AST::Expression parse_additive();
        AST::Expression parse_multiplicative();
        AST::Expression parse_cast();
        AST::Expression parse_prefix();
        AST::Expression parse_postfix();
        AST::Expression parse_primary();

        //Literals
        AST::Expression parse_number_literal();
        AST::Expression parse_string_literal();
        AST::Expression parse_char_literal();
        AST::Expression parse_bool_literal();

        //Type
        AST::Type parse_type();
    };
};

#endif // PARSE_H
