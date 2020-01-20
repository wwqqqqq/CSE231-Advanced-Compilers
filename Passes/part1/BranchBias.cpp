#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <string>
#include <iterator>

#include <stdint.h>
#include <stdlib.h>

using namespace llvm;
using namespace std;

namespace {
struct BB : public FunctionPass {
    static char ID;
    BB() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        Module* Mod = F.getParent();
        LLVMContext& context = F.getContext();
        // void updateBranchInfo(bool taken)
        FunctionCallee updateFunction = Mod->getOrInsertFunction("updateBranchInfo",
                                    Type::getVoidTy(context), // return type
                                    Type::getInt1Ty(context) // first parameter type bool
        );
        // void printOutBranchInfo()
        FunctionCallee printFunction = Mod->getOrInsertFunction("printOutBranchInfo",
                                    Type::getVoidTy(context)  // return type
                                    // no parameter
        );

        for (BasicBlock &BB : F) {
            // generate arg list
            IRBuilder<> Builder(&BB);

            for (Instruction &I : BB) {
                if(BranchInst::classof(&I)) {
                    errs() << "Find a branch instruction" << '\n';
                    errs() << I << '\n';
                    BranchInst* brInstr = cast<BranchInst>(&I);// BranchInst::Create(&BB, &I);
                    bool cond = brInstr->isConditional();
                    if(cond == true)
                        errs() << "isConditional" << '\n';
                    else if(cond == false)
                        errs() << "unconditional" << '\n';
                    if(brInstr->isConditional()) {
                        // only consider conditional branch
                        Value* cond = brInstr->getCondition();
                        errs() << (*cond) << '\n';
                        vector<Value*> args;
                        args.push_back(cond);
                        Builder.SetInsertPoint(&I);
                        Builder.CreateCall(updateFunction, args);
                    }
                }
                if((string)I.getOpcodeName() == "ret") {
                    errs() << "Find the ret instruction" << '\n';
                    Builder.SetInsertPoint(&I);
                    Builder.CreateCall(printFunction);
                }
            }

        }

        return true;
    }
  
}; // end of struct BB
}  // end of anonymous namespace

char BB::ID = 0;
static RegisterPass<BB> X("cse231-bb", "Collecting Dynamic Instruction Counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
