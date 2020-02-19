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

    static LivenessInfo* join(LivenessInfo * info1, set<unsigned> &info2, LivenessInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        infoSet1.insert(info2.begin(), info2.end());
        result->setInfo(infoSet1);
        return result;
    }

    static LivenessInfo* join(LivenessInfo * info1, unsigned info2, LivenessInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        infoSet1.insert(info2);
        result->setInfo(infoSet1);
        return result;
    }

    static LivenessInfo* remove(LivenessInfo * info1, unsigned info2, LivenessInfo * result) {
        std::set<unsigned> infoSet = info1->getInfo();
        infoSet.erase(info2);
        result->setInfo(infoSet);
        return result;
    }

    static LivenessInfo* remove(LivenessInfo * info1, set<unsigned> &info2, LivenessInfo * result) {
        std::set<unsigned> infoSet = info1->getInfo();
        infoSet.erase(info2.begin(), info2.end());
        result->setInfo(infoSet);
        return result;
    }
};

class LivenessAnalysis : public DataFlowAnalysis<LivenessInfo, false>  {   // <info, direction>
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        std::set<string> opname2 = {"br", "switch", "store"};

        int getCategory(Instruction* I) {
            if(isa<PHINode>(I)) {
                return 3;
            }
            string op = I->getOpcodeName();
            if(opname2.count(op) > 0) {
                return 2;
            }
            return 1;
        }

        set<unsigned> usedVariables(Instruction* I) {
            set<unsigned> result;
            for(Use &U:I->operands()) {
                Value* V = U.get();
                Instruction* defineInst = dyn_cast<Instruction>(V);
                result.insert(instr2index(defineInst));
            }
            return result;
        }

        unsigned definedVariable(Instruction* I) {
            return instr2index(I);
        }

        LivenessInfo* getOutInfoPHI(Instruction* I, LivenessInfo* in, LivenessInfo* out, BasicBlock* outBB) {
            for(Use &U:I->operands()) {
                Value* V = U.get();
                Instruction* defineInst = dyn_cast<Instruction>(V);
                if(defineInst->getParent() == outBB) {
                    unsigned index = instr2index(defineInst);
                    LivenessInfo::join(in, index, out);
                    break;
                }
            }
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
            
            switch(getCategory(I)) {
                case 1: { // First category: IR instructions that return a value (defines a variable) 
                    LivenessInfo::join(info, usedVariables(I), info);
                    LivenessInfo::remove(info, definedVariable(I), info);
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        Infos.push_back(info);
                    }
                }
                    break;
                case 2: { // Second categoryL IR instructions that do not return a value
                    LivenessInfo::join(info, usedVariables(I), info);
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        Infos.push_back(info);
                    }
                }
                    break;
                case 3: { // Third category: PHI instructions
                    std::set<unsigned> definedVars;
                    BasicBlock* bb = I->getParent();
                    unsigned firstNonPHI = instr2index(bb->getFirstNonPHI());
                    for(unsigned i = currentIndex; i < firstNonPHI; i++) {
                        definedVars.insert(i);
                    }
                    LivenessInfo::remove(info, definedVars, info);
                    for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                        LivenessInfo* info_i = new LivenessInfo(info.getInfo());
                        for(unsigned index = currentIndex; index < firstNonPHI; index++) {
                            Instruction* PHI_inst = index2instr(index);
                            BasicBlock* outBB = index2instr(OutgoingEdges[i])->getParent();
                            getOutInfoPHI(PHI_inst, info_i, info_i, outBB);
                        }
                        Infos.push_back(info);
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
