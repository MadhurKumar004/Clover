#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>


#include "../passes.h"

namespace Codegen {


llvm::PreservedAnalyses DcePass::run(llvm::Function &F,
                                     llvm::FunctionAnalysisManager &FAM) {

    bool changed = true;
    while(changed){
        changed = false;
        //a vector, made for small input values for performance
        llvm::SmallVector<llvm::AllocaInst*, 16> dead_allocas;

        // iterate on each block of function and then each Instruction
        // of that block
        for (llvm::BasicBlock &BB : F) {
            for (llvm::Instruction &I : BB) {
                //checks if the instruction is AllocaInst
                auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(&I);
                if (!alloca) continue;

                // check if any use of this alloca is a load
                bool has_load = false;
                for (llvm::User *U : alloca->users()) {
                    //if the user is load, then we cant remove this instruction
                    if (llvm::isa<llvm::LoadInst>(U)) {
                        has_load = true;
                        break;
                    }
                }

                if (!has_load)
                    // push the instruction which dont have any loadinst
                    dead_allocas.push_back(alloca);
            }
        }

        // if nothing in dead_alloca means nothing to remove
        if (dead_allocas.empty()) return llvm::PreservedAnalyses::all();

        //iterate on all dead_alloca
        for(llvm::AllocaInst *alloca : dead_allocas){
            llvm::SmallVector<llvm::Instruction*, 8> to_delete;
            for(llvm::User *U : alloca->users())
                //check all the users of alloca (other then load)
                to_delete.push_back(llvm::cast<llvm::Instruction>(U));

            //iterate over the all the alloca users and delete them from parents
            for(llvm::Instruction *I : to_delete)
                I->eraseFromParent();

            //at last delete that alloca itself
            alloca->eraseFromParent();
            changed = true;
        }

        llvm::SmallVector<llvm::Instruction* , 16> worklist;
        for (llvm::BasicBlock &BB : F) {
            for (llvm::Instruction &I : BB) {
                if(I.isTerminator()) continue;
                if(I.mayHaveSideEffects()) continue;
                if(I.getType()->isVoidTy()) continue;
                if(I.use_empty())
                    worklist.push_back(&I);
            }
        }

        while(!worklist.empty()){
            llvm::Instruction* I = worklist.pop_back_val();

            //these checks are mainly for the instruction that
            // are going to be inserted in this while loop
            if(!I->use_empty()) continue;
            if(I->isTerminator()) continue;
            if(I->mayHaveSideEffects()) continue;
            if(I->getType()->isVoidTy()) continue;

            //check for operands in a statement
            for(llvm::Use &U : I->operands()){
                //check if that operand is an instruction
                if(auto *op = llvm::dyn_cast<llvm::Instruction>(U)){
                    //if these is only one use(the current line)
                    if(op->hasOneUse())
                        //then we push it for deletion
                        worklist.push_back(op);
                }
            }

            for(unsigned i = 0, e = I->getNumOperands(); i != e; ++i)
                I->setOperand(i, nullptr);

            I->eraseFromParent();
            changed = true;
        }
    }

    llvm::PreservedAnalyses PA;
    PA.preserveSet<llvm::CFGAnalyses>();
    return PA;


}

} // namespace Codegen
