#include "sem.h"
#include <variant>

namespace Sem {

// ── SemType helpers ──────────────────────────────────────

bool types_equal(const SemTypePtr &a, const SemTypePtr &b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case SemType::Builtin:  return a->builtin == b->builtin;
        case SemType::UserDef:  return a->user_name == b->user_name;
        case SemType::Ptr:
        case SemType::ConstPtr: return types_equal(a->inner, b->inner);
        case SemType::None:     return true;
    }
    return false;
}

bool is_integer(const SemTypePtr &t) {
    if (!t || t->kind != SemType::Builtin) return false;
    switch (t->builtin) {
        case AST::BuiltinType::I1:  case AST::BuiltinType::I8:
        case AST::BuiltinType::I16: case AST::BuiltinType::I32:
        case AST::BuiltinType::I64: case AST::BuiltinType::I128:
        case AST::BuiltinType::U8:  case AST::BuiltinType::U16:
        case AST::BuiltinType::U32: case AST::BuiltinType::U64:
        case AST::BuiltinType::U128:
            return true;
        default: return false;
    }
}

bool is_float(const SemTypePtr &t) {
    if (!t || t->kind != SemType::Builtin) return false;
    switch (t->builtin) {
        case AST::BuiltinType::F16: case AST::BuiltinType::F32:
        case AST::BuiltinType::F64: case AST::BuiltinType::F128:
            return true;
        default: return false;
    }
}

bool is_numeric(const SemTypePtr &t) { return is_integer(t) || is_float(t); }

bool is_bool(const SemTypePtr &t) {
    return t && t->kind == SemType::Builtin && t->builtin == AST::BuiltinType::I1;
}

SemTypePtr from_ast_type(const AST::Type &t) {
    return std::visit([](auto const &v) -> SemTypePtr {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AST::Type::Builtin>) {
            return make_builtin(v.type);
        } else if constexpr (std::is_same_v<T, AST::Type::UserDef>) {
            return make_user(v.name);
        } else if constexpr (std::is_same_v<T, AST::Type::Pointer>) {
            return make_ptr(from_ast_type(*v.inner));
        } else if constexpr (std::is_same_v<T, AST::Type::ConstPtr>) {
            return make_const_ptr(from_ast_type(*v.inner));
        } else {
            return make_none();
        }
    }, t.value);
}

// ── ScopeStack ───────────────────────────────────────────

void ScopeStack::push() { stack.emplace_back(); }

void ScopeStack::pop() { stack.pop_back(); }

bool ScopeStack::declare(const std::string &name, Symbol sym) {
    auto &top = stack.back();
    if (top.find(name) != top.end()) return false; // duplicate
    top.emplace(name, std::move(sym));
    return true;
}

Symbol* ScopeStack::lookup(const std::string &name) {
    for (int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
        auto it = stack[i].find(name);
        if (it != stack[i].end()) return &it->second;
    }
    return nullptr;
}

// ── SemanticAnalyzer ─────────────────────────────────────

SemanticAnalyzer::SemanticAnalyzer(AST::Program &program)
    : program(program) {}

void SemanticAnalyzer::analyze() {
    // push global scope
    scopes.push();

    // Pass 1: collect top-level declarations
    declaration_pass();
    if (has_errors()) return;

    // Pass 2 (future): resolve types & names
    // resolve_pass();

    // Pass 3 (future): type-check expressions & statements
    // typecheck_pass();

    // Pass 4 (future): control-flow checks
    // controlflow_pass();

    // pop global scope
    scopes.pop();
}

bool SemanticAnalyzer::has_errors() const {
    for (auto &d : diags)
        if (d.severity == Diagnostic::Severity::Error) return true;
    return false;
}

const std::vector<Diagnostic>& SemanticAnalyzer::get_diagnostics() const {
    return diags;
}

void SemanticAnalyzer::print_errors() const {
    for (auto &d : diags) {
        const char *tag = (d.severity == Diagnostic::Severity::Error) ? "error" : "warning";
        std::cerr << tag << ": " << d.message << "\n";
    }
}

void SemanticAnalyzer::error(const std::string &msg) {
    diags.push_back({Diagnostic::Severity::Error, msg});
}

void SemanticAnalyzer::warning(const std::string &msg) {
    diags.push_back({Diagnostic::Severity::Warning, msg});
}

} // namespace Sem