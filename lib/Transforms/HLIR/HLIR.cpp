//===- HLIR.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// This file contains proof-of-concept passes for the HLIR project at LANL.
/// Currently, it contains the following passes...
/// - LaunchCall
///   Allows function calls with the hlir.launch metadata node to be wrapped
///   into pthreads, not unlike go routines in the go language.
//
//===----------------------------------------------------------------------===//

#include <list>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

  /// Launch Call is a transform pass which will transform launch calls
  /// into pthread_create calls.
  ///
  /// Future: Because HLIR lowers into llvm, we need a structure not unlike
  ///         the codegen facilities in llvm. That means that this would
  ///         be an abstract class, and would be made concrete by various
  ///         runtime generators.
  class LaunchCall : public BasicBlockPass {
  private:
    Function *pthread_create;

    PointerType *VoidPtrTy;
    PointerType *PthreadAttrPtrTy;
    PointerType *Int64PtrTy;
    PointerType *StartFTy;
    FunctionType *PthreadTy;

    /// Initialize all types that are used in this pass (really just for
    /// convenience).
    void initTypes(Module &M) {
      /*
        Taken from `man pthread_create`:

        int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg);
      */
      this->VoidPtrTy = Type::getInt8PtrTy(M.getContext());
      this->Int64PtrTy = Type::getInt64PtrTy(M.getContext());

      Type *StructComps[2] = {
        Type::getInt64Ty(M.getContext()),
        ArrayType::get(Type::getInt8Ty(M.getContext()), 48)
      };
      Type *StructTy = StructType::create(StructComps, "union.pthread_attr_t");
      this->PthreadAttrPtrTy = PointerType::get(StructTy, 0);

      Type *StartFArgTy[1] = { this->VoidPtrTy };
      this->StartFTy = PointerType::get(
        FunctionType::get(this->VoidPtrTy, StartFArgTy, false), 0);

      Type *PthreadArgsTy[4] = {
        this->Int64PtrTy,
        this->PthreadAttrPtrTy,
        this->StartFTy,
        this->VoidPtrTy
      };
      this->PthreadTy = FunctionType::get(IntegerType::get(M.getContext(), 32),
                                          PthreadArgsTy, false);
    }

    /// Make a declaration of `pthread_create` if necessary. Returns whether or
    /// not the module was changed.
    ///
    /// TODO: Currently always makes the declaration.
    bool initPthreadCreate(Module &M) {
      this->pthread_create = Function::Create(PthreadTy, Function::ExternalLinkage,
                                              "pthread_create", &M);
      return true;
    }

    /// Declare and return a structure with elements equal to the argument types
    /// of a function.
    StructType *makeArgStructType(Function *Func) {
      StructType *WrapTy = NULL;

      if(Func->getFunctionType()->getNumParams()) {
        WrapTy = StructType::create(Func->getFunctionType()->params());
      }

      return WrapTy;
    }

    Function *makeWrapperFunction(Module *M, Function *Func, StructType* WrapTy) {
      Type *Arg[1]          = { this->VoidPtrTy };
      FunctionType *FuncTy  = FunctionType::get(this->VoidPtrTy, Arg, false);
      Function *WrappedFunc = Function::Create(FuncTy, Function::ExternalLinkage,
                                               "hlir.pthread.wrapped." +
                                               Func->getName(), M);

      BasicBlock *Block = BasicBlock::Create(Func->getContext(), "entry", WrappedFunc, 0);
      IRBuilder<> B(Block);
      std::vector<Value*> UnpackedArgs;

      if(WrapTy) {
        //WrapTy->dump();
        // Unpack and call.
        AllocaInst *PtrArg = B.CreateAlloca(this->VoidPtrTy);
        B.CreateStore(WrappedFunc->arg_begin(), PtrArg);
        Value* PackedArg = B.CreateLoad(PtrArg);
        PackedArg = B.CreateBitCast(PackedArg, PointerType::get(WrapTy, 0));

        // This assumes that the order of the structure is the same as the order
        // of the original arguments.
        int ElemIndex = 0;
        for(auto _ : WrapTy->elements()) {
          // Those types are NECESSARY
          Value *GEPIndex[2] = {ConstantInt::get(Type::getInt64Ty(Func->getContext()),
                                                 0),
                                ConstantInt::get(Type::getInt32Ty(Func->getContext()),
                                                 ElemIndex)};
          Value *ElemPtr = B.CreateGEP(PackedArg, GEPIndex);
          //Value *Elem = B.CreateLoad(ElemPtr);
          UnpackedArgs.push_back(B.CreateLoad(ElemPtr));
          ElemIndex++;
        }
      }

      B.CreateCall(Func, ArrayRef<Value*>(UnpackedArgs));
      B.CreateRet(ConstantPointerNull::get(this->VoidPtrTy));
      return WrappedFunc;
    }

  public:
    static char ID;
    LaunchCall() : BasicBlockPass(ID) {};

    /// Before the pass is run, a declaration of pthread_create will be made.
    /// This function will also initialize some commonly used types.
    ///
    /// NOTE: This happens here because BasicBlockPass's are not allowed to
    ///       make new global elements, but initialize rs are.
    /// TODO: Only make the function if it is needed. This will probably
    ///       require an analysis pass (which we already need).
    /// TODO: Right now the type of pthread_attr_t is hard coded and not
    ///       portable. This is probably acceptable as a proof-of-concept.
    /// TODO: Currently wraps every function, just for shits and giggles.
    bool doInitialization(Module &M) override {
      bool Changed = false;
      this->initTypes(M);
      Changed |= this->initPthreadCreate(M);

      // Wrap every living function on the planet.
      // Stupid copy because I'm lazy
      std::vector<Function*> Funcs;
      for(auto &Func : M.functions()) {
        Funcs.push_back(&Func);
      }

      for(auto Func : Funcs){
        StructType *Ty = makeArgStructType(Func);
        makeWrapperFunction(&M, Func, Ty);
      }

      return Changed;
    }

    /// This function represents the actual pass. It will look for any call
    /// instruction with the `hlir.launch` metadata node attached, and will
    /// replace that call with the launching of a pthread which will run the
    /// function.
    ///
    /// TODO: Actually wrap ANY function into a proper work function.
    /// TODO: Actually look for `hlir.launch` metadata, as opposed to any
    ///       kind of metadata.
    /// TODO: Possibly look into actually checking if the pthread worked?
    ///       That would require actually making new basic blocks...
    bool runOnBasicBlock(BasicBlock& BB) override {
      bool Changed = false;
      std::list<Instruction*> LaunchCalls;

      // Go over every instruction in the block, and search for relevant calls.
      // NOTE: I first gather, then operate so that I can delete the instructions
      //       and continue to iterate.
      for(auto &instruc : BB) {
        if(isa<CallInst>(instruc) && instruc.hasMetadata()) {
          LaunchCalls.push_back(&instruc);
        }
      }

      // For every launch call, call -> alloc thread, call pthread.
      for(auto &instruc : LaunchCalls) {
        instruc->dump();
        Function *CalledF = cast<CallInst>(instruc)->getCalledFunction();
        AllocaInst *ThreadPtr = new AllocaInst(Type::getInt64Ty(BB.getContext()),
                                            "", instruc);

        assert(CalledF->getType() == this->StartFTy &&
               "Currently we can only launch functions which are already of "
               "worker type!");

        Value *Args[4] = {ThreadPtr,
                          ConstantPointerNull::get(this->PthreadAttrPtrTy),
                          CalledF,
                          ConstantPointerNull::get(this->VoidPtrTy)};
        CallInst *call = CallInst::Create(this->pthread_create, Args, "", instruc);

        instruc->eraseFromParent();
        Changed = true;
      }

      return Changed;
    }

  }; // class LaunchCall
} // namespace


char LaunchCall::ID = 0;
static RegisterPass<LaunchCall> X("launch", "Task Launch Pass");
