#include <iostream>
#include <map>
#include <string>

#include <stdint.h>
#include <stdlib.h>
#include "llvm/IR/Instruction.h"
using namespace llvm;
using namespace std; 

std::map<string, unsigned> instr_map;
int branch_count[2];

//clang++ /tmp/lib231.ll /tmp/hello-cdi.ll `llvm-config --system-libs --cppflags --ldflags --libs core` -o /tmp/cdi_hello

// For section 2
// num: the number of unique instructions in the basic block. It is the length of keys and values.
// keys: the array of the opcodes of the instructions
// values: the array of the counts of the instructions
extern "C" __attribute__((visibility("default")))
void updateInstrInfo(unsigned num, uint32_t * keys, uint32_t * values) {
  int i;
  uint32_t key;
  uint32_t value;
  string keyString;

  for (i=0; i<num; i++) {
    keyString = Instruction::getOpcodeName(keys[i]);
    value = values[i];
    if (instr_map.count(keyString) == 0)
    	instr_map.insert(std::pair<string, uint32_t>(keyString, value));
    else
    	instr_map[keyString] = instr_map[keyString] + value;
  }

  return;
}

// For section 3
// If taken is true, then a conditional branch is taken;
// If taken is false, then a conditional branch is not taken.
extern "C" __attribute__((visibility("default")))
void updateBranchInfo(bool taken) {

	if (taken)
		branch_count[0] ++;
	branch_count[1] ++;

  return;
}

// For section 2
extern "C" __attribute__((visibility("default")))
void printOutInstrInfo() {

  for (std::map<string, uint32_t>::iterator it=instr_map.begin(); it!=instr_map.end(); ++it)
    std::cerr << it->first << '\t' << it->second << '\n';

  instr_map.clear();

  return;
}

// For section 3
extern "C" __attribute__((visibility("default")))
void printOutBranchInfo() {

	std::cerr << "taken\t" << branch_count[0] << '\n';
	std::cerr << "total\t" << branch_count[1] << '\n';

	branch_count[0] = 0;
	branch_count[1] = 0;

  return;
}

