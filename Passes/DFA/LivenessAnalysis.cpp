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
#include <set>

#include <stdint.h>
#include <stdlib.h>
#include <iostream>

#include "231DFA.h"

using namespace llvm;
using namespace std;

class LivenessInfo : public Info {
    private:
        std::set<unsigned> info; // indices of live variables at this moment
    public:
    LivenessInfo() {}
    LivenessInfo(std::set<unsigned> s) {
        info = s;
    }

    /*
     * Print out the information
     */
    void print() {
        for(auto it : info) {
            errs() << (it) << "|";
        }
        errs() << "\n";
    }

    std::set<unsigned> getInfo() {
        return info;
    }

    void setInfo(std::set<unsigned> s) {
        info = s;
    }

    void join(unsigned var) {
        info.insert(var);
    }

    void join(std::set<unsigned> vars) {
        info.insert(vars.begin(), vars.end());
    }

    void remove(unsigned var) {
        info.erase(var);
    }

    void remove(std::set<unsigned> vars) {
        info.erase(vars.begin(), vars.end());
    }

    /*
     * Compare two pieces of information
     */
    static bool equals(LivenessInfo * info1, LivenessInfo * info2) {
        return info1->getInfo() == info2->getInfo();
    }
    /*
     * Join two pieces of information.
     * The third parameter points to the result.
     */
    static LivenessInfo* join(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result) { // union
        std::set<unsigned> infoSet1 = info1->getInfo();
        std::set<unsigned> infoSet2 = info2->getInfo();
        infoSet1.insert(infoSet2.begin(), infoSet2.end());
        result->setInfo(infoSet1);
        return result;
    }

};

class LivenessAnalysis : public DataFlowAnalysis<LivenessInfo, false>  {   // <info, direction>
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        std::set<string> opname2 = {"br", "switch", "store"};
        std::set<string> opname1 = {"alloca", "load", "getelementptr", "icmp", "fcmp", "select"};

        int getCategory(Instruction* I) {
            if(isa<PHINode>(I)) {
                return 3;
            }
            if(I->isBinaryOp())
                return 1;
            string op = I->getOpcodeName();
            if(opname1.count(op) > 0) {
                return 1;
            }
            if(opname2.count(op) > 0) {
                return 2;
            }
            return 2;
        }

        set<unsigned> usedVariables(Instruction* I) {
            set<unsigned> result;
            for(Use &U:I->operands()) {
                Value* V = U.get();
                Instruction* defineInst = dyn_cast<Instruction>(V);
                if(defineInst != nullptr) {
                    result.insert(instr2index(defineInst));
                }
            }
            return result;
        }

        LivenessInfo* getOutInfoPHI(Instruction* I, LivenessInfo* info, BasicBlock* outBB) {
            for(Use &U:I->operands()) {
                Value* V = U.get();
                Instruction* defineInst = dyn_cast<Instruction>(V);
                if(defineInst == nullptr) {
                    continue;
                }
                if(defineInst->getParent() == outBB) {
                    unsigned index = instr2index(defineInst);
                    info->join(index);
                    break;
                }
            }
            return info;
        }

    public:
        LivenessAnalysis(LivenessInfo & bottom, LivenessInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I,
                        std::vector<unsigned> & IncomingEdges,
                        std::vector<unsigned> & OutgoingEdges,
                        std::vector<LivenessInfo *> & Infos) {
            LivenessInfo* info = new LivenessInfo();
            unsigned currentIndex = instr2index(I);
            for(auto it : IncomingEdges) {
                Edge edge = std::make_pair(it, currentIndex);
                LivenessInfo* in = edge2info(edge);
                LivenessInfo::join(info, in, info);
            }

            // errs() << currentIndex << ": ";
            // errs() << (*I) << "\n";
            
            switch(getCategory(I)) {
                case 1: { // First category: IR instructions that return a value (defines a variable) 
                    info->join(usedVariables(I));
                    info->remove(currentIndex);
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        Infos.push_back(info);
                    }
                }
                    break;
                case 2: { // Second categoryL IR instructions that do not return a value
                    info->join(usedVariables(I));
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        Infos.push_back(info);
                    }
                }
                    break;
                case 3: { // Third category: PHI instructions
                    BasicBlock* bb = I->getParent();
                    unsigned firstNonPHI = instr2index(bb->getFirstNonPHI());
                    for(unsigned i = currentIndex; i < firstNonPHI; i++) {
                        info->remove(i);
                    }
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        LivenessInfo* info_i = new LivenessInfo(info->getInfo());
                        for(unsigned index = currentIndex; index < firstNonPHI; index++) {
                            Instruction* PHI_inst = index2instr(index);
                            BasicBlock* outBB = index2instr(OutgoingEdges[i])->getParent();
                            getOutInfoPHI(PHI_inst, info_i, outBB);
                        }
                        Infos.push_back(info_i);
                    }
                }
                    break;
                default: break;
            }
        }
};

namespace {
struct LivenessAnalysisPass : public FunctionPass {
    static char ID;
    LivenessAnalysisPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        LivenessInfo bottom;
        LivenessInfo initialState;
        LivenessAnalysis *la = new LivenessAnalysis(bottom, initialState);
        la->runWorklistAlgorithm(&F);
        la->print();
        return true;
    }
  
}; // end of struct LivenessAnalysisPass
}  // end of anonymous namespace

char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X("cse231-liveness", "Liveness Analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
