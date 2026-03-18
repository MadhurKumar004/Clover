#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"

namespace Codegen {

class Generator {
public:
    Generator(AST::Program& program, const std::string& module_name = "clover");
    ~Generator();

    std::unique_ptr<llvm::Module> generate();
    llvm::LLVMContext& context();

private:
    llvm::LLVMContext ctx_;
    llvm::IRBuilder<> builder_;
    std::unique_ptr<llvm::Module> module_;
    AST::Program& program_;

    // Scopes: stack of (name -> alloca) maps
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes_;
    std::unordered_map<std::string, llvm::Function*> functions_;
    std::unordered_map<std::string, llvm::StructType*> struct_types_;
    std::unordered_map<std::string, std::unordered_map<std::string, unsigned>> struct_fields_;

    // Loop control
    llvm::BasicBlock* break_target_    = nullptr;
    llvm::BasicBlock* continue_target_ = nullptr;

    // Scope helpers
    void push_scope();
    void pop_scope();
    void declare_local(const std::string& name, llvm::AllocaInst* alloca);
    llvm::AllocaInst* lookup_local(const std::string& name);

    // Two-pass: declare prototypes, then emit bodies
    void declare_all();
    void emit_all();

    // Type conversion
    llvm::Type* to_llvm_type(const AST::Type& type);
    llvm::Type* builtin_to_llvm(AST::BuiltinType bt);

    // Top-level declarations
    void declare_function(const AST::Function& fn);
    void declare_extern(const AST::ExternDecl& ext);
    void declare_struct(const AST::StructDecl& st);

    // Function body emission
    void emit_function(const AST::Function& fn);

    // Block: takes Block (which has vector<StmtPtr> body)
    void emit_block(const AST::Block& block);

    // Statement emission
    void emit_stmt(const AST::Statement& stmt);
    void emit_var_decl(const AST::VarDecl& decl);
    void emit_if_stmt(const AST::IfStmt& stmt);
    void emit_do_stmt(const AST::DoStmt& stmt);
    void emit_loop_stmt(const AST::LoopStmt& stmt);
    void emit_return_stmt(const AST::ReturnStmt& stmt);
    void emit_unsafe_stmt(const AST::UnsafeStmt& stmt);

    // Expression emission
    llvm::Value* emit_expr(const AST::Expression& expr);
    llvm::Value* emit_number(const AST::Expression::Number& num);
    llvm::Value* emit_bool(const AST::Expression::Bool& b);
    llvm::Value* emit_char(const AST::Expression::Char& c);
    llvm::Value* emit_string(const AST::Expression::String& s);
    llvm::Value* emit_identifier(const AST::Expression::Identifier& id);
    llvm::Value* emit_binary(const AST::Expression::Binary& bin);
    llvm::Value* emit_unary(const AST::Expression::Unary& un);
    llvm::Value* emit_call(const AST::Expression::Call& call);
    llvm::Value* emit_assign(const AST::Expression::Assign& asgn);
    llvm::Value* emit_field_access(const AST::Expression::FieldAccess& fa);
    llvm::Value* emit_cast(const AST::Expression::Cast& cast);

    // Helpers
    llvm::AllocaInst* create_entry_alloca(
        llvm::Function* fn, const std::string& name, llvm::Type* type);
    llvm::Function* current_function();
    llvm::Value* emit_lvalue(const AST::Expression& expr);
    llvm::Type* lvalue_type(const AST::Expression& expr);
    llvm::Value* cast_value_to(llvm::Value* val, llvm::Type* target_ty);
    bool is_float_type(llvm::Type* ty);
    bool is_int_type(llvm::Type* ty);
};

} // namespace Codegen

#endif // CODEGEN_H
