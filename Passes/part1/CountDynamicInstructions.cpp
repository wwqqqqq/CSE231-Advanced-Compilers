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
struct CDI : public FunctionPass {
    static char ID;
    CDI() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        Module* Mod = F.getParent();
        LLVMContext& context = F.getContext();
        // void updateInstrInfo(unsigned num, uint32_t * keys, uint32_t * values)
        FunctionCallee updateFunction = Mod->getOrInsertFunction("updateInstrInfo",
                                    Type::getVoidTy(context), // return type
                                    Type::getInt32Ty(context), // first parameter type unsigned
                                    Type::getInt32PtrTy(context), // second parameter type pointer to uint32_t
                                    Type::getInt32PtrTy(context)  // third parameter type pointer to uint32_t
        );
        // void printOutInstrInfo()
        FunctionCallee printFunction = Mod->getOrInsertFunction("printOutInstrInfo",
                                    Type::getVoidTy(context)  // return type
                                    // no parameter
        );

        for (BasicBlock &BB : F) {
            // void updateInstrInfo(unsigned num, uint32_t * keys, uint32_t * values)
            std::map<uint32_t, uint32_t> instr_map;
            for (Instruction &I : BB) {
                uint32_t key = I.getOpcode();
                if (instr_map.count(key) == 0)
                    instr_map.insert(std::pair<uint32_t, uint32_t>(key, 1));
                else
                    instr_map[key] = instr_map[key] + 1;
            }
            ConstantInt* num = ConstantInt::get(Type::getInt32Ty(context),instr_map.size());
            vector<uint32_t> keys;
            vector<uint32_t> values;
            for(std::map<uint32_t, uint32_t>::iterator it=instr_map.begin(); it != instr_map.end(); it++) {
                keys.push_back(it->first);
                values.push_back(it->second);
            }
            ArrayType* arrayTy = ArrayType::get(Type::getInt32Ty(context), instr_map.size());
            // llvm::ArrayRef< T >::operator std::vector< T >	(		)	const
            // ArrayRef<uint32_t> keys_arrayref = keys
            // ArrayRef<uint32_t> values_arrayref = values
            // global args of function updateInstrInfo
            GlobalVariable* keys_global = new GlobalVariable(*Mod, 
                                                            arrayTy, 
                                                            true, 
                                                            GlobalValue::InternalLinkage, 
                                                            ConstantDataArray::get(context, keys), 
                                                            "keys global");
            GlobalVariable* values_global = new GlobalVariable(*Mod, 
                                                            arrayTy, 
                                                            true, 
                                                            GlobalValue::InternalLinkage, 
                                                            ConstantDataArray::get(context, values), 
                                                            "values global");

            // generate arg list
            // ArrayRef<Value*>
            IRBuilder<> Builder(&*BB.getFirstInsertionPt());
            vector<Value*> args;
            args.push_back(num);

            Value* idx_list[2] = {ConstantInt::get(Type::getInt32Ty(context), 0), ConstantInt::get(Type::getInt32Ty(context), 0)};
            args.push_back(Builder.CreateInBoundsGEP(keys_global, idx_list));
            args.push_back(Builder.CreateInBoundsGEP(values_global, idx_list));

            errs() << "Assemble the arg list" << '\n';

            Builder.CreateCall(updateFunction, args);

            errs() << "Create call at the first insertion point" << '\n';

            for (Instruction &I : BB) {
                if((string)I.getOpcodeName() == "ret") {
                    errs() << "Find the ret instruction" << '\n';
                    Builder.SetInsertPoint(&I);
                    Builder.CreateCall(printFunction);
                }
            }

        }

        return true;
    }
  
}; // end of struct CDI
}  // end of anonymous namespace

char CDI::ID = 0;
static RegisterPass<CDI> X("cse231-cdi", "Collecting Dynamic Instruction Counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
