#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <variant>

namespace AST {
struct Expression;
struct Statement;
struct Type;

using ExprPtr = std::unique_ptr<Expression>;
using StmtPtr = std::unique_ptr<Statement>;
using TypePtr = std::unique_ptr<Type>;

//Types ----------------------------------
enum class BuiltinType {
    I1, I8, I16, I32, I64, I128,
    U8, U16, U32, U64, U128,
    F16, F32, F64, F128,
    Char
};

struct Type {
    struct Builtin { BuiltinType type; };
    struct UserDef { std::string name; };
    struct Pointer { TypePtr inner; }; // *T
    struct ConstPtr {TypePtr inner; };  // *const T

    std::variant< Builtin, UserDef, Pointer, ConstPtr> value;
};

//Literal ---------------------------------------
struct NumberLiteral {
    std::string raw; // "42", "0xFF", "0b1010101", "3.14"
    std::optional<std::string> suffix; // "i32", "u8"
};

struct CharLiteral { char value; };
struct BoolLiteral { bool value; };
struct StringLiteral {std::string value; };

//Expressions --------------------------------------------
enum class UnaryOp { Negate, Not, BitNot, Deref, Addrof };
enum class BinaryOp {
    Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Gt, Le, Ge, And, Or, BitAnd, BitOr, BitXor,
    Shl, Shr
};

struct Expression {
    //PrimaryExpr
    struct Identifier { std::string name; };
    struct Number { NumberLiteral lit; };
    struct Char { CharLiteral lit; };
    struct Bool { BoolLiteral lit; };
    struct String { StringLiteral lit; };
    struct Grouped { ExprPtr inner; }; // '( expr )'

    //PostfixExpr
    struct Call {
        ExprPtr callee;
        std::vector<ExprPtr> args;
    };

    struct Index {
        ExprPtr array;
        ExprPtr idx;
    };

    struct FieldAccess {
        ExprPtr object;
        std::string field;
    };

    struct PtrFieldAccess {
        ExprPtr object;
        std::string field;
    };

    //prefixExpr
    struct Unary {
        UnaryOp op;
        ExprPtr operand;
    };

    //castExpr
    struct Cast {
        ExprPtr operand;
        TypePtr target;
    };

    struct Binary {
        BinaryOp op;
        ExprPtr left;
        ExprPtr right;
    };

    struct Assign {
        ExprPtr target;
        ExprPtr value;
    };

    std::variant<Identifier, Number, Char, Bool, String, Grouped,
        Call, Index, FieldAccess, PtrFieldAccess, Unary, Cast, Binary, Assign> value;
};

//Statement ---------------------------------------------------------
struct Block;
using BlockPtr = std::unique_ptr<Block>;

struct VarDecl {
    bool is_const = false;
    std::string name;
    Type type;
    ExprPtr init;
};

struct IfStmt {
    struct Branch{
        ExprPtr condition;
        BlockPtr body;
    };
    std::vector<Branch> branches;
    std::optional<BlockPtr> else_body;
};

struct DoStmt {
    ExprPtr condition;
    BlockPtr body;
};

struct LoopStmt {
    BlockPtr body;
};

struct ReturnStmt{
    ExprPtr value;
};

struct BreakStmt {};
struct ContinueStmt {};
struct UnsafeStmt { BlockPtr body; };
struct ExprStmt { ExprPtr expr; };

struct Statement {
    std::variant<
        VarDecl,
        IfStmt,
        DoStmt,
        LoopStmt,
        ReturnStmt,
        BreakStmt,
        ContinueStmt,
        UnsafeStmt,
        ExprStmt
    > value;
};

// Block --------------------------------------
struct Block {
    std::vector<StmtPtr> body;
};

//Top level declaration
struct Param {
    std::string name;
    Type type;
};

struct Function {
    std::optional<Type> return_type;
    std::string name;
    std::vector<Param> params;
    BlockPtr body;
};

struct FieldDecl{
    std::string name;
    Type type;
};

struct StructDecl {
    std::string name;
    std::vector<FieldDecl> fields;
};

struct ConstDecl {
    std::string name;
    Type type;
    ExprPtr value;
};

struct ExternDecl {
    std::optional<Type> return_type;
    std::string name;
    std::vector<Param> params;
};

// Root program
using TopLevel = std::variant<Function, StructDecl, ConstDecl, ExternDecl>;
struct Program {
    std::vector<TopLevel> items;
};
};      // namespace AST end;

#endif // AST_H
