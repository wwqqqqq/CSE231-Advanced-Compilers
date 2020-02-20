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
        typedef std::pair<char,unsigned> Identifier;
        std::map<Identifier, std::set<Identifier>> info;
    public:
    MayPointToInfo() {}
    // MayPointToInfo(const MayPointToInfo& other) {
    //     info = other.getInfo();
    // }
    MayPointToInfo(std::map<Identifier, std::set<Identifier>> s) {
        info = s;
    }

    /*
     * Print out the information
     */
    void print() {
        for(auto it : info) {
            errs() << (it.first.first) << (it.first.second) << "->(";
            for(auto it2 : it.second) {
                errs() << (it2.first) << (it2.second) << "/";
            }
            errs() << ")|";
        }
        errs() << "\n";
    }

    std::map<Identifier, std::set<Identifier>> getInfo() {
        return info;
    }

    std::set<Identifier> mayPointTo(Identifier pointer) {
        if(info.count(pointer) == 0) {
            std::set<Identifier> s;
            return s;
        }
        else return info[pointer];
    }

    void setInfo(std::map<Identifier, std::set<Identifier>> s) {
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
        std::map<Identifier, std::set<Identifier>> infoSet1 = info1->getInfo();
        std::map<Identifier, std::set<Identifier>> infoSet2 = info2->getInfo();
        for(auto it : infoSet2) {
            if(infoSet1.count(it.first) == 0) {
                infoSet1.insert(it);
            }
            else {
                std::set<Identifier> s2 = it.second;
                infoSet1[it.first].insert(s2.begin(), s2.end());
            }
        }
        result->setInfo(infoSet1);
        return result;
    }

    static MayPointToInfo* join(MayPointToInfo * info, Identifier pointer, Identifier var, MayPointToInfo * result) {
        std::map<Identifier, std::set<Identifier>> infoMap = info->getInfo();
        if(infoMap.count(pointer) == 0) {
            std::set<Identifier> s;
            s.insert(var);
            infoMap.insert(std::pair<Identifier, std::set<Identifier>>(pointer, s));
        }
        else {
            infoMap[pointer].insert(var);
        }
        result->setInfo(infoMap);
        return result;
    }

    static MayPointToInfo* join(MayPointToInfo * info, Identifier pointer, std::set<Identifier> vars, MayPointToInfo * result) {
        if(vars.size() == 0) {
            result->setInfo(info->getInfo());
            return result;
        }
        std::map<Identifier, std::set<Identifier>> infoMap = info->getInfo();
        if(infoMap.count(pointer) == 0) {
            std::set<Identifier> s;
            s.insert(vars.begin(), vars.end());
            infoMap.insert(std::pair<Identifier,std::set<Identifier>>(pointer, s));
            infoMap[pointer] = s;
        }
        else {
            infoMap[pointer].insert(vars.begin(), vars.end());
        }
        result->setInfo(infoMap);
        return result;
    }
};

class MayPointToAnalysis : public DataFlowAnalysis<MayPointToInfo, true>  {   // <info, direction>
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        typedef std::pair<char,unsigned> Identifier;

        set<unsigned> usedVariables(Instruction* I) {
            set<unsigned> result;
            for(Use &U:I->operands()) {
                Value* V = U.get();
                Instruction* defineInst = dyn_cast<Instruction>(V);
                if(defineInst != nullptr)
                    result.insert(instr2index(defineInst));
            }
            return result;
        }

        Identifier getIdentifier(char type, unsigned index) {
            return std::pair<char,unsigned>(type, index);
        }
    public:
        MayPointToAnalysis(MayPointToInfo & bottom, MayPointToInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I,
                        std::vector<unsigned> & IncomingEdges,
                        std::vector<unsigned> & OutgoingEdges,
                        std::vector<MayPointToInfo *> & Infos) {
            MayPointToInfo* info = new MayPointToInfo();
            unsigned currentIndex = instr2index(I);
            for(auto it : IncomingEdges) {
                Edge edge = std::pair<unsigned, unsigned>(it, currentIndex);
                MayPointToInfo* in = edge2info(edge);
                MayPointToInfo::join(info, in, info);
            }

            // errs() << currentIndex << ": ";
            // errs() << (*I) << "\n";

            std::string opcode = std::string(I->getOpcodeName());
            Identifier currentR = getIdentifier('R', currentIndex);
            Identifier currentM = getIdentifier('M', currentIndex);
            
            if(opcode == "alloca") {
                MayPointToInfo::join(info, currentR, currentM, info);
            }
            else if(opcode == "bitcast") {
                Instruction* Rv = dyn_cast<Instruction>(I->getOperand(0));
                Identifier v = getIdentifier('R', instr2index(Rv));
                if(instr2index(Rv) != 0) {
                    std::set<Identifier> X = info->mayPointTo(v);
                    MayPointToInfo::join(info, currentR, X, info);
                }
            }
            else if(opcode == "getelementptr") {
                Instruction* Rv = dyn_cast<Instruction>(I->getOperand(0));
                Identifier v = getIdentifier('R', instr2index(Rv));
                if(instr2index(Rv) != 0) {
                    std::set<Identifier> X = info->mayPointTo(v);
                    MayPointToInfo::join(info, currentR, X, info);
                }
            }
            else if(opcode == "load") {
                Instruction* Rp = dyn_cast<Instruction>(I->getOperand(0));
                Identifier p = getIdentifier('R', instr2index(Rp));
                if(instr2index(Rp) != 0) {
                    std::set<Identifier> X = info->mayPointTo(p);
                    for(auto it : X) {
                        std::set<Identifier> Y = info->mayPointTo(it);
                        MayPointToInfo::join(info, currentR, Y, info);
                    }
                }
            }
            else if(opcode == "store") {
                Instruction* Rv = dyn_cast<Instruction>(I->getOperand(0));
                Identifier v = getIdentifier('R', instr2index(Rv));
                Instruction* Rp = dyn_cast<Instruction>(I->getOperand(1));
                Identifier p = getIdentifier('R', instr2index(Rp));
                if(instr2index(Rv) != 0 && instr2index(Rp) != 0) {
                    std::set<Identifier> X = info->mayPointTo(v);
                    std::set<Identifier> Y = info->mayPointTo(p);
                    for(auto it : Y) {
                        MayPointToInfo::join(info, it, X, info);
                    }
                }
            }
            else if(opcode == "select") {
                // operand is 1/2
                Instruction* I1 = dyn_cast<Instruction>(I->getOperand(1));
                unsigned R1 = instr2index(I1);
                if(R1 != 0) {
                    std::set<Identifier> X = info->mayPointTo(getIdentifier('R', R1));
                    MayPointToInfo::join(info, currentR, X, info);
                }
                Instruction* I2 = dyn_cast<Instruction>(I->getOperand(2));
                unsigned R2 = instr2index(I2); //
                if(R2 == 0) {
                    std::set<Identifier> X = info->mayPointTo(getIdentifier('R', R2));
                    MayPointToInfo::join(info, currentR, X, info);
                }
            }
            else if(opcode == "phi") {
                BasicBlock* bb = I->getParent();
                unsigned firstNonPHI = instr2index(bb->getFirstNonPHI());
                for(unsigned i = currentIndex; i < firstNonPHI; i++) {
                    Instruction* PhiInst = index2instr(i);
                    std::set<unsigned> R = usedVariables(PhiInst);
                    for(auto it : R) {
                        MayPointToInfo::join(info, currentR, info->mayPointTo(getIdentifier('R', it)), info);
                    }
                }
            }
            // else {
            //     // out = in
            // }

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
        MayPointToAnalysis *mpt = new MayPointToAnalysis(bottom, initialState);
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
