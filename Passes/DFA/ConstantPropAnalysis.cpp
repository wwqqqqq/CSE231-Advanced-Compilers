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
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/InstrTypes.h"


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

class ConstPropInfo : public Info {
    private:
        enum ConstState { Bottom, Const, Top };
        typedef struct {
            ConstState state;
            Constant* value;
        } ConstInfo;
        std::map<Value*, ConstInfo> ConstPropContent;

        bool isSameConstant(Constant* c1, Constant* c2) {
            return true;
        }

    public:
    ConstPropInfo() {}
    ConstPropInfo(CallGraph &CG, string s) {
        ConstInfo info;
        info.state = (s == "bottom")?Bottom : Top;
        info.value = NULL;
        for(auto gv : CG.getModule().getGlobalList()) {
            // NOT SURE IF THIS WILL WORK OR IS GV A GLOBAL VARIABLE
            Value* v = dyn_cast<Value>(gv);
            info.insert(pair<Value*, ConstInfo>(v, info));
        }
    }

    ConstPropInfo(std::map<Value*, ConstInfo> s) {
        ConstPropContent = s;
    }

    /*
     * Print out the information
     */
    void print() {
        char bottom = (char)193;
        char top = (char)194;
        for(auto it : info) {
            errs() << (it.first) << "="; // << (it.second) << "|";
            if(it.second.state == Bottom) {
                errs() << bottom;
            }
            else if(it.second.state == Const) {
                errs() << it.second.value;
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
        if(info.count(v) == 0) {
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

    /*
     * Compare two pieces of information
     */
    static bool equals(ConstPropInfo * info1, ConstPropInfo * info2) {
        return info1->getInfo() == info2->getInfo();
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
                continue;
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
                if(*(infoSet1[it.first].value) == *(it.second.value)) {
                    continue;
                }
                else {
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
                if(isSameConstant(ConstPropContent[it.first].value, it.second.value) {
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
            ConstPropContent[v] = c;
        }
    }

    void setConstant(Value* v, Constant* c) {
        ConstInfo info;
        info.state = Const;
        info.value = c;
        if(ConstPropContent.count(v) == 0) {
            ConstPropContent.insert(pair<Value*, ConstInfo>(v, info));
        }
        else {
            ConstPropContent[v] = c;
        }
    }
    
};

class ConstPropAnalysis : public DataFlowAnalysis<ConstPropInfo, true>  {   // <info, direction>
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        enum ConstState { Bottom, Const, Top };
        typedef struct {
            ConstState state;
            Constant* value;
        } ConstInfo;

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
            if(Constant::classof(v)) {
                return dyn_cast<Constant>(v);
            }
            ConstInfo constInfo = info->getConstInfo(v);
            if(constInfo.state == Const) {
                return constInfo.value;
            }
            return NULL;


            Instruction* I = dyn_cast<Instruction>(v);
            string opcode = std::string(I->getOpcodeName());

            ConstantFolder FOLDER;

            if(I->isBinaryOp()) {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL)
                    return NULL;
                BinaryOperator* bin_op = dyn_cast<BinaryOperator>(I);
                Constant* value = FOLDER.CreateBinOp(opcode, c1, c2);
                return value;
            }
            else if(I->isUnaryOp()) {
                Value* op = I->getOperand(0);
                Constant* c = getConstValue(op1, info);
                if(c == NULL) return NULL;
                UnaryOperator* un_op = dyn_cast<UnaryOperator>(I);
                Constant* value = FOLDER.CreateUnOp(opcode, c);
                return value;
            }
            else if(opcode == "call") {
                // if v is a return value of call, we may assume it is not constant
                return NULL;
            }
            else if(opcode == "load") {
                // very hard
            }
            else if(opcode == "icmp") {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL)
                    return NULL;
                ComInst* com = dyn_cast<ComInst>(I);
                Constant* value = FOLDER.CreateICmp(bin_op, c1, c2);
                return value;
            }
            else if(opcode == "fcmp") {
                //
            }
            else if(opcode == "phi" || opcode == "select") {
                std::set<Value*> R = usedVariables(I);
                Constant* value = NULL;
                for(auto it : R) {
                    Constant* c = getConstValue(it, info);
                    if(c == NULL) {
                        return NULL;
                    }
                    if(value == NULL) {
                        value = c;
                    }
                    else if(!isSameConstant(value, c)) {
                        return NULL;
                    }
                }
                return value;
            }

            return NULL;

        }

        bool isSameConstant(Constant* c1, Constant* c2) {
            return true;
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

            v = dyn_cast<Value>(I);
            
            if(I->isBinaryOp()) {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL) {
                    info->setTop(v);
                }
                BinaryOperator* bin_op = dyn_cast<BinaryOperator>(I);
                Constant* value = FOLDER.CreateBinOp(bin_op->getOpcode(), c1, c2);
                info->setConstant(v, value);
            }
            else if(I->isUnaryOp()) {
                Value* op = I->getOperand(0);
                Constant* c = getConstValue(op1, info);
                if(c == NULL) {
                    info->setTop(v);
                }
                UnaryOperator* un_op = dyn_cast<UnaryOperator>(I);
                Constant* value = FOLDER.CreateUnOp(un_op->getOpcode(), c);
                info->setConstant(v, value);
            }
            else if(opcode == "load") {
                // y = *p ->
                // %4 = load i32*, i32** @p, align 8
                // %5 = load i32, i32* %4, align 4
                // store i32 %5, i32* @y, align 4
                // *p = b ->
                // %6 = load i32, i32* @b, align 4
                // %7 = load i32*, i32** @p, align 8
                // store i32 %6, i32* %7, align 4
                // *p = *p1 + *p2 ->
                // %8 = load i32*, i32** @p1, align 8
                // %9 = load i32, i32* %8, align 4
                // %10 = load i32*, i32** @p2, align 8
                // %11 = load i32, i32* %10, align 4
                // %12 = add nsw i32 %9, %11
                // %13 = load i32*, i32** @p, align 8
                // store i32 %12, i32* %13, align 4
                Instruction* nextInstr = index2intr(currentIndex + 1);
                if(nextInstr != NULL && nextInstr != nullptr && nextInstr->getParent() == I->getParent()) {
                    std::string nextOpcode = std::string(nextInstr->getOpcodeName());
                    if(nextOpcode == "store") {
                        Value* returnval = cast<Value>(I);
                        Value* v = getOperand(nextInstr, 1);
                        if(returnval == v) {
                            // Exist instructions like *val = ...
                            // TODO: Set all variables in MPT to top
                        }
                    }
                }
            }
            else if(opcode == "store") {
                // x = 2 -> store i32 2, i32* @x, align 4
                // p = &x -> store i32* @x, i32** @p, align 8
                // val = &x -> store i32* @x, i32** %1, align 8
                // *p = b ->
                // %6 = load i32, i32* @b, align 4
                // %7 = load i32*, i32** @p, align 8
                // store i32 %6, i32* %7, align 4
                Value* v = I->getOperand(1);
                if(isGlobalVariable(v)) {
                    // global variable v is changed
                    Value* operand = I->getOperand(0);
                    Constant* c = getConstValue(operand, info);
                    if(c == NULL) {
                        info->setTop(v);
                    }
                    else {
                        info->setConstant(v, c);
                    }
                }
                else {
                    // have to consider the case where v is pointing to a global variable
                    Instruction* defineInst = dyn_cast<Instruction>(v);
                    string opcode2 = std::string(defineInst->getOpcodeName());
                    if(opcode2 == "load") {
                        // v is a pointer
                        Value* pointedValue = defineInst->getOperand(1);
                        if(isGlobalVariable(pointedValue)) {
                            Value* operand = I->getOperand(0);
                            Constant* c = getConstValue(operand, info);
                            if(c == NULL) {
                                info->setTop(pointedValue);
                            }
                            else {
                                info->setConstant(pointedValue, c);
                            }
                        }
                    }
                }
            }
            else if(opcode == "call") {
                // TODO: set all variables in the callee's MOD set to top
                // y = add1() -> 
                // %14 = call i32 @_Z4add1v()
                // store i32 %14, i32* @y, align 4
            }
            else if(opcode == "icmp") {
                Value* op1 = I->getOperand(0);
                Value* op2 = I->getOperand(1);
                Constant* c1 = getConstValue(op1, info);
                Constant* c2 = getConstValue(op2, info);
                if(c1 == NULL || c2 == NULL) {
                    info->setTop(v);
                }
                ComInst* com = dyn_cast<ComInst>(I);
                Predicate pred = com->getPredicate();
                Constant* value = FOLDER.CreateICmp(pred, c1, c2);
                info->setConstant(v, value);
            }
            else if(opcode == "fcmp") {
                ;
            }
            else if(opcode == "phi") {
                
            }
            else if(opcode == "select") {
                // similar with Phi
                Value* ret = dyn_cast<Value>(I);
                if(isGlobalVariable(ret)) {
                    std::set<Value*> R = usedVariables(I);
                    Constant* value = NULL;
                    bool isConstant = true;
                    for(auto it : R) {
                        Constant* c = getConstValue(it, info)
                        if(c == NULL) {
                            isConstant = false;
                            break;
                        }
                        if(value == NULL) {
                            value = c;
                        }
                        else if(!isSameConstant(value, c)) {
                            isConstant = false;
                            break;
                        }
                    }
                    if(isConstant) {
                        info->setConstant(ret, value);
                    }
                }
            }
            

            for(size_t i = 0; i < OutgoingEdges.size(); i++) {
                Infos.push_back(info);
            }
        }
};

namespace {
struct ConstPropAnalysisPass : public CallGraphSCCPass {
    static char ID;
    ConstPropAnalysisPass() : CallGraphSCCPass(ID) {}

    std::set<Value*> MPT;
    std::unordered_map<Function*, std::set<GlobalVariable*>> LMOD;
    std::unordered_map<Function*, std::set<GlobalVariable*>> CMOD;
    std::unordered_map<Function*, std::set<GlobalVariable*>> MOD; // union of LMOD and CMOD

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
        // TO GET INDEX'S CORRESPONDING OPERAND OF AN INSTRUCTION
        // std::set<Value*> s;
        // int i = 0;
        // for(Use &U:I->operands()) {
        //     if(i == index) {
        //         Value* V = U.get();
        //         s.insert(V);
        //         break;
        //     }
        //     i++;
        // }
        // return s;
        Value* v = I->getOperand(index);
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

        for(Function &F : CG.getModule().functions()) {
            // NOT SURE IF THIS WORKS
            std::set<Value*> localMPT;
            std::set<Value*> localLMOD;
            bool addMPTtoLMOD  =false;
            for (Function::iterator bi = func->begin(), e = func->end(); bi != e; ++bi) {
				BasicBlock * block = &*bi;
                for (auto ii = block->begin(), ie = block->end(); ii != ie; ++ii) {
					Instruction * instr = &*ii;
					std::string opcode = std::string(instr->getOpcodeName());
                    if(opcode == "store") {
                        // e.g. store i32* @x, i32** %2, align 8
                        Value* v = getOperand(instr, 0);
                        if(GlobalVariable::classof(v)) {
                            localMPT.insert(v);
                        }
                        // e.g. store i32 4, i32* @b, align 4
                        v = getOperand(instr, 1);
                        if(GlobalVariable::classof(v)) {
                            localLMOD.insert(v);
                        }
                    }
                    else if(opcode == "load" && !addMPTtoLMOD) {
                        // e.g.
                        // %4 = load i32*, i32** %1, align 8
                        // store i32 10, i32* %4, align 4
                        auto tmp = ii;
                        tmp++; // tmp is now the next instruction
                        if(tmp == ie)
                            continue;
                        Instruction* nextInstr = &*tmp;
                        std::string nextOpcode = std::string(nextInstr->getOpcodeName());
                        if(nextOpcode == "store") {
                            Value* returnval = cast<Value>(instr);
                            Value* v = getOperand(nextInstr, 1);
                            if(returnval == v) {
                                // Exist instructions like *val = ...
                                addMPTtoLMOD = true;
                            }
                        }
                    }
                    else if(CallInst::classof(instr)) {
                        // e.g. call void @_Z4swapRiS_(i32* dereferenceable(4) @x, i32* dereferenceable(4) @y)
                        // call void @_Z5swap2PiS_(i32* @x, i32* @y)
                        CallInst* call_instr = cast<CallInst>(&instr);
                        std::set<Value*> s = getOperands(instr);
                        for(auto &v:s) {
                            if(GlobalVariable::classof(v)) {
                                localMPT.insert(v);
                            }
                        }
                    }
				}
            }
            if(addMPTtoLMOD) {
                localLMOD.insert(localMPT.begin(), localMPT.end());
            }
            MPT.insert(localMPT.begin(), localMPT.end());
            LMOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, localLMOD));
            MOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, localLMOD));
            std::set<GlobalVariable*> empty;
            CMOD.insert(pair<Function*, std::set<GlobalVariable*>>(&F, empty));
        }
        return true;
    }

    bool runOnSCC(CallGraphSCC &SCC) override {
        /*
        build CMOD data structures
        */

        // CMOD: callee MOD: the variables modified in the body of other functions by calls
        // Run a worklist algorithm to get CMOD
        std::deque<Function*> worklist;
        std::map<Function*, std::set<Function*>> callee;
        std::map<Function*, std::set<Function*>> caller;
        for (auto it = SCC.begin(); it != SCC.end(); it++) {
            CallGraphNode* node = *it;
            Function* func = node->getFunction();
            if(func != nullptr)
                worklist.push_back(func);
        }
        // initialize caller and callee map
        // initialize worklist
        for(CallGraphNode* callerNode : SCC) {
            Function* func = node->getFunction();
            if(func == nullptr || function == NULL)
                continue;
            for(auto pair : *callerNode) {
                CallGraphNode* calleeNode = pair.second;
                Function* calleeFunc = calleeNode->getFunction();
                if(calleeFunc == nullptr || calleeFunc == NULL)
                    continue;
                if(callee.count(func) == 0) {
                    std::set<Function*> s;
                    s.insert(calleeFunc);
                    callee.insert(pair<Function*, std::set<Function*>>(func, s));
                }
                else {
                    callee[func].insert(calleeFunc);
                }
                if(caller.count(calleeFunc) == 0) {
                    std::set<Function*> s;
                    s.insert(func);
                    callee.insert(pair<Function*, std::set<Function*>>(calleeFunc, s));
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
            std::set<Value*> s;
            std::set<Function*> calleeFunctions = callee[F];
            for(auto func : calleeFunctions) {
                std::set<Value*> tmp = MOD[func];
                s.insert(tmp.begin(), tmp.end());
            }
            std::set<Value*> old = LMOD[F];
            if(old != s) {
                // not identical
                LMOD[F] = s;
                MOD[F].insert(s.begin(), s.end());
                if(caller.count(F) == 0)
                    continue;
                std::set<Function*> callerFunctions = caller[F];
                for(auto func : callerFunctions) {
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
        for(Function &F : CG.getModule().functions()) {
            ConstPropInfo bottom = ConstPropInfo(CG, "bottom");
            ConstPropInfo initialState = ConstPropInfo(CG, "initial");
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
