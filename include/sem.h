#ifndef SEM_H
#define SEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <iostream>

#include "ast.h"

namespace Sem {

// ── Diagnostic ───────────────────────────────────────────
struct Diagnostic {
    enum class Severity { Error, Warning };
    Severity severity;
    std::string message;
};

// ── SemType ──────────────────────────────────────────────
// A copyable, canonical type representation for semantic analysis.
// AST::Type uses unique_ptr (non-copyable), so we need our own.
struct SemType {
    enum Kind { Builtin, UserDef, Ptr, ConstPtr, None } kind = None;
    AST::BuiltinType builtin{};       // valid when kind == Builtin
    std::string user_name;             // valid when kind == UserDef
    std::shared_ptr<SemType> inner;    // valid when kind == Ptr / ConstPtr
};

using SemTypePtr = std::shared_ptr<SemType>;

// helpers to build SemTypes
inline SemTypePtr make_builtin(AST::BuiltinType b) {
    auto t = std::make_shared<SemType>();
    t->kind = SemType::Builtin; t->builtin = b;
    return t;
}
inline SemTypePtr make_user(const std::string &name) {
    auto t = std::make_shared<SemType>();
    t->kind = SemType::UserDef; t->user_name = name;
    return t;
}
inline SemTypePtr make_ptr(SemTypePtr inner) {
    auto t = std::make_shared<SemType>();
    t->kind = SemType::Ptr; t->inner = std::move(inner);
    return t;
}
inline SemTypePtr make_const_ptr(SemTypePtr inner) {
    auto t = std::make_shared<SemType>();
    t->kind = SemType::ConstPtr; t->inner = std::move(inner);
    return t;
}
inline SemTypePtr make_none() {
    return std::make_shared<SemType>(); // kind == None
}

bool types_equal(const SemTypePtr &a, const SemTypePtr &b);
bool is_integer(const SemTypePtr &t);
bool is_float(const SemTypePtr &t);
bool is_numeric(const SemTypePtr &t);
bool is_bool(const SemTypePtr &t);

// Convert AST::Type → SemType
SemTypePtr from_ast_type(const AST::Type &t);

// ── Symbol ───────────────────────────────────────────────
struct Symbol {
    enum class Kind { Variable, Param, Function, Struct, Const, Extern } kind;
    SemTypePtr type;                  // the type of this symbol
    bool is_const = false;

    // extra info for functions/externs
    std::vector<SemTypePtr> param_types;
    SemTypePtr return_type;

    // extra info for structs
    std::vector<std::pair<std::string, SemTypePtr>> fields;
};

// ── ScopeStack ───────────────────────────────────────────
class ScopeStack {
    std::vector<std::unordered_map<std::string, Symbol>> stack;
public:
    void push();
    void pop();
    bool declare(const std::string &name, Symbol sym);
    Symbol* lookup(const std::string &name);
};

// ── SemanticAnalyzer ─────────────────────────────────────
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(AST::Program &program);
    void analyze();

    bool has_errors() const;
    const std::vector<Diagnostic>& get_diagnostics() const;
    void print_errors() const;

    // public so passes (in separate .cpp files) can access shared state
    AST::Program &program;
    ScopeStack scopes;
    std::vector<Diagnostic> diags;

    // state used by passes
    SemTypePtr current_return_type;
    int loop_depth = 0;
    bool in_unsafe = false;

    // helpers
    void error(const std::string &msg);
    void warning(const std::string &msg);

    // ── Passes (each in its own .cpp under src/check/) ──
    void declaration_pass();
    void resolve_pass();       // future
    void typecheck_pass();     // future
    void controlflow_pass();   // future

    // ── Expression / statement checkers (used by typecheck_pass) ──
    SemTypePtr check_expr(const AST::Expression &e);
    void check_stmt(const AST::Statement &s);
    void check_block(const AST::Block &b);
    void check_function(const AST::Function &f);
};

} // namespace Sem

#endif // SEM_H
