//===- HLIRLower.h - Passes to Lower HLIR into LLVM IR -------*- C++ -*----===//
//
//                     Project Ares
//
// This file is distributed under the expression permission of Los Alamos
// National laboratory. It is Licensed under the BSD-3 license. For more
// information, see LICENSE.md in the Ares root directory.
//
//===----------------------------------------------------------------------===//
#include <list>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "llvm/Transforms/HLIR/HLIRLower.h"

using namespace llvm;

bool HLIRLower::runOnModule(Module &M) {
  bool Changed = false;
  Changed |= this->InitLower(M);

  std::list<Instruction *> LaunchCalls;

  // For Every Function
  for (auto &F : M) {
    // For Every Basic Block
    for (auto &BB : F) {
      // For Every Instruction
      for (auto &I : BB) {
        // NOTE: I first gather, then operate so that I can delete the
        // instructions and continue to iterate.
        if (isa<CallInst>(I) && I.hasMetadata()) {
          LaunchCalls.push_back(&I);
        }
      }
    }
  }

  for (auto &I : LaunchCalls) {
    Changed |= LowerLaunchCall(&M, cast<CallInst>(I));
  }

  return Changed;
}
