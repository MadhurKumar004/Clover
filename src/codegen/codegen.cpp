//This file generates llvm ir for the clover ast

#include "ast.h"
#include "codegen.h"
#include "passes.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

//#include <iostream>
#include <stdexcept>
#include <variant>

namespace Codegen{

    Generator::Generator(AST::Program& program, const std::string& module_name)
        :   builder_(ctx_),
            module_(std::make_unique<llvm::Module>(module_name, ctx_)),
            program_(program)
    {}

    Generator::~Generator() = default;

    llvm::LLVMContext& Generator::context(){ return ctx_; }

    std::unique_ptr<llvm::Module> Generator::generate(){
        // pass1 for declaring all function, struct, extern
        declare_all();

        //pass2 for emitting function bodies
        emit_all();

        // Detect invalid IR before running any analysis/pass pipelines.
        if (llvm::verifyModule(*module_, &llvm::errs())) {
            throw std::runtime_error("codegen: invalid LLVM module before passes");
        }

        {
            llvm::PassBuilder PB;
            llvm::ModuleAnalysisManager MAM;
            llvm::FunctionAnalysisManager FAM;
            llvm::LoopAnalysisManager LAM;
            llvm::CGSCCAnalysisManager CGAM;

            PB.registerModuleAnalyses(MAM);
            PB.registerFunctionAnalyses(FAM);
            PB.registerLoopAnalyses(LAM);
            PB.registerCGSCCAnalyses(CGAM);
            PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

            llvm::FunctionPassManager FPM;
            FPM.addPass(DcePass{});

            llvm::ModulePassManager MPM;
            MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));

            MPM.run(*module_, MAM);
            MAM.clear();
        }

        return std::move(module_);
    }

    void Generator::push_scope(){
        scopes_.emplace_back();
    }

    void Generator::pop_scope(){
        scopes_.pop_back();
    }

    void Generator::declare_local(const std::string& name, llvm::AllocaInst* alloca){
        scopes_.back()[name] = alloca;
    }

    llvm::AllocaInst* Generator::lookup_local(const std::string& name){
        //Search from innermost scope to outermost
        for(auto it = scopes_.rbegin(); it != scopes_.rend(); ++it){
            auto found = it->find(name);
            if(found != it->end())
                return found->second;
        }
        return nullptr;
    }

    //Helper : create alloca in entry block
    llvm::AllocaInst* Generator::create_entry_alloca(
            llvm::Function* fn,
            const std::string& name,
            llvm::Type* type
    ){
        //create temp builder that insert at start of entry block
        llvm::IRBuilder<> tmp_builder(
                &fn->getEntryBlock(),
                fn->getEntryBlock().begin()
        );
        return tmp_builder.CreateAlloca(type, nullptr, name);
    }

    llvm::Function* Generator::current_function(){
        return builder_.GetInsertBlock()->getParent();
    }

    bool Generator::is_float_type(llvm::Type* ty){
        return ty->isFloatTy() || ty->isDoubleTy();
    }

    bool Generator::is_int_type(llvm::Type* ty){
        return ty->isIntegerTy();
    }

    llvm::Value* Generator::cast_value_to(llvm::Value* val, llvm::Type* target_ty){
        llvm::Type* src_ty = val->getType();

        if(src_ty == target_ty)
            return val;

        // Int -> Float
        if(is_int_type(src_ty) && is_float_type(target_ty))
            return builder_.CreateSIToFP(val, target_ty, "coerce");

        // Float -> Int
        if(is_float_type(src_ty) && is_int_type(target_ty))
            return builder_.CreateFPToSI(val, target_ty, "coerce");

        // Float -> Float (extend/truncate)
        if(is_float_type(src_ty) && is_float_type(target_ty)){
            if(src_ty->getPrimitiveSizeInBits() < target_ty->getPrimitiveSizeInBits())
                return builder_.CreateFPExt(val, target_ty, "coerce");
            return builder_.CreateFPTrunc(val, target_ty, "coerce");
        }

        // Int -> Int (extend/truncate)
        if(is_int_type(src_ty) && is_int_type(target_ty)){
            unsigned src_bits = src_ty->getIntegerBitWidth();
            unsigned dst_bits = target_ty->getIntegerBitWidth();
            if(src_bits < dst_bits)
                return builder_.CreateSExt(val, target_ty, "coerce");
            if(src_bits > dst_bits)
                return builder_.CreateTrunc(val, target_ty, "coerce");
            return val;
        }

        // Pointer <-> Pointer/Int
        if(src_ty->isPointerTy() && target_ty->isPointerTy())
            return builder_.CreateBitCast(val, target_ty, "coerce");

        if(is_int_type(src_ty) && target_ty->isPointerTy())
            return builder_.CreateIntToPtr(val, target_ty, "coerce");

        if(src_ty->isPointerTy() && is_int_type(target_ty))
            return builder_.CreatePtrToInt(val, target_ty, "coerce");

        throw std::runtime_error("codegen: cannot coerce value to destination type");
    }

    llvm::Type* Generator::lvalue_type(const AST::Expression& expr){
        return std::visit([this](auto& node) -> llvm::Type*{
            using T = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<T, AST::Expression::Identifier>){
                auto* alloca = lookup_local(node.name);
                if(!alloca)
                    throw std::runtime_error("codegen: unknown variable '" + node.name + "'");
                return alloca->getAllocatedType();
            } else if constexpr (std::is_same_v<T, AST::Expression::FieldAccess>){
                llvm::Type* obj_ty = lvalue_type(*node.object);
                auto* struct_ty = llvm::dyn_cast<llvm::StructType>(obj_ty);
                if(!struct_ty || !struct_ty->hasName())
                    throw std::runtime_error("codegen: field access on non-struct type");

                std::string struct_name = struct_ty->getName().str();
                auto fields_it = struct_fields_.find(struct_name);
                if(fields_it == struct_fields_.end())
                    throw std::runtime_error("codegen: unknown struct '" + struct_name + "'");

                auto idx_it = fields_it->second.find(node.field);
                if(idx_it == fields_it->second.end())
                    throw std::runtime_error("codegen: unknown field '" + node.field + "' in '" + struct_name + "'");

                return struct_ty->getElementType(idx_it->second);
            } else {
                throw std::runtime_error("codegen: expression is not a lvalue");
            }
        }, expr.value);
    }

    llvm::Type* Generator::builtin_to_llvm(AST::BuiltinType bt){
        switch(bt){
            case AST::BuiltinType::I1:   return llvm::Type::getInt1Ty(ctx_);
            case AST::BuiltinType::I8:   return llvm::Type::getInt8Ty(ctx_);
            case AST::BuiltinType::I16:  return llvm::Type::getInt16Ty(ctx_);
            case AST::BuiltinType::I32:  return llvm::Type::getInt32Ty(ctx_);
            case AST::BuiltinType::I64:  return llvm::Type::getInt64Ty(ctx_);
            case AST::BuiltinType::I128: return llvm::Type::getInt128Ty(ctx_);
            case AST::BuiltinType::U8:   return llvm::Type::getInt8Ty(ctx_);
            case AST::BuiltinType::U16:  return llvm::Type::getInt16Ty(ctx_);
            case AST::BuiltinType::U32:  return llvm::Type::getInt32Ty(ctx_);
            case AST::BuiltinType::U64:  return llvm::Type::getInt64Ty(ctx_);
            case AST::BuiltinType::U128: return llvm::Type::getInt128Ty(ctx_);
            case AST::BuiltinType::F16:  return llvm::Type::getHalfTy(ctx_);
            case AST::BuiltinType::F32:  return llvm::Type::getFloatTy(ctx_);
            case AST::BuiltinType::F64:  return llvm::Type::getDoubleTy(ctx_);
            case AST::BuiltinType::F128: return llvm::Type::getFP128Ty(ctx_);
            case AST::BuiltinType::Char: return llvm::Type::getInt8Ty(ctx_);
            default:
                throw std::runtime_error("codegen: unsupported builtin type");
        }
    }

    llvm::Type* Generator::to_llvm_type(const AST::Type& type){
        // Use std::visit to handle each AST::Type::value
        return std::visit([this](auto& ty) -> llvm::Type* {
                using T = std::decay_t<decltype(ty)>;

                if constexpr (std::is_same_v<T, AST::Type::Builtin>){
                        return builtin_to_llvm(ty.type);
                } else if constexpr (std::is_same_v<T, AST::Type::UserDef>){
                    auto it = struct_types_.find(ty.name);
                    if(it == struct_types_.end())
                        throw std::runtime_error("codegen: unknown type '" + ty.name + "'");
                    return it->second;
                } else if constexpr (std::is_same_v<T, AST::Type::Pointer> ||
                                    std::is_same_v<T, AST::Type::ConstPtr>){
                    llvm::Type* inner = to_llvm_type(*ty.inner);
                    return llvm::PointerType::get(ctx_, 0);
                } else {
                    throw std::runtime_error("codegen: unsupported type variant");
                }

        }, type.value);
    }

    void Generator::declare_all(){
        for(auto& item : program_.items) {
            std::visit([this](auto& node){
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, AST::StructDecl>){
                    declare_struct(node);
                } else if constexpr (std::is_same_v<T, AST::Function>){
                    declare_function(node);
                } else if constexpr (std::is_same_v<T, AST::ExternDecl>){
                    declare_extern(node);
                } else if constexpr (std::is_same_v<T, AST::ConstDecl>){
                    //for now handled inline where used
                }
            }, item);
        }
    }

    void Generator::declare_struct(const AST::StructDecl& st){
        auto struct_ty = llvm::StructType::create(ctx_, st.name);
        struct_types_[st.name] = struct_ty;

        std::vector<llvm::Type*> field_types;
        std::unordered_map<std::string, unsigned> field_indices;

        for(unsigned i = 0; i < st.fields.size(); ++i){
            field_types.push_back(to_llvm_type(st.fields[i].type));
            field_indices[st.fields[i].name] = i;
        }

        struct_ty->setBody(field_types, /* isPacked*/ false);
        struct_fields_[st.name] = std::move(field_indices);
    }

    void Generator::declare_extern(const AST::ExternDecl& ext){
        std::vector<llvm::Type*> param_types;
        for(const auto& p : ext.params)
            param_types.push_back(to_llvm_type(p.type));

        llvm::Type* ret_ty = ext.return_type
            ? to_llvm_type(*ext.return_type)
            : llvm::Type::getVoidTy(ctx_);

        auto* fn_ty = llvm::FunctionType::get(ret_ty, param_types, false);

        auto* fn = llvm::Function::Create(
                fn_ty,
                llvm::Function::ExternalLinkage,
                ext.name,
                module_.get()
        );

        functions_[ext.name] = fn;
    }

    void Generator::declare_function(const AST::Function& fn){
        std::vector<llvm::Type*> param_types;
        for(const auto& p : fn.params)
            param_types.push_back(to_llvm_type(p.type));

        llvm::Type* ret_ty = fn.return_type
            ? to_llvm_type(*fn.return_type)
            : llvm::Type::getVoidTy(ctx_);

        auto* fn_ty = llvm::FunctionType::get(ret_ty, param_types, false);

        auto* llvm_fn = llvm::Function::Create(
                fn_ty,
                llvm::Function::ExternalLinkage,
                fn.name,
                module_.get()
        );

        unsigned i = 0;
        for(auto& arg : llvm_fn->args())
            arg.setName(fn.params[i++].name);

        functions_[fn.name] = llvm_fn;
    }

    void Generator::emit_all(){
        for(auto& item : program_.items){
            std::visit([this](auto& node){
                    using T = std::decay_t<decltype(node)>;
                    if constexpr(std::is_same_v<T, AST::Function>){
                        emit_function(node);
                    }
                    }, item);
        }
    }

    void Generator::emit_function(const AST::Function& fn){
        llvm::Function* llvm_fn = functions_[fn.name];
        if(!llvm_fn) throw std::runtime_error("codegen: function not declared: "  + fn.name);

        //dont re-emit if already has a body
        if(!llvm_fn->empty()) return;
        auto* entry = llvm::BasicBlock::Create(ctx_, "entry", llvm_fn);
        builder_.SetInsertPoint(entry);

        //push a new scopre for function-level variables
        push_scope();

        std::vector<std::pair<llvm::AllocaInst*, llvm::Argument*>> param_allocas;
        unsigned i = 0;
        for(auto& arg : llvm_fn->args()){
            const auto& param = fn.params[i++];
            auto* alloca = create_entry_alloca(llvm_fn, param.name, arg.getType());
            declare_local(param.name, alloca);
            param_allocas.push_back({alloca, &arg});
        }

        for(auto& [alloca, arg] : param_allocas){
            builder_.CreateStore(arg, alloca);
        }

        //generate function body
        emit_block(*fn.body);

        if(!builder_.GetInsertBlock()->getTerminator()){
            if(llvm_fn->getReturnType()->isVoidTy()){
                builder_.CreateRetVoid();
            } else{
                //return a zero/null deafult (semantic checker should catch this)
                builder_.CreateRet(llvm::Constant::getNullValue(llvm_fn->getReturnType()));
            }
        }

        pop_scope();
    }

    void Generator::emit_block(const AST::Block& block){
        push_scope();

        for(const auto& stmt_ptr : block.body){
            emit_stmt(*stmt_ptr);

            if(builder_.GetInsertBlock()->getTerminator())
                break;
        }

        pop_scope();
    }

    void Generator::emit_stmt(const AST::Statement& stmt){
        std::visit([this](auto& node){
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, AST::VarDecl>){
                    emit_var_decl(node);
                } else if constexpr (std::is_same_v<T, AST::IfStmt>){
                    emit_if_stmt(node);
                } else if constexpr (std::is_same_v<T, AST::DoStmt>){
                    emit_do_stmt(node);
                } else if constexpr (std::is_same_v<T, AST::LoopStmt>){
                    emit_loop_stmt(node);
                } else if constexpr (std::is_same_v<T, AST::ReturnStmt>){
                    emit_return_stmt(node);
                } else if constexpr (std::is_same_v<T, AST::UnsafeStmt>){
                    emit_unsafe_stmt(node);
                } else if constexpr (std::is_same_v<T, AST::ExprStmt>){
                    emit_expr(*node.expr);
                } else if constexpr (std::is_same_v<T, AST::BreakStmt>){
                    //unconditional branch to loop exit
                    if(!break_target_)
                        throw std::runtime_error("codegen: break outside loop");
                    builder_.CreateBr(break_target_);
                } else if constexpr (std::is_same_v<T, AST::ContinueStmt>){
                    if(!continue_target_)
                        throw std::runtime_error("codegen: continue outside loop");
                    builder_.CreateBr(continue_target_);
                }
        }, stmt.value);
    }

    void Generator::emit_var_decl(const AST::VarDecl& decl){
        auto* fn = current_function();
        auto* ty = to_llvm_type(decl.type);
        auto* slot = create_entry_alloca(fn, decl.name, ty);
        if(decl.init){
            auto* init_val = emit_expr(*decl.init);
            init_val = cast_value_to(init_val, ty);
            builder_.CreateStore(init_val, slot);
        }

        declare_local(decl.name, slot);
    }

    void Generator::emit_if_stmt(const AST::IfStmt& stmt){
        auto* fn = current_function();
        auto* merge_bb = llvm::BasicBlock::Create(ctx_, "if.end");

        for(size_t i = 0; i < stmt.branches.size(); ++i){
            const auto& branch = stmt.branches[i];
            auto* cond_val = emit_expr(*branch.condition);

            // Coerce to i1 if needed
            if(!cond_val->getType()->isIntegerTy(1)){
                if(is_int_type(cond_val->getType())){
                    cond_val = builder_.CreateICmpNE(
                            cond_val,
                            llvm::ConstantInt::get(cond_val->getType(), 0),
                            "ifcond"
                            );
                } else if(is_float_type(cond_val->getType())){
                    cond_val = builder_.CreateFCmpONE(
                            cond_val,
                            llvm::ConstantFP::get(cond_val->getType(), 0.0),
                            "ifcond"
                            );
                }
            }

            auto* then_bb = llvm::BasicBlock::Create(ctx_, "if.then", fn);
            auto* next_bb = llvm::BasicBlock::Create(ctx_, "if.next");

            builder_.CreateCondBr(cond_val, then_bb, next_bb);

            // Emit then block
            builder_.SetInsertPoint(then_bb);
            emit_block(*branch.body);
            if(!builder_.GetInsertBlock()->getTerminator())
                builder_.CreateBr(merge_bb);

            // Move to next condition or else/merge
            fn->insert(fn->end(), next_bb);
            builder_.SetInsertPoint(next_bb);
        }

        // Emit else block if present
        if(stmt.else_body){
            emit_block(**stmt.else_body);
        }
        if(!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(merge_bb);

        fn->insert(fn->end(), merge_bb);
        builder_.SetInsertPoint(merge_bb);
    }

    void Generator::emit_do_stmt(const AST::DoStmt& stmt){
        auto* fn = current_function();

        auto* body_bb = llvm::BasicBlock::Create(ctx_, "do.body", fn);
        auto* cond_bb = llvm::BasicBlock::Create(ctx_, "do.cond");
        auto* end_bb = llvm::BasicBlock::Create(ctx_, "do.end");

        //save and set loop targets for break/continue
        auto* old_break = break_target_;
        auto* old_continue = continue_target_;
        break_target_ = end_bb;
        continue_target_ = cond_bb;

        //jump into loop body
        builder_.CreateBr(body_bb);

        //Emit body
        builder_.SetInsertPoint(body_bb);
        emit_block(*stmt.body);
        if(!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(cond_bb);

        //emit condition check
        fn->insert(fn->end(), cond_bb);
        builder_.SetInsertPoint(cond_bb);
        auto* cond_val = emit_expr(*stmt.condition);
        if(!cond_val->getType()->isIntegerTy(1)){
            cond_val = builder_.CreateICmpNE(
                    cond_val,
                    llvm::ConstantInt::get(cond_val->getType(), 0),
                    "do.cond"
                    );
        }
        builder_.CreateCondBr(cond_val, body_bb, end_bb);

        //Continue after loop
        fn->insert(fn->end(), end_bb);
        builder_.SetInsertPoint(end_bb);

        //Restore old loop targets
        break_target_ = old_break;
        continue_target_ = old_continue;
    }

    void Generator::emit_loop_stmt(const AST::LoopStmt& stmt){
        auto* fn = current_function();

        auto* loop_bb = llvm::BasicBlock::Create(ctx_, "loop.body", fn);
        auto* end_bb = llvm::BasicBlock::Create(ctx_, "loop.end");

        auto* old_break = break_target_;
        auto* old_continue = continue_target_;
        break_target_ = end_bb;
        continue_target_ = loop_bb;

        builder_.CreateBr(loop_bb);

        builder_.SetInsertPoint(loop_bb);
        emit_block(*stmt.body);
        if(!builder_.GetInsertBlock()->getTerminator())
            builder_.CreateBr(loop_bb); // loop back

        fn->insert(fn->end(), end_bb);
        builder_.SetInsertPoint(end_bb);

        break_target_ = old_break;
        continue_target_ = old_continue;
    }
    
    void Generator::emit_return_stmt(const AST::ReturnStmt& stmt){
        if(stmt.value){
            auto* val = emit_expr(*stmt.value);
            builder_.CreateRet(val);
        } else {
            builder_.CreateRetVoid();
        }
    }

    void Generator::emit_unsafe_stmt(const AST::UnsafeStmt& stmt){
        emit_block(*stmt.body);
    }

    llvm::Value* Generator::emit_lvalue(const AST::Expression& expr){
        return std::visit([this](auto& node) -> llvm::Value*{
                using T = std::decay_t<decltype(node)>;

                if constexpr (std::is_same_v<T, AST::Expression::Identifier>){
                    auto* alloca = lookup_local(node.name);
                    if(!alloca)
                        throw std::runtime_error("codegen: unknown variable '" + node.name + "'");
                    return alloca; // return the pointer not loaded value
                } else if constexpr (std::is_same_v<T, AST::Expression::FieldAccess>){
                    auto* obj_ptr = emit_lvalue(*node.object);
                    auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(obj_ptr);
                    if(!alloca)
                        throw std::runtime_error("codegen: field access on non-alloca");

                    auto* struct_ty = llvm::dyn_cast<llvm::StructType>(alloca->getAllocatedType());
                    if(!struct_ty || !struct_ty->hasName())
                        throw std::runtime_error("codegen: field access on non-struct type");

                    std::string struct_name = struct_ty->getName().str();
                    auto fields_it = struct_fields_.find(struct_name);
                    if(fields_it == struct_fields_.end())
                        throw std::runtime_error("codegen: unknown struct '" + struct_name + "'");

                    auto idx_it = fields_it->second.find(node.field);
                    if(idx_it == fields_it->second.end())
                        throw std::runtime_error("codegen: unknown field '" + node.field + "' in '" + struct_name + "'");

                    unsigned field_idx = idx_it->second;

                    return builder_.CreateStructGEP(struct_ty, obj_ptr, field_idx, node.field + "_ptr");
                } else {
                    throw std::runtime_error("codegen: expression is not a lvalue");
                }
        }, expr.value);
    }

    llvm::Value* Generator::emit_expr(const AST::Expression& expr){
        return std::visit([this](auto& node) -> llvm::Value*{
                using T = std::decay_t<decltype(node)>;

                if constexpr (std::is_same_v<T, AST::Expression::Number>){
                    return emit_number(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Bool>){
                    return emit_bool(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Char>){
                    return emit_char(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::String>){
                    return emit_string(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Identifier>){
                    return emit_identifier(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Binary>){
                    return emit_binary(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Unary>){
                    return emit_unary(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Call>){
                    return emit_call(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Assign>){
                    return emit_assign(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::FieldAccess>){
                    return emit_field_access(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Cast>){
                    return emit_cast(node);
                } else if constexpr (std::is_same_v<T, AST::Expression::Grouped>){
                    //just emit inner expression
                    return emit_expr(*node.inner);
                } else if constexpr (std::is_same_v<T, AST::Expression::PtrFieldAccess>){
                    throw std::runtime_error("codegen: pointer field access not yet implemented");
                } else if constexpr (std::is_same_v<T, AST::Expression::Index>){
                    throw std::runtime_error("codegen: index expression not yet implemented");
                } else {
                    throw std::runtime_error("codegen: unsupported expression");
                }
        }, expr.value);
    }

    llvm::Value* Generator::emit_number(const AST::Expression::Number& num){
        const auto& raw = num.lit.raw;

        // Float if raw contains '.'
        if(raw.find('.') != std::string::npos){
            return llvm::ConstantFP::get(
                    llvm::Type::getDoubleTy(ctx_),
                    std::stod(raw)
            );
        }

        long long int_val;
        if(raw.size() > 2 && raw[0] == '0'){
            char prefix = raw[1];
            if(prefix == 'x' || prefix == 'X')
                int_val = std::stoll(raw, nullptr, 16);
            else if (prefix == 'b' || prefix == 'B')
                int_val = std::stoll(raw.substr(2), nullptr, 2);
            else if (prefix == 'o' || prefix == 'O')
                int_val = std::stoll(raw.substr(2), nullptr, 8);
            else
                int_val = std::stoll(raw);
        } else {
            int_val = std::stoll(raw);
        }

        return llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(ctx_),
                int_val,
                true
        );
    }

    llvm::Value* Generator::emit_bool(const AST::Expression::Bool& b){
        return llvm::ConstantInt::get(
                llvm::Type::getInt1Ty(ctx_),
                b.lit.value ? 1 : 0
        );
    }

    llvm::Value* Generator::emit_char(const AST::Expression::Char& c){
        return llvm::ConstantInt::get(
                llvm::Type::getInt8Ty(ctx_),
                static_cast<unsigned char>(c.lit.value)
        );
    }

    llvm::Value* Generator::emit_string(const AST::Expression::String& s){
        return builder_.CreateGlobalString(s.lit.value, "str");
    }

    llvm::Value* Generator::emit_identifier(const AST::Expression::Identifier& id){
        auto* alloca = lookup_local(id.name);
        if(alloca)
            return builder_.CreateLoad(alloca->getAllocatedType(), alloca, id.name);

        auto fn_it = functions_.find(id.name);
        if(fn_it != functions_.end())
            return fn_it->second;

        throw std::runtime_error("codegen: undefined identifier '" + id.name + "'");
    }

    llvm::Value* Generator::emit_binary(const AST::Expression::Binary& bin){
        using Op = AST::BinaryOp;

        auto* lhs = emit_expr(*bin.left);
        auto* rhs = emit_expr(*bin.right);
        bool is_fp = is_float_type(lhs->getType());

        switch(bin.op){
            case Op::Add:
                return is_fp
                    ? builder_.CreateFAdd(lhs, rhs, "addtmp")
                    : builder_.CreateAdd(lhs, rhs, "addtmp");

            case Op::Sub:
                return is_fp
                    ? builder_.CreateFSub(lhs, rhs, "subtmp")
                    : builder_.CreateSub(lhs, rhs, "subtmp");

            case Op::Mul:
                return is_fp
                    ? builder_.CreateFMul(lhs, rhs, "multmp")
                    : builder_.CreateMul(lhs, rhs, "multmp");

            case Op::Div:
                return is_fp
                    ? builder_.CreateFDiv(lhs, rhs, "divtmp")
                    : builder_.CreateSDiv(lhs, rhs, "divtmp");

            case Op::Mod:
                return builder_.CreateSRem(lhs, rhs, "modtmp");

            case Op::Eq:
                return is_fp
                    ? builder_.CreateFCmpOEQ(lhs, rhs, "eqtmp")
                    : builder_.CreateICmpEQ(lhs, rhs, "eqtmp");

            case Op::Ne:
                return is_fp
                    ? builder_.CreateFCmpONE(lhs, rhs, "netmp")
                    : builder_.CreateICmpNE(lhs, rhs, "netmp");

            case Op::Lt:
                    return is_fp
                        ? builder_.CreateFCmpOLT(lhs, rhs, "lttmp")
                        : builder_.CreateICmpSLT(lhs, rhs, "lttmp");

            case Op::Le:
                    return is_fp
                        ? builder_.CreateFCmpOLE(lhs, rhs, "letmp")
                        : builder_.CreateICmpSLE(lhs, rhs, "letmp");

            case Op::Gt:
                    return is_fp
                        ? builder_.CreateFCmpOGT(lhs, rhs, "gttmp")
                        : builder_.CreateICmpSGT(lhs, rhs, "gttmp");

            case Op::Ge:
                    return is_fp
                        ? builder_.CreateFCmpOGE(lhs, rhs, "getmp")
                        : builder_.CreateICmpSGE(lhs, rhs, "getmp");

            case Op::BitAnd:
                    return builder_.CreateAnd(lhs, rhs, "andtmp");
            case Op::BitOr:
                    return builder_.CreateOr(lhs, rhs, "ortmp");
            case Op::BitXor:
                    return builder_.CreateXor(lhs, rhs, "xortmp");
            case Op::Shl:
                    return builder_.CreateShl(lhs, rhs, "shltmp");
            case Op::Shr:
                    return builder_.CreateAShr(lhs, rhs, "shrtmp");

            case Op::And:
                    return builder_.CreateAnd(lhs, rhs, "logandtmp");
            case Op::Or:
                    return builder_.CreateOr(lhs, rhs, "logortmp");

            default:
                    throw std::runtime_error("codegen: unsupported binary operator");
        }
    }

    llvm::Value* Generator::emit_unary(const AST::Expression::Unary& un){
        using Op = AST::UnaryOp;

        switch(un.op){
            case Op::Negate: {
                auto* operand = emit_expr(*un.operand);
                return is_float_type(operand->getType())
                    ? builder_.CreateFNeg(operand, "negtmp")
                    : builder_.CreateNeg(operand, "negtmp");
            }

            case Op::Not: {
                // Logical NOT: compare == 0
                auto* operand = emit_expr(*un.operand);
                if(is_int_type(operand->getType())){
                    return builder_.CreateICmpEQ(
                            operand,
                            llvm::ConstantInt::get(operand->getType(), 0),
                            "nottmp"
                    );
                }
                return builder_.CreateFCmpOEQ(
                        operand,
                        llvm::ConstantFP::get(operand->getType(), 0.0),
                        "nottmp"
                );
            }

            case Op::BitNot: {
                auto* operand = emit_expr(*un.operand);
                return builder_.CreateNot(operand, "bitnottmp");
            }

            case Op::Deref: {
                auto* operand = emit_expr(*un.operand);
                if(!operand->getType()->isPointerTy())
                    throw std::runtime_error("codegen: dereference of non-pointer");
                return builder_.CreateLoad(
                    llvm::Type::getInt32Ty(ctx_), operand, "dereftmp"
                );
            }

            case Op::Addrof:
                return emit_lvalue(*un.operand);

            default:
                throw std::runtime_error("codegen: unsupported unary operator");
        }
    }

    llvm::Value* Generator::emit_call(const AST::Expression::Call& call){
        auto* callee_val = emit_expr(*call.callee);
        auto* callee_fn = llvm::dyn_cast<llvm::Function>(callee_val);
        if(!callee_fn)
            throw std::runtime_error("codegen: callee is not a function");

        std::vector<llvm::Value*> args;
        args.reserve(call.args.size());
        for(const auto& arg : call.args){
            args.push_back(emit_expr(*arg));
        }

        auto* fn_ty = callee_fn->getFunctionType();
        unsigned param_count = fn_ty->getNumParams();
        if(!fn_ty->isVarArg() && args.size() != param_count)
            throw std::runtime_error("codegen: argument count mismatch in call");

        if(fn_ty->isVarArg() && args.size() < param_count)
            throw std::runtime_error("codegen: too few arguments in variadic call");

        for(unsigned i = 0; i < param_count; ++i){
            args[i] = cast_value_to(args[i], fn_ty->getParamType(i));
        }

        if(callee_fn->getReturnType()->isVoidTy()){
            return builder_.CreateCall(callee_fn, args);
        } else {
            return builder_.CreateCall(callee_fn, args, "calltmp");
        }
    }

    llvm::Value* Generator::emit_assign(const AST::Expression::Assign& asgn){
        auto* ptr = emit_lvalue(*asgn.target);
        auto* val = emit_expr(*asgn.value);
        auto* target_ty = lvalue_type(*asgn.target);
        val = cast_value_to(val, target_ty);
        builder_.CreateStore(val, ptr);
        return val;
    }

    llvm::Value* Generator::emit_field_access(const AST::Expression::FieldAccess& fa){
        auto* obj_ptr = emit_lvalue(*fa.object);
        auto* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(obj_ptr);
        if(!alloca_inst)
            throw std::runtime_error("codegen: field access on non-alloca");

        auto* struct_ty = llvm::dyn_cast<llvm::StructType>(alloca_inst->getAllocatedType());
        if(!struct_ty || !struct_ty->hasName())
            throw std::runtime_error("codegen: field access on non-struct");

        std::string struct_name = struct_ty->getName().str();
        auto fields_it = struct_fields_.find(struct_name);
        if(fields_it == struct_fields_.end())
            throw std::runtime_error("codegen: unknown struct '" + struct_name + "'");

        auto idx_it = fields_it->second.find(fa.field);
        if(idx_it == fields_it->second.end())
            throw std::runtime_error("codegen: unknown field '" + fa.field + "'");

        unsigned field_idx = idx_it->second;
        auto* field_ptr = builder_.CreateStructGEP(struct_ty, obj_ptr, field_idx, fa.field + "_ptr");

        llvm::Type* field_ty = struct_ty->getElementType(field_idx);
        return builder_.CreateLoad(field_ty, field_ptr, fa.field);
    }


    llvm::Value* Generator::emit_cast(const AST::Expression::Cast& cast){
        auto* val = emit_expr(*cast.operand);
        auto* target_ty = to_llvm_type(*cast.target);

        llvm::Type* src_ty = val->getType();

        //same type : no-op
        if(src_ty == target_ty)
            return val;

        //Int -> Float
        if(is_int_type(src_ty) && is_float_type(target_ty))
            return builder_.CreateSIToFP(val, target_ty, "casttmp");

        //Float -> Int
        if(is_float_type(src_ty) && is_int_type(target_ty))
            return builder_.CreateFPToSI(val, target_ty, "casttmp");

        //Float -> Float (extend or truncate)
        if(is_float_type(src_ty) && is_float_type(target_ty)){
            if(src_ty->getPrimitiveSizeInBits() < target_ty->getPrimitiveSizeInBits())
                return builder_.CreateFPExt(val, target_ty, "casttmp");
            else
                return builder_.CreateFPTrunc(val, target_ty, "casttmp");
        }

        //Int -> Int (extend or truncate)
        if(is_int_type(src_ty) && is_int_type(target_ty)){
            unsigned src_bits = src_ty->getIntegerBitWidth();
            unsigned dst_bits = target_ty->getIntegerBitWidth();

            if(src_bits < dst_bits)
                return builder_.CreateSExt(val, target_ty, "casttmp"); // sign extend
            else if(src_bits > dst_bits)
                return builder_.CreateTrunc(val, target_ty, "casttmp"); // truncate
            else
                return val; // same width
        }

        //Pointer -> pointer (bitcast)
        if(src_ty->isPointerTy() && target_ty->isPointerTy())
            return builder_.CreateBitCast(val, target_ty, "casttmp");

        //Int -> pointer
        if(is_int_type(src_ty) && target_ty->isPointerTy())
            return builder_.CreateIntToPtr(val, target_ty, "casttmp");

        //Pointer -> Int
        if(src_ty->isPointerTy() && is_int_type(target_ty))
            return builder_.CreatePtrToInt(val, target_ty, "casttmp");

        throw std::runtime_error("codegen: unsupported cast");
    }
} //namespace Codegen
