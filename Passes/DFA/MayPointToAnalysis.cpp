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

class MayPointToInfo : public Info {
    private:
        std::map<unsigned, set<unsigned>> info;
    public:
    MayPointToInfo() {}
    // MayPointToInfo(const MayPointToInfo& other) {
    //     info = other.getInfo();
    // }
    MayPointToInfo(std::set<unsigned> s) {
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
    static bool equals(MayPointToInfo * info1, MayPointToInfo * info2) {
        return info1->getInfo() == info2->getInfo();
    }
    /*
     * Join two pieces of information.
     * The third parameter points to the result.
     */
    static MayPointToInfo* join(MayPointToInfo * info1, MayPointToInfo * info2, MayPointToInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        std::set<unsigned> infoSet2 = info2->getInfo();
        infoSet1.insert(infoSet2.begin(), infoSet2.end());
        result->setInfo(infoSet1);
        return result;
    }

    static MayPointToInfo* join(MayPointToInfo * info1, unsigned info2, MayPointToInfo * result) {
        std::set<unsigned> infoSet1 = info1->getInfo();
        infoSet1.insert(info2);
        result->setInfo(infoSet1);
        return result;
    }
};

class MayPointToAnalysis : public DataFlowAnalysis<MayPointToInfo, true>  {   // <info, direction>
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
        MayPointToAnalysis(MayPointToInfo & bottom, MayPointToInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I,
                        std::vector<unsigned> & IncomingEdges,
                        std::vector<unsigned> & OutgoingEdges,
                        std::vector<MayPointToInfo *> & Infos) {
                        // in the function declaration in DataFlowAnalysis, Info refers to the Info parameter in template
            MayPointToInfo* info = new MayPointToInfo();
            unsigned currentIndex = instr2index(I);
            for(auto it : IncomingEdges) {
                Edge edge = std::make_pair(it, currentIndex);
                MayPointToInfo* in = edge2info(edge);
                MayPointToInfo::join(info, in, info);
            }
            
            switch(getCategory(I)) {
                case 1: { // First category: IR instructions that return a value (defines a variable) 
                    MayPointToInfo::join(info, currentIndex, info);
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
                    MayPointToInfo::join(info, new MayPointToInfo(phiSet), info);
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
struct MayPointToAnalaysisPass : public FunctionPass {
    static char ID;
    MayPointToAnalaysisPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        MayPointToInfo bottom;
        MayPointToInfo initialState;
        MayPointToAnalaysis *mpt = new MayPointToAnalaysis(bottom, initialState);
        mpt->runWorklistAlgorithm(&F);
        mpt->print();
        return true;
    }
  
}; // end of struct MayPointToAnalaysisPass
}  // end of anonymous namespace

char MayPointToAnalaysisPass::ID = 0;
static RegisterPass<MayPointToAnalaysisPass> X("cse231-maypointto", "May-Point-To Analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
