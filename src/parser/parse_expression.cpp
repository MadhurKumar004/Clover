#include "parse.h"
#include <stdexcept>

namespace Parse {
    //entry
    AST::Expression Parse::parse_expression(){
        return parse_assignment();
    }
    //assignment
    AST::Expression Parse::parse_assignment(){
        auto left = parse_or();
        if(match(Tok::TokenKind::ASSIGN)){
            auto value = parse_assignment();
            return AST::Expression{
                AST::Expression::Assign{
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(value))
                }
            };
        }
        return left;
    }

    // ||
    AST::Expression Parse::parse_or(){
        auto left = parse_and();
        while(match(Tok::TokenKind::OR)){
            auto right = parse_and();
            left = AST::Expression{
                AST::Expression::Binary{
                    AST::BinaryOp::Or,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }

        return left;
    }

    // &&
    AST::Expression Parse::parse_and(){
        auto left = parse_equality();
        while(match(Tok::TokenKind::AND)){
            auto right = parse_equality();
            left = AST::Expression{
                AST::Expression::Binary{
                    AST::BinaryOp::And,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }

        return left;
    }

    // == !=
    AST::Expression Parse::parse_equality(){
        auto left = parse_comparison();
        while(match(Tok::TokenKind::EQUAL)||
                match(Tok::TokenKind::NOT_EQUAL)){
            Tok::TokenKind op = previous()->kind;
            auto right = parse_comparison();
            AST::BinaryOp bop =
                (op == Tok::TokenKind::EQUAL)
                ? AST::BinaryOp::Eq
                : AST::BinaryOp::Ne;

            left = AST::Expression{
                AST::Expression::Binary{
                    bop,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }

        return left;
    }

    // < > <= >=
    AST::Expression Parse::parse_comparison(){
        auto left = parse_bitwise_or();
        while(match(Tok::TokenKind::LESSER) ||
                match(Tok::TokenKind::GREATER) ||
                match(Tok::TokenKind::LESSER_EQUAL) ||
                match(Tok::TokenKind::GREATER_EQUAL)){
            Tok::TokenKind op = previous()->kind;
            auto right = parse_bitwise_or();
            AST::BinaryOp bop;
            switch(op){
                case Tok::TokenKind::LESSER: bop = AST::BinaryOp::Lt; break;
                case Tok::TokenKind::GREATER: bop = AST::BinaryOp::Gt; break;
                case Tok::TokenKind::LESSER_EQUAL: bop = AST::BinaryOp::Le; break;
                case Tok::TokenKind::GREATER_EQUAL: bop = AST::BinaryOp::Ge; break;
                default: throw std::runtime_error("invalid comparison op");
            }

            left = AST::Expression{
                AST::Expression::Binary{
                    bop,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }

        return left;
    }

    // |
    AST::Expression Parse::parse_bitwise_or(){
        auto left = parse_bitwise_xor();
        while(match(Tok::TokenKind::BITWISEOR)){
            auto right = parse_bitwise_xor();
            left = AST::Expression{
                AST::Expression::Binary{
                    AST::BinaryOp::BitOr,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }

    // ^
    AST::Expression Parse::parse_bitwise_xor(){
        auto left = parse_bitwise_and();
        while(match(Tok::TokenKind::BITWISEXOR)){
            auto right = parse_bitwise_and();
            left = AST::Expression{
                AST::Expression::Binary{
                    AST::BinaryOp::BitXor,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }


    // &
    AST::Expression Parse::parse_bitwise_and(){
        auto left = parse_shift();
        while(match(Tok::TokenKind::BITWISEAND)){
            auto right = parse_shift();
            left = AST::Expression{
                AST::Expression::Binary{
                    AST::BinaryOp::BitAnd,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }

    // << >>
    AST::Expression Parse::parse_shift(){
        auto left = parse_additive();
        while(match(Tok::TokenKind::LSHIFT) ||
            match(Tok::TokenKind::RSHIFT)){
            Tok::TokenKind op = previous()->kind;
            auto right = parse_additive();
            AST::BinaryOp bop =
                (op == Tok::TokenKind::LSHIFT)
                ? AST::BinaryOp::Shl
                : AST::BinaryOp::Shr;
            left = AST::Expression{
                AST::Expression::Binary{
                    bop,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }

    // + -
    AST::Expression Parse::parse_additive(){
        auto left = parse_multiplicative();
        while(match(Tok::TokenKind::PLUS) ||
            match(Tok::TokenKind::MINUS)){
            Tok::TokenKind op = previous()->kind;
            auto right = parse_multiplicative();
            AST::BinaryOp bop =
                (op == Tok::TokenKind::PLUS)
                ? AST::BinaryOp::Add
                : AST::BinaryOp::Sub;
            left = AST::Expression{
                AST::Expression::Binary{
                    bop,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }

    // * / %
    AST::Expression Parse::parse_multiplicative(){
        auto left = parse_cast();
        while(match(Tok::TokenKind::MULTIPLY) ||
            match(Tok::TokenKind::DIVIDE) ||
            match(Tok::TokenKind::MODULO)){
            Tok::TokenKind op = previous()->kind;
            auto right = parse_cast();
            AST::BinaryOp bop;
            switch(op){
                case Tok::TokenKind::MULTIPLY: bop = AST::BinaryOp::Mul; break;
                case Tok::TokenKind::DIVIDE: bop = AST::BinaryOp::Div; break;
                case Tok::TokenKind::MODULO: bop = AST::BinaryOp::Mod; break;
                default: throw std::runtime_error("invalid op");
            }
            left = AST::Expression{
                AST::Expression::Binary{
                    bop,
                    std::make_unique<AST::Expression>(std::move(left)),
                    std::make_unique<AST::Expression>(std::move(right))
                }
            };
        }
        return left;
    }

    AST::Expression Parse::parse_cast() {
        auto expr = parse_prefix();
        if(match(Tok::TokenKind::AS)) {
            AST::Type target = parse_type();
            return AST::Expression{
                AST::Expression::Cast{
                    std::make_unique<AST::Expression>(std::move(expr)),
                    std::make_unique<AST::Type>(std::move(target))
                }
            };
        }
        return expr;
    }

    AST::Expression Parse::parse_prefix() {
        if(match(Tok::TokenKind::NOT)) {
            return AST::Expression{
                AST::Expression::Unary{
                    AST::UnaryOp::Not,
                    std::make_unique<AST::Expression>(parse_prefix())
                }
            };
        }
        if(match(Tok::TokenKind::MINUS)) {
            return AST::Expression{
                AST::Expression::Unary{
                    AST::UnaryOp::Negate,
                    std::make_unique<AST::Expression>(parse_prefix())
                }
            };
        }
        if(match(Tok::TokenKind::TILDE)) {
            return AST::Expression{
                AST::Expression::Unary{
                    AST::UnaryOp::BitNot,
                    std::make_unique<AST::Expression>(parse_prefix())
                }
            };
        }
        if(match(Tok::TokenKind::MULTIPLY)) {
            return AST::Expression{
                AST::Expression::Unary{
                    AST::UnaryOp::Deref,
                    std::make_unique<AST::Expression>(parse_prefix())
                }
            };
        }
        if(match(Tok::TokenKind::BITWISEAND)) {
            return AST::Expression{
                AST::Expression::Unary{
                    AST::UnaryOp::Addrof,
                    std::make_unique<AST::Expression>(parse_prefix())
                }
            };
        }
        return parse_postfix();
    }

    AST::Expression Parse::parse_postfix() {
        auto expr = parse_primary();
        while(true) {
            // function call
            if(match(Tok::TokenKind::LPAREN)) {
                std::vector<AST::ExprPtr> args;
                if(!check(Tok::TokenKind::RPAREN)) {
                    do {
                        args.push_back(
                            std::make_unique<AST::Expression>(parse_expression())
                        );
                    } while(match(Tok::TokenKind::COMMA));

                }
                consume(Tok::TokenKind::RPAREN, "expected ')'");
                expr = AST::Expression{
                    AST::Expression::Call{
                        std::make_unique<AST::Expression>(std::move(expr)),
                        std::move(args)
                    }
                };
                continue;
            }
            // array indexing
            if(match(Tok::TokenKind::LBRACKET)) {
                auto idx = parse_expression();
                consume(Tok::TokenKind::RBRACKET, "expected ']'");
                expr = AST::Expression{
                    AST::Expression::Index{
                        std::make_unique<AST::Expression>(std::move(expr)),
                        std::make_unique<AST::Expression>(std::move(idx))
                    }
                };
                continue;
            }
            // field access / pointer field access
            if(match(Tok::TokenKind::DOT)) {
                if(match(Tok::TokenKind::MULTIPLY)) {
                    // .* -> PtrFieldAccess
                    Tok::Token& name = consume(
                        Tok::TokenKind::IDENT,
                        "expected field name"
                    );
                    expr = AST::Expression{
                        AST::Expression::PtrFieldAccess{
                            std::make_unique<AST::Expression>(std::move(expr)),
                            std::string(name.val)
                        }
                    };
                    continue;
                }
                // . -> FieldAccess
                Tok::Token& name = consume(
                    Tok::TokenKind::IDENT,
                    "expected field name"
                );
                expr = AST::Expression{
                    AST::Expression::FieldAccess{
                        std::make_unique<AST::Expression>(std::move(expr)),
                        std::string(name.val)
                    }
                };
                continue;
            }
            break;
        }
        return expr;
    }

    AST::Expression Parse::parse_primary(){
        // identifier
        if(match(Tok::TokenKind::IDENT)){
            return AST::Expression{
                AST::Expression::Identifier{
                    std::string(previous()->val)
                }
            };
        }
        // integer literal
        if(match(Tok::TokenKind::LITINT)){
            return parse_number_literal();
        }
        // float literal
        if(match(Tok::TokenKind::LITFLOAT)){
            return parse_number_literal();
        }
        // char literal
        if(match(Tok::TokenKind::LITCHAR)){
            return parse_char_literal();
        }
        // string literal
        if(match(Tok::TokenKind::LITSTRING)){
            return parse_string_literal();
        }
        // boolean true
        if(match(Tok::TokenKind::TRUE)){
            return parse_bool_literal();
        }
        // boolean false
        if(match(Tok::TokenKind::FALSE)){
            return parse_bool_literal();
        }
        // grouped expression
        if(match(Tok::TokenKind::LPAREN)){
            auto expr = parse_expression();
            consume(Tok::TokenKind::RPAREN, "expected ')'");
            return AST::Expression{
                AST::Expression::Grouped{
                    std::make_unique<AST::Expression>(std::move(expr))
                }
            };
        }
        throw std::runtime_error("invalid expression");
    }

    AST::Expression Parse::parse_number_literal(){
        auto tok = previous();
        AST::NumberLiteral lit;
        lit.raw = std::string(tok->val);
        return AST::Expression{
            AST::Expression::Number{std::move(lit)}
        };
    }

    AST::Expression Parse::parse_char_literal(){
        auto tok = previous();
        char value = 0;
        if(tok->val.size() >= 1)
            value = tok->val[0];

        return AST::Expression{
            AST::Expression::Char{
                AST::CharLiteral{value}
            }
        };
    }

    AST::Expression Parse::parse_string_literal(){
        auto tok = previous();
        return AST::Expression{
            AST::Expression::String{
                AST::StringLiteral{std::string(tok->val)}
            }
        };
    }

    AST::Expression Parse::parse_bool_literal(){
        auto tok = previous();
        bool value = (tok->kind == Tok::TokenKind::TRUE);
        return AST::Expression{
            AST::Expression::Bool{
                AST::BoolLiteral{value}
            }
        };
    }
}
