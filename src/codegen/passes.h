#ifndef PASSES_H
#define PASSES_H

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instruction.h>

namespace Codegen{

    struct DcePass : llvm::PassInfoMixin<DcePass> {
        llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
        static bool is_dead(llvm::Instruction& I);
    };

} // namespace Codegen

#endif // PASSES_H
