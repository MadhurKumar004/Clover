#include "parse.h"
#include <stdexcept>

namespace Parse {
    static AST::BuiltinType builtin_from_token(Tok::TokenKind k) {
        switch(k) {
            case Tok::TokenKind::I1:   return AST::BuiltinType::I1;
            case Tok::TokenKind::I8:   return AST::BuiltinType::I8;
            case Tok::TokenKind::I16:  return AST::BuiltinType::I16;
            case Tok::TokenKind::I32:  return AST::BuiltinType::I32;
            case Tok::TokenKind::I64:  return AST::BuiltinType::I64;
            case Tok::TokenKind::I128: return AST::BuiltinType::I128;
            case Tok::TokenKind::U8:   return AST::BuiltinType::U8;
            case Tok::TokenKind::U16:  return AST::BuiltinType::U16;
            case Tok::TokenKind::U32:  return AST::BuiltinType::U32;
            case Tok::TokenKind::U64:  return AST::BuiltinType::U64;
            case Tok::TokenKind::U128: return AST::BuiltinType::U128;
            case Tok::TokenKind::F16:  return AST::BuiltinType::F16;
            case Tok::TokenKind::F32:  return AST::BuiltinType::F32;
            case Tok::TokenKind::F64:  return AST::BuiltinType::F64;
            case Tok::TokenKind::F128: return AST::BuiltinType::F128;
            case Tok::TokenKind::CHAR: return AST::BuiltinType::Char;
            default: throw std::runtime_error("not a builtin type");
        }
    }

    AST::Type Parse::parse_type(){
        // Pointer: *type or *const type
        if(match(Tok::TokenKind::MULTIPLY)){
            if(match(Tok::TokenKind::CONST)){
                AST::Type inner = parse_type();
                return AST::Type{
                    AST::Type::ConstPtr{
                        std::make_unique<AST::Type>(std::move(inner))
                    }
                };
            }
            AST::Type inner = parse_type();
            return AST::Type{
                AST::Type::Pointer{
                    std::make_unique<AST::Type>(std::move(inner))
                }
            };
        }

        // Builtin types
        Tok::Token* tok = peek();
        if(tok) {
            switch(tok->kind) {
                case Tok::TokenKind::I1:
                case Tok::TokenKind::I8:
                case Tok::TokenKind::I16:
                case Tok::TokenKind::I32:
                case Tok::TokenKind::I64:
                case Tok::TokenKind::I128:
                case Tok::TokenKind::U8:
                case Tok::TokenKind::U16:
                case Tok::TokenKind::U32:
                case Tok::TokenKind::U64:
                case Tok::TokenKind::U128:
                case Tok::TokenKind::F16:
                case Tok::TokenKind::F32:
                case Tok::TokenKind::F64:
                case Tok::TokenKind::F128:
                case Tok::TokenKind::CHAR: {
                    Tok::TokenKind k = tok->kind;
                    advance();
                    return AST::Type{
                        AST::Type::Builtin{builtin_from_token(k)}
                    };
                }
                default:
                    break;
            }
        }

        // User-defined type
        if(match(Tok::TokenKind::IDENT)){
            return AST::Type{
                AST::Type::UserDef{
                    std::string(previous()->val)
                }
            };
        }

        throw std::runtime_error("expected type");
    }
}
