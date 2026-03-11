#include "sem.h"
#include <variant>

namespace Sem {

void SemanticAnalyzer::declaration_pass() {
    for (auto &item : program.items) {
        std::visit([this](auto &decl) {

            using T = std::decay_t<decltype(decl)>;
            if constexpr (std::is_same_v<T, AST::Function>) {
                // Build symbol
                Symbol sym;
                sym.kind = Symbol::Kind::Function;
                sym.return_type = decl.return_type
                    ? from_ast_type(*decl.return_type)
                    : make_none();

                for (auto &p : decl.params)
                    sym.param_types.push_back(from_ast_type(p.type));

                if (!scopes.declare(decl.name, std::move(sym)))
                    error("redefinition of function '" + decl.name + "'");

            } else if constexpr (std::is_same_v<T, AST::StructDecl>) {
                Symbol sym;
                sym.kind = Symbol::Kind::Struct;
                sym.type = make_user(decl.name);

                for (auto &f : decl.fields)
                    sym.fields.emplace_back(f.name, from_ast_type(f.type));

                // Check for duplicate field names
                for (size_t i = 0; i < decl.fields.size(); ++i)
                    for (size_t j = i + 1; j < decl.fields.size(); ++j)
                        if (decl.fields[i].name == decl.fields[j].name)
                            error("duplicate field '" + decl.fields[i].name +
                                  "' in struct '" + decl.name + "'");

                if (!scopes.declare(decl.name, std::move(sym)))
                    error("redefinition of struct '" + decl.name + "'");

            } else if constexpr (std::is_same_v<T, AST::ConstDecl>) {
                Symbol sym;
                sym.kind = Symbol::Kind::Const;
                sym.type = from_ast_type(decl.type);
                sym.is_const = true;

                if (!scopes.declare(decl.name, std::move(sym)))
                    error("redefinition of constant '" + decl.name + "'");

            } else if constexpr (std::is_same_v<T, AST::ExternDecl>) {
                Symbol sym;
                sym.kind = Symbol::Kind::Extern;
                sym.return_type = decl.return_type
                    ? from_ast_type(*decl.return_type)
                    : make_none();

                for (auto &p : decl.params)
                    sym.param_types.push_back(from_ast_type(p.type));

                if (!scopes.declare(decl.name, std::move(sym)))
                    error("redefinition of extern '" + decl.name + "'");
            }
        }, item);
    }

    // Verify `main` exists
    Symbol *main_sym = scopes.lookup("main");
    if (!main_sym)
        error("no 'main' function defined");
    else if (main_sym->kind != Symbol::Kind::Function)
        error("'main' must be a function");
}

} // namespace Sem
