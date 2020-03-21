#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/Utils/FunctionComparator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APFloat.h"



#include <map>
#include <string>
#include <iterator>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include <stdint.h>
#include <stdlib.h>
#include <iostream>

#include "231DFA.h"

using namespace llvm;
using namespace std;

enum ConstState { Bottom, Const, Top };

typedef struct {
    ConstState state;
    Constant* value;
} ConstInfo;

std::set<Value*> MPT;
std::unordered_map<Function*, std::set<GlobalVariable*>> MOD; // union of LMOD and CMOD
std::unordered_set<Value*> GlobVars;

// helper functions

bool isSameConstant(Constant* c1, Constant* c2) {
    // FunctionComparator fc;
    // if(fc.cmpConstants(c1, c2) == 0)
    //     return true;
    // return false;
    ConstantInt* ci1 = dyn_cast<ConstantInt>(c1);
    ConstantInt* ci2 = dyn_cast<ConstantInt>(c2);
    if(ci1 && ci2) {
        return ci1->getSExtValue() == ci2->getSExtValue();
    }
    ConstantFP* cf1 = dyn_cast<ConstantFP>(c1);
    ConstantFP* cf2 = dyn_cast<ConstantFP>(c2);
    if(cf1 && cf2) {
        return cf1->getValueAPF().convertToFloat() == cf2->getValueAPF().convertToFloat();
    }
    return false;
}

void printConstant(Constant* c) {
    // string ty;
    // c->getType()->print(&raw_string_ostream(ty));
    // ConstantInt* ci = dyn_cast<ConstantInt>(c);
    // if(ci) {
    //     return ty + " " + to_string(ci->getSExtValue());
    // }
    // ConstantFP* cf = dyn_cast<ConstantFP>(c);
    // if(cf) {
    //     if(c->getType()->isFloatTy()) {
    //         return ty + " " + to_string(cf->getValueAPF().convertToFloat());
    //     }
    //     if(c->getType()->isDoubleTy()) {
    //         return ty + " " + to_string(cf->getValueAPF().convertToDouble());
    //     }
    // }
    // return ty;
    errs() << (*c);
}

string getValueName(Value* v) {
    StringRef sr = v->getName();
    const char* name = sr.data();
    if(name == NULL)
        return "-";
    std::string str = std::string(name);
    return str;
}

string getValueName(GlobalVariable* gv) {
    Value* v = dyn_cast<Value>(gv);
    return getValueName(v);
}

void printValue(Value* v) {
    // errs() << (*v);
    errs() << getValueName(v);
}

void printValue(GlobalVariable* gv) {
    Value* v = dyn_cast<Value>(gv);
    printValue(v);
}

class ConstPropInfo : public Info {
    private:
        std::map<Value*, ConstInfo> ConstPropContent;

    public:
    ConstPropInfo() {}
    ConstPropInfo(string s) {
        ConstInfo info;
        info.state = (s == "bottom")?Bottom : Top;
        info.value = NULL;
        for(auto &v : GlobVars) {
            ConstPropContent.insert(pair<Value*, ConstInfo>(v, info));
        }
    }

    ConstPropInfo(std::map<Value*, ConstInfo> s) {
        ConstPropContent = s;
    }

    /*
     * Print out the information
     */
    void print() {
        string bottom = "⊥";
        string top = "⊤";
        // char bottom = 'B';
        // char top = 'T';
        for(auto it : ConstPropContent) {
            if(!GlobalVariable::classof(it.first))
                continue;
            printValue(it.first);
            errs() << "="; // << (it.second) << "|";
            if(it.second.state == Bottom) {
                errs() << bottom;
            }
            else if(it.second.state == Const) {
                printConstant(it.second.value);
            }
            else {
                errs() << top;
            }
            errs() << "|";
        }
        errs() << "\n";
    }

    std::map<Value*, ConstInfo> getInfo() {
        return ConstPropContent;
    }

    ConstInfo getConstInfo(Value* v) {
        if(ConstPropContent.count(v) == 0) {
            ConstInfo info;
            info.state = Top;
            info.value = NULL;
            return info;
        }
        else return ConstPropContent[v];
    }

    void setInfo(std::map<Value*, ConstInfo> s) {
        ConstPropContent = s;
    }

    bool isConstant(pair<Value*, ConstInfo> it) {
        if(it.second.state == Const)
            return true;
        return false;
    } 

    /*
     * Compare two pieces of information
     */
    static bool equals(ConstPropInfo * info1, ConstPropInfo * info2) {
        // return info1->getInfo() == info2->getInfo();
        std::map<Value*, ConstInfo> infoSet1 = info1->getInfo();
        std::map<Value*, ConstInfo> infoSet2 = info2->getInfo();
        if(infoSet1.size() != infoSet2.size())
            return false;
        for(auto it : infoSet2) {
            if(infoSet1.count(it.first) == 0) {
                return false;
            }
            ConstInfo cf1 = infoSet1[it.first];
            ConstInfo cf2 = it.second;
            if(cf1.state != cf2.state)
                return false;
            if(cf1.state == Const) {
                if(!isSameConstant(cf1.value, cf2.value))
                    return false;
            }
        }
        return true;
    }
    /*
     * Join two pieces of information.
     * The third parameter points to the result.
     */
    static ConstPropInfo* join(ConstPropInfo * info1, ConstPropInfo * info2, ConstPropInfo * result) {
        // intersection
        std::map<Value*, ConstInfo> infoSet1 = info1->getInfo();
        std::map<Value*, ConstInfo> infoSet2 = info2->getInfo();
        for(auto it : infoSet2) {
            if(infoSet1.count(it.first) == 0) {
                // continue;
                infoSet1.insert(it);
            }
            else if(it.second.state == Bottom) {
                continue;
            }
            else if(infoSet1[it.first].state == Bottom) {
                infoSet1[it.first] = it.second;
            }
            else if(infoSet1[it.first].state == Top || it.second.state == Top) {
                infoSet1[it.first].state = Top;
            }
            else {
                // they are both Constant
                // errs() << "******TWO CONSTANT******\n";
                // errs() << *(it.first) << "\n";
                if(isSameConstant(infoSet1[it.first].value, it.second.value)) {
                    continue;
                }
                else {
                    // errs() << "different: ";
                    // printConstant(infoSet1[it.first].value);
                    // errs() << ", ";
                    // printConstant(it.second.value);
                    // errs() << "\n";
                    infoSet1[it.first].state = Top;
                }
            }
        }
        result->setInfo(infoSet1);
        return result;
    }

    void join(ConstPropInfo * info2) {
        // intersection
        std::map<Value*, ConstInfo> infoSet2 = info2->getInfo();
        for(auto it : infoSet2) {
            if(ConstPropContent.count(it.first) == 0) {
                continue;
            }
            else if(it.second.state == Bottom) {
                continue;
            }
            else if(ConstPropContent[it.first].state == Bottom) {
                ConstPropContent[it.first] = it.second;
            }
            else if(ConstPropContent[it.first].state == Top || it.second.state == Top) {
                ConstPropContent[it.first].state = Top;
            }
            else {
                // they are both Constant
                if(isSameConstant(ConstPropContent[it.first].value, it.second.value)) {
                    continue;
                }
                else {
                    ConstPropContent[it.first].state = Top;
                }
            }
        }
    }

    void setTop(Value* v) {
        ConstInfo info;
        info.state = Top;
        info.value = NULL;
        if(ConstPropContent.count(v) == 0) {
            ConstPropContent.insert(pair<Value*, ConstInfo>(v, info));
        }
        else {
            ConstPropContent[v] = info;
        }
        // errs() << "result set top\n";
    }

    void setConstant(Value* v, Constant* c) {
        ConstInfo info;
        info.state = Const;
        info.value = c;
        if(ConstPropContent.count(v) == 0) {
            ConstPropContent.insert(pair<Value*, ConstInfo>(v, info));
        }
        else {
            ConstPropContent[v] = info;
        }
        // errs() << "result set constant\n";
    }

    void setInfo(Value* v, ConstInfo info) {
        if(ConstPropContent.count(v) == 0) {
            ConstPropContent.insert(pair<Value*, ConstInfo>(v, info));
        }
        else {
            ConstPropContent[v] = info;
        }
    }
    
};

class ConstPropAnalysis : public DataFlowAnalysis<ConstPropInfo, true>  {   // <info, direction>
    private:
        typedef std::pair<unsigned, unsigned> Edge;

        set<Value*> usedVariables(Instruction* I) {
            set<Value*> result;
            for(Use &U:I->operands()) {
                Value* V = U.get();
                result.insert(V);
            }
            return result;
        }

        ConstState getConstantState(Value* v, ConstPropInfo *info) {
            ConstInfo constInfo = info->getConstInfo(v);
            return constInfo.state;
        }

        Constant* getConstValue(Value* v, ConstPropInfo *info) {
            // if v is not constant, return NULL
            if(!GlobalVariable::classof(v) && Constant::classof(v)) {
                // errs() << "is constant by class\n";
                return dyn_cast<Constant>(v);
            }
            ConstInfo constInfo = info->getConstInfo(v);
            if(constInfo.state == Const) {
                // errs() << "is constant by info\n";
                return constInfo.value;
            }

            // errs() << "not constant..\n";
            return NULL;
        }

        bool isGlobalVariable(Value* v) {
            if(GlobalVariable::classof(v)) {
                return true;
            }
            return false;
        }
    public:
        ConstPropAnalysis(ConstPropInfo & bottom, ConstPropInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I,
                        std::vector<unsigned> & IncomingEdges,
                        std::vector<unsigned> & OutgoingEdges,
                        std::vector<ConstPropInfo *> & Infos) {
            ConstPropInfo* info = new ConstPropInfo();
            unsigned currentIndex = instr2index(I);
            for(auto it : IncomingEdges) {
                Edge edge = std::pair<unsigned, unsigned>(it, currentIndex);
                ConstPropInfo* in = edge2info(edge);
                ConstPropInfo::join(info, in, info);
            }

            // errs() << currentIndex << ": ";
            // errs() << (*I) << "\n";

            std::string opcode = std::string(I->getOpcodeName());

            Value* v = dyn_cast<Value>(I);
            ConstantFolder FOLDER;


            // errs() << "current constant value in map before flow function:\n";
            // for(auto it : info->getInfo()) {
            //     if(it.second.state == Const) {
            //         errs() << (*(it.first)) << "\n";
            //     }
            // }
            
            if(I->isBinaryOp()) {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL || c1 == nullptr || c2 == nullptr) {
                    info->setTop(v);
                }
                else {
                    BinaryOperator* bin_op = dyn_cast<BinaryOperator>(I);
                    if(bin_op == NULL || bin_op == nullptr) {
                        info->setTop(v);
                    }
                    Constant* value = FOLDER.CreateBinOp(bin_op->getOpcode(), c1, c2);
                    info->setConstant(v, value);
                }
            }
            else if(I->isUnaryOp()) {
                Value* op = I->getOperand(0);
                Constant* c = getConstValue(op, info);
                if(c == NULL) {
                    info->setTop(v);
                }
                else {
                    UnaryOperator* un_op = dyn_cast<UnaryOperator>(I);
                    Constant* value = FOLDER.CreateUnOp(un_op->getOpcode(), c);
                    info->setConstant(v, value);
                }
            }
            else if(opcode == "load") {
                // LoadInst* load_inst = dyn_cast<LoadInst>(I);
                Value* op = I->getOperand(0);
                if(!I->getType()->isPointerTy()) {
                    Constant* c = getConstValue(op, info);
                    if(c == NULL)
                        info->setTop(v);
                    else
                        info->setConstant(v, c);
                }
                else {
                    info->setTop(v);
                }
            }
            else if(opcode == "store") {
                Value* v1 = I->getOperand(0);
                Value* v2 = I->getOperand(1);
                if(!v1->getType()->isPointerTy()) {
                    Constant* c = getConstValue(v1, info);
                    // if(c == NULL) {
                    //     errs() << "NULL\n";
                    // }
                    // else errs() << (*c) << "\n";
                    if(c == NULL) {
                        // errs() << "set top\n";
                        info->setTop(v2);
                    }
                    else {
                        // errs() << "set constant: ";
                        // errs() << (*v2) << "\n";
                        info->setConstant(v2, c);
                    }
                }
                else {
                    info->setTop(v2);
                }

                Value* p = I->getOperand(1);
                Instruction* defineInst = dyn_cast<Instruction>(p);
                if(defineInst != NULL && defineInst != nullptr) {
                    std::string opcode2 = std::string(defineInst->getOpcodeName());
                    if(opcode2 == "load" && p->getType()->isPointerTy()) {
                        for(auto glob: MPT) {
                            info->setTop(glob);
                        }
                    }
                }
            }
            else if(opcode == "call") {
                // set all variables in the callee's MOD set to top
                info->setTop(v); // if v is a return value of call, we may assume it is not constant
                CallInst* call_inst = dyn_cast<CallInst>(I);
                for(auto glob: MOD[call_inst->getCalledFunction()]) {
                    info->setTop(dyn_cast<Value>(glob));
                }
            }
            else if(opcode == "icmp") {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL) {
                    info->setTop(v);
                }
                else {
                    CmpInst* cmp = dyn_cast<CmpInst>(I);
                    CmpInst::Predicate pred = cmp->getPredicate();
                    Constant* value = FOLDER.CreateICmp(pred, c1, c2);
                    info->setConstant(v, value);
                }
            }
            else if(opcode == "fcmp") {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL) {
                    info->setTop(v);
                }
                else {
                    CmpInst* cmp = dyn_cast<CmpInst>(I);
                    CmpInst::Predicate pred = cmp->getPredicate();
                    Constant* value = FOLDER.CreateFCmp(pred, c1, c2);
                    info->setConstant(v, value);
                }
            }
            else if(opcode == "phi") {
                BasicBlock* bb = I->getParent();
                unsigned firstNonPHI = instr2index(bb->getFirstNonPHI());
                for(unsigned i = currentIndex; i < firstNonPHI; i++) {
                    Instruction* PhiInst = index2instr(i);
                    std::set<Value*> R = usedVariables(PhiInst);
                    Value* ret = dyn_cast<Value>(PhiInst);
                    Constant* value = NULL;
                    bool isConstant = true;
                    // errs() << "phi node\n";
                    // errs() << (*PhiInst) << "\n";
                    // errs() << "Number of used variables: " << (R.size()) << "\n";
                    for(auto it : R) {
                        // errs() << (*it) << "\n";
                        Constant* c = getConstValue(it, info);
                        // if(c == NULL) {
                        //     errs() << "NULL\n";
                        // }
                        // else 
                        //     errs() << (*c) << "\n";
                        if(c == NULL) {
                            isConstant = false;
                            break;
                        }
                        if(value == NULL) {
                            value = c;
                        }
                        else if(!isSameConstant(value, c)) {
                            // errs() << "not same constant\n";
                            isConstant = false;
                            break;
                        }
                    }
                    if(isConstant) {
                        // errs() << "set constant\n";
                        info->setConstant(ret, value);
                    }
                }
            }
            else if(opcode == "select") {
                Value* v1 = I->getOperand(0);
                Value* v2 = I->getOperand(1);
                Value* v3 = I->getOperand(2);
                Constant* c1 = getConstValue(v1, info);
                Constant* c2 = getConstValue(v2, info);
                Constant* c3 = getConstValue(v3, info);
                if(c1 == NULL) {
                    if(c2 != NULL && c3 != NULL && isSameConstant(c2, c3)) {
                        info->setConstant(v, c2);
                    }
                    else {
                        info->setTop(v);
                    }
                }
                else if(c2 == NULL || c3 == NULL) {
                    info->setTop(v);
                }
                else {
                    Constant* value = FOLDER.CreateSelect(c1, c2, c3);
                    info->setConstant(v, value);
                }
            }

            // errs() << "current constant value in map:\n";
            // for(auto it : info->getInfo()) {
            //     if(it.second.state == Const) {
            //         errs() << (*(it.first)) << "\n";
            //     }
            // }
            

            for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                Infos.push_back(info);
            }
        }
};

namespace {
struct ConstPropAnalysisPass : public CallGraphSCCPass {
    static char ID;
    ConstPropAnalysisPass() : CallGraphSCCPass(ID) {}
    std::unordered_map<Function*, std::set<GlobalVariable*>> LMOD;
    std::unordered_map<Function*, std::set<GlobalVariable*>> CMOD;
    
    std::set<Value*> getOperands(Instruction *I) {
        // TO GET THE OPERANDS OF AN INSTRUCTION
        std::set<Value*> s;
        for(Use &U:I->operands()) {
            Value* V = U.get();
            s.insert(V);
        }
        return s;
    }

    Value* getOperand(Instruction *I, int index) {
        Value* v = I->getOperand(index);
        return v;
    }

    bool doInitialization(CallGraph &CG) override {
        /*
        Build MPT and LMOD data structure
        */

        // LMOD:
        //  If an instruction modifies a global variable, the global variable must be added to LMOD, glob = ....
        //  If an instruction modifies a dereferenced pointer in any given function F, i.e., *var = ...., you'll need to add the subset of MPT containing GlobalVariables to the LMOD for that particular function. // NO IDEA HOW TO FIND THOSE INSTRUCTIONS
        // MPT: 
        //  If you read the address of any variable, the variable must be added to the MPT set; 
        //  If you encounter an operand in a function that is being passes by reference, you need to add it to the MPT set as well

        for (auto &glob : CG.getModule().getGlobalList()) {
            GlobVars.insert(dyn_cast<Value>(&glob));
        }

        for(Function &F : CG.getModule().functions()) {
            std::set<Value*> localMPT;
            for (Function::iterator bi = F.begin(), e = F.end(); bi != e; ++bi) {
				BasicBlock * block = &*bi;
                for (auto ii = block->begin(), ie = block->end(); ii != ie; ++ii) {
					Instruction * instr = &*ii;
					std::string opcode = std::string(instr->getOpcodeName());
                    if(opcode == "store") {
                        Value* v = (Value *) instr->getOperand(0);
                        if (v->getType()->isPointerTy()) {
                            localMPT.insert(v);
                        }
                        // e.g. store i32* @x, i32** %2, align 8
                        if(GlobalVariable::classof(v)) {
                            localMPT.insert(v);
                        }
                    }
                    else if(CallInst::classof(instr)) {
                        // CallInst* call_instr = cast<CallInst>(&instr);
                        std::set<Value*> s = getOperands(instr);
                        for(auto &v:s) {
                            if(v->getType()->isPointerTy()) {
                                // errs() << (*v) << "\n";
                                localMPT.insert(v);
                            }
                        }
                    }
                    else if (isa<ReturnInst>(instr)) {
                        if (instr->getOperand(0)->getType() != NULL) {
                            if (instr->getType()->isPointerTy()) {
                                localMPT.insert((Value *) instr->getOperand(0));
                            }
                        }
                    }
				}
            }
            MPT.insert(localMPT.begin(), localMPT.end());
        }

        for(Function &F : CG.getModule().functions()) {
            std::set<GlobalVariable*> localLMOD;
            bool addMPTtoLMOD  =false;
            for (Function::iterator bi = F.begin(), e = F.end(); bi != e; ++bi) {
				BasicBlock * block = &*bi;
                for (auto ii = block->begin(), ie = block->end(); ii != ie; ++ii) {
					Instruction * instr = &*ii;
					std::string opcode = std::string(instr->getOpcodeName());
                    if(opcode == "store") {
                        if(!addMPTtoLMOD) {
                            Value* p = instr->getOperand(1);
                            Instruction* defineInst = dyn_cast<Instruction>(p);
                            if(defineInst != NULL && defineInst != nullptr) {
                                std::string opcode2 = std::string(defineInst->getOpcodeName());
                                if(opcode2 == "load" && p->getType()->isPointerTy()) {
                                    addMPTtoLMOD = true;
                                }
                            }
                        }
                        // e.g. store i32 4, i32* @b, align 4
                        Value* v = getOperand(instr, 1);
                        if(GlobalVariable::classof(v)) {
                            localLMOD.insert(dyn_cast<GlobalVariable>(v));
                        }
                    }
				}
            }
            if(addMPTtoLMOD) {
                for(auto v:MPT) {
                    GlobalVariable* gv = dyn_cast<GlobalVariable>(v);
                    if(gv) localLMOD.insert(gv);
                }
                // localLMOD.insert(localMPT.begin(), localMPT.end());
            }
            LMOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, localLMOD));
            MOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, localLMOD));
            std::set<GlobalVariable*> empty;
            CMOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, empty));
        }

        // debug only
        // errs() << "MPT:" << (MPT.size()) << "\n";
        // for(auto v : MPT) {
        //     errs() << *v;
        //     errs() << "\n";
        // }
        // errs() << "MOD:\n";
        // for(auto it : MOD) {
        //     errs() << "F:";
        //     printValue(it.first);
        //     errs() << "\n";
        //     for(auto it2 : (it.second)) {
        //         printValue(it2);
        //         errs() << "\n";
        //     }
        // }
        return true;
    }

    bool runOnSCC(CallGraphSCC &SCC) override {
        /*
        build CMOD data structures
        */

        // CMOD: callee MOD: the variables modified in the body of other functions by calls
        // Run a worklist algorithm to get CMOD
        std::deque<Function*> worklist;
        std::set<Function*> functionInSCC;
        std::map<Function*, std::set<Function*>> callee;
        std::map<Function*, std::set<Function*>> caller;
        for (CallGraphNode* node : SCC) {
            // CallGraphNode* node = *it;
            Function* func = node->getFunction();
            if(func != nullptr) {
                worklist.push_back(func);
                functionInSCC.insert(func);
            }
        }
        // initialize caller and callee map
        // initialize worklist
        for(CallGraphNode* callerNode : SCC) {
            Function* func = callerNode->getFunction();
            if(func == nullptr || func == NULL)
                continue;
            for(auto pair : *callerNode) {
                CallGraphNode* calleeNode = pair.second;
                Function* calleeFunc = calleeNode->getFunction();
                if(calleeFunc == nullptr || calleeFunc == NULL)
                    continue;
                if(callee.count(func) == 0) {
                    std::set<Function*> s;
                    s.insert(calleeFunc);
                    // callee.insert(pair<Function*, std::set<Function*>>(func, s));
                    callee.insert(std::make_pair(func, s));
                }
                else {
                    callee[func].insert(calleeFunc);
                }
                if(caller.count(calleeFunc) == 0) {
                    std::set<Function*> s;
                    s.insert(func);
                    // callee.insert(pair<Function*, std::set<Function*>>(calleeFunc, s));
                    caller.insert(std::make_pair(calleeFunc, s));
                }
                else {
                    caller[calleeFunc].insert(func);
                }
            }
        }
        // iterate on worklist
        while (	!worklist.empty() ) {
            Function* F = worklist.front();
			worklist.pop_front();
            if(callee.count(F) == 0)
                continue;
            std::set<GlobalVariable*> s;
            std::set<Function*> calleeFunctions = callee[F];
            for(auto func : calleeFunctions) {
                std::set<GlobalVariable*> tmp = MOD[func];
                s.insert(tmp.begin(), tmp.end());
            }
            std::set<GlobalVariable*> old = LMOD[F];
            if(old != s) {
                // not identical
                LMOD[F] = s;
                auto tmp = MOD[F];
                tmp.insert(s.begin(), s.end());
                MOD[F] = tmp;
                // MOD[F].insert(s.begin(), s.end());
                if(caller.count(F) == 0)
                    continue;
                std::set<Function*> callerFunctions = caller[F];
                for(auto func : callerFunctions) {
                    if(functionInSCC.count(func) != 0)
                        worklist.push_back(func);
                }
            }
        }
        return true;
    }

    bool doFinalization(CallGraph &CG) override {
        /*
        Iterate over the entire call graph and call the worklist algorithm to do the Constant Prop Analysis on each function and print the analysis results
        */
        // std::set<Value*> MPT;
        // std::unordered_map<Function*, std::set<GlobalVariable*>> MOD;
        // errs() << "MOD after RunOnSCC:\n";
        // for(auto it : MOD) {
        //     errs() << "F:";
        //     printValue(it.first);
        //     errs() << "\n";
        //     for(auto it2 : (it.second)) {
        //         printValue(it2);
        //         errs() << "\n";
        //     }
        // }

        for(Function &F : CG.getModule().functions()) {
            // errs() << "F = " << getValueName(&F)<<"\n";
            // ConstPropInfo bottom;
            // ConstPropInfo initialState;
            ConstPropInfo bottom = ConstPropInfo("bottom");
            ConstPropInfo initialState = ConstPropInfo("top");;
            ConstPropAnalysis *cp = new ConstPropAnalysis(bottom, initialState);
            cp->runWorklistAlgorithm(&F);
            cp->print();
        }
        return true;
    }
  
}; // end of struct MayPointToAnalaysisPass
}  // end of anonymous namespace

char ConstPropAnalysisPass ::ID = 0;
static RegisterPass<ConstPropAnalysisPass > X("cse231-constprop", "Constant Propagation Analysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
