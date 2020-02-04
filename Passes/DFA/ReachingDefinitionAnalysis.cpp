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

class ReachingInfo : public Info {
    private:
        std::set<unsigned> info;
    public:
    ReachingInfo() {}
    // ReachingInfo(const ReachingInfo& other) {
    //     info = other.getInfo();
    // }
    ReachingInfo(std::set<unsigned> s) {
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
    static bool equals(ReachingInfo * info1, ReachingInfo * info2) {
        return info1->getInfo() == info2->getInfo();
    }
    /*
     * Join two pieces of information.
     * The third parameter points to the result.
     */
    static ReachingInfo* join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        std::set<unsigned> infoSet2 = info2->getInfo();
        infoSet1.insert(infoSet2.begin(), infoSet2.end());
        result->setInfo(infoSet1);
        return result;
    }

    static ReachingInfo* join(ReachingInfo * info1, unsigned info2, ReachingInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        infoSet1.insert(info2);
        result->setInfo(infoSet1);
        return result;
    }
};

class ReachingDefinitionAnalysis : public DataFlowAnalysis<ReachingInfo, true>  {   // <info, direction>
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
    public:
        ReachingDefinitionAnalysis(ReachingInfo & bottom, ReachingInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I,
                        std::vector<unsigned> & IncomingEdges,
                        std::vector<unsigned> & OutgoingEdges,
                        std::vector<ReachingInfo *> & Infos) {
                        // in the function declaration in DataFlowAnalysis, Info refers to the Info parameter in template
            ReachingInfo* info = new ReachingInfo();
            unsigned currentIndex = instr2index(I);
            for(auto it : IncomingEdges) {
                Edge edge = std::make_pair(it, currentIndex);
                ReachingInfo* in = edge2info(edge);
                ReachingInfo::join(info, in, info);
            }
            
            switch(getCategory(I)) {
                case 1: { // First category: IR instructions that return a value (defines a variable) 
                    ReachingInfo::join(info, currentIndex, info);
                }
                    break;
                case 2: // Second categoryL IR instructions that do not return a value
                    break;
                case 3: { // Third category: PHI instructions
                    std::set<unsigned> phiSet;
                    BasicBlock* bb = I->getParent();
                    unsigned firstNonPHI = instr2index(bb->getFirstNonPHI());
                    for(unsigned i = currentIndex; i < firstNonPHI; i++) {
                        phiSet.insert(i);
                    }
                    ReachingInfo::join(info, new ReachingInfo(phiSet), info);
                }
                    break;
                default: break;
            }

            for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                Infos.push_back(info);
            }
        }
};

namespace {
struct ReachingDefinitionAnalysisPass : public FunctionPass {
    static char ID;
    ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        ReachingInfo bottom;
        ReachingInfo initialState;
        ReachingDefinitionAnalysis *rda = new ReachingDefinitionAnalysis(bottom, initialState);
        rda->runWorklistAlgorithm(&F);
        rda->print();
        return true;
    }
  
}; // end of struct ReachingDefinitionAnalysisPass
}  // end of anonymous namespace

char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X("cse231-reaching", "Reaching Definition Analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
