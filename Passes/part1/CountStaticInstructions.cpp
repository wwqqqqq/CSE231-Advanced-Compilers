#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <string>
#include <iterator>

#include <stdint.h>
#include <stdlib.h>

using namespace llvm;
using namespace std;

namespace {
struct CSI : public FunctionPass {
  static char ID;
  CSI() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
        std::map<std::string, unsigned> instr_map;
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                std::string key(I.getOpcodeName());
                if (instr_map.count(key) == 0)
                    instr_map.insert(std::pair<std::string, unsigned>(key, 1));
                else
                    instr_map[key] = instr_map[key] + 1;
            }
        }

        for (std::map<std::string, unsigned>::iterator it=instr_map.begin(); it!=instr_map.end(); ++it)
            errs() << it->first << '\t' << it->second << '\n';

        instr_map.clear();
        return true;
  }
  
}; // end of struct CSI
}  // end of anonymous namespace

char CSI::ID = 0;
static RegisterPass<CSI> X("cse231-csi", "Collecting Static Instruction Counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
