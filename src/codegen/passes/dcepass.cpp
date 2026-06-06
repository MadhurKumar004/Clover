#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/Local.h>


#include "../passes.h"

namespace Codegen {


llvm::PreservedAnalyses DcePass::run(llvm::Function &F,
                                     llvm::FunctionAnalysisManager &FAM) {

    bool changed = true;
    while (changed) {
        changed = false;

        // Collect allocas with no loads (candidates for removal)
        llvm::SmallVector<llvm::AllocaInst *, 16> dead_allocas;
        for (llvm::BasicBlock &BB : F) {
            for (llvm::Instruction &I : BB) {
                if (auto *AI = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                    bool hasLoad = false;
                    for (llvm::User *U : AI->users()) {
                        if (llvm::isa<llvm::LoadInst>(U)) {
                            hasLoad = true;
                            break;
                        }
                    }
                    if (!hasLoad)
                        dead_allocas.push_back(AI);
                }
            }
        }

        if (dead_allocas.empty())
            return llvm::PreservedAnalyses::all();

        for (llvm::AllocaInst *AI : dead_allocas) {
            // If all uses of the alloca are stores (possibly through GEP/bitcast),
            // then the stores are dead and can be removed along with the alloca.
            llvm::SmallVector<llvm::Instruction*, 16> to_delete;
            llvm::SmallVector<llvm::User*, 16> worklist;
            for (llvm::User *U : AI->users()) worklist.push_back(U);

            bool has_load_or_effect = false;
            while (!worklist.empty() && !has_load_or_effect) {
                llvm::User *U = worklist.pop_back_val();

                if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(U)) {
                    has_load_or_effect = true;
                    break;
                }

                if (auto *SI = llvm::dyn_cast<llvm::StoreInst>(U)) {
                    if (SI->isVolatile()) { has_load_or_effect = true; break; }
                    to_delete.push_back(SI);
                    continue;
                }

                if (auto *I = llvm::dyn_cast<llvm::Instruction>(U)) {
                    // intermediate instruction (gep/bitcast/etc). We need to
                    // examine its users transitively.
                    to_delete.push_back(I);
                    for (llvm::User *UU : I->users())
                        worklist.push_back(UU);
                    continue;
                }

                // Conservatively bail out for non-instruction users
                has_load_or_effect = true;
                break;
            }

            if (has_load_or_effect)
                continue;

            // Erase collected instructions safely (reverse to respect deps)
            for (auto *I : llvm::reverse(to_delete)) {
                I->dropAllReferences();
                I->eraseFromParent();
            }

            if (AI->use_empty()) {
                AI->eraseFromParent();
                changed = true;
            }
        }

        //Remove other trivially-dead instructions
        llvm::SmallVector<llvm::Instruction *, 16> worklist;
        for (llvm::BasicBlock &BB : F)
            for (llvm::Instruction &I : BB)
                if (!I.isTerminator() && !I.mayHaveSideEffects() && !I.getType()->isVoidTy() && I.use_empty())
                    worklist.push_back(&I);

        while (!worklist.empty()) {
            llvm::Instruction *I = worklist.pop_back_val();
            if (llvm::isInstructionTriviallyDead(I)) {
                llvm::RecursivelyDeleteTriviallyDeadInstructions(I);
                changed = true;
            }
        }
    }

    return llvm::PreservedAnalyses::none();

}

} // namespace Codegen
