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
/// Currently, it contains a single pass (HLIRLower) which will lower HLIR
/// into LLVM IR. This lowering can currently handle the following HLIR
/// constructs...
/// - LaunchCall
///   Allows function calls with the hlir.launch metadata node to be wrapped
///   into pthreads, not unlike go routines in the go language.
//
//===----------------------------------------------------------------------===//
#include <list>
#include <map>

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
  class HLIRLower : public ModulePass {
  private:
    /// Common Functions
    Function *pthread_create;
    Function *pthread_exit;
    Function *pthread_join;

    /// Common Types
    PointerType *VoidPtrTy;
    PointerType *PthreadAttrPtrTy;
    PointerType *Int64PtrTy;
    PointerType *StartFTy;
    FunctionType *CreateTy;
    FunctionType *ExitTy;
    FunctionType *JoinTy;

    /// Maps a function to a wrapped version of that function which is
    /// able to be called by llvm.
    std::map<Function*, Function*> FuncToWrapFunc;
    std::map<Function*, StructType*> FuncToWrapArg;

    /// Initialize all types that are used in this pass (really just for
    /// convenience).
    void initTypes(Module &M) {
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

      /*
        int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg);
      */
      Type *PthreadArgsTy[4] = {
        this->Int64PtrTy,
        this->PthreadAttrPtrTy,
        this->StartFTy,
        this->VoidPtrTy
      };
      this->CreateTy = FunctionType::get(IntegerType::get(M.getContext(), 32),
                                         PthreadArgsTy, false);

      /*
        void pthread_exit(void *retval);
      */
      Type* ExitArgsTy[1] = {
        this->VoidPtrTy
      };
      this->ExitTy = FunctionType::get(Type::getVoidTy(M.getContext()), ExitArgsTy,
                                      false);

      /*
        int pthread_join(pthread_t thread, void **retval);
      */
      Type* JoinArgsTy[2] = {
        Type::getInt64Ty(M.getContext()),
        PointerType::get(this->VoidPtrTy, 0)
      };
      this->JoinTy = FunctionType::get(Type::getInt32PtrTy(M.getContext()),
                                       JoinArgsTy, false);
    }

    /// Make a declaration of `pthread_create` if necessary. Returns whether or
    /// not the module was changed.
    ///
    /// TODO: Currently always makes the declaration.
    bool initPthreadCreate(Module *M) {
      this->pthread_create = Function::Create(CreateTy, Function::ExternalLinkage,
                                              "pthread_create", M);
      this->pthread_exit   = Function::Create(ExitTy, Function::ExternalLinkage,
                                            "pthread_exit", M);
      this->pthread_join   = Function::Create(JoinTy, Function::ExternalLinkage,
                                            "pthread_join", M);

      return true;
    }

    /// Declare and return a structure with elements equal to the argument types
    /// of a function. The first element will be the return value, unless the
    /// return value is void.
    StructType *makeArgStructType(Function *Func) {
      StructType *WrapTy = NULL;
      std::vector<Type*> StructContent;

      if(Func->getFunctionType()->getReturnType() !=
         Type::getVoidTy(Func->getContext())) {
        StructContent.push_back(Func->getFunctionType()->getReturnType());
      }

      for(auto param : Func->getFunctionType()->params()) {
          StructContent.push_back(param);
      }

      WrapTy = StructType::create(ArrayRef<Type*>(StructContent));
      return WrapTy;
    }


    /// Declare, construct, and return a Function wrapped to be launched by a pthread.
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

    bool LowerLaunchCall(Module *M, CallInst *I){
      IRBuilder<> B(I);
      Function *F = I->getCalledFunction();
      Function *WrappedF;
      StructType *WrappedArgTy;

      // Get wrapped function, or wrap function if it does not currently have
      // a wrapped form
      if( this->FuncToWrapFunc.find(F) == this->FuncToWrapFunc.end() ) {
        WrappedArgTy = makeArgStructType(F);
        this->FuncToWrapArg.emplace(F, WrappedArgTy);
        this->FuncToWrapFunc.emplace(F, makeWrapperFunction(M, F, WrappedArgTy));
      }

      WrappedF     = this->FuncToWrapFunc[F];
      WrappedArgTy = this->FuncToWrapArg[F];
      if( !this->pthread_create ) {
        initPthreadCreate(M);

      }

      // Alloc a thread, pack args, launch pthread, remove old instruction.
      Value *ThreadPtr = B.CreateAlloca(Type::getInt64Ty(M->getContext()));
      Value *ArgPtr    = B.CreateAlloca(WrappedArgTy);
      int ArgId = 0;
      for(auto &Arg : I->arg_operands()) {
        Value *GEPIndex[2] = {ConstantInt::get(Type::getInt64Ty(M->getContext()),
                                               0),
                              ConstantInt::get(Type::getInt32Ty(M->getContext()),
                                               ArgId)};
        B.CreateStore(Arg, B.CreateGEP(ArgPtr, GEPIndex));
        ArgId++;
      }
      Value *PThreadArgs[4] = {ThreadPtr,
                               ConstantPointerNull::get(this->PthreadAttrPtrTy),
                               WrappedF,
                               B.CreateBitCast(ArgPtr, this->VoidPtrTy)};
      B.CreateCall(this->pthread_create, PThreadArgs);
      I->eraseFromParent();

      return true;
    }

  public:
    static char ID;
    HLIRLower() : ModulePass(ID), pthread_create(nullptr), pthread_exit(nullptr),
                  pthread_join(nullptr) {};

    /// This function represents the actual pass.
    /// CURRENT STATE:
    ///  It will look for any call
    /// instruction with the `hlir.launch` metadata node attached, and will
    /// replace that call with the launching of a pthread which will run the
    /// function.
    ///
    /// TODO: Actually look for `hlir.launch` metadata, as opposed to any
    ///       kind of metadata.
    /// TODO: Possibly look into actually checking if the pthread worked?
    ///       That would require actually making new basic blocks...
    bool runOnModule(Module& M) override {
      bool Changed = false;
      std::list<Instruction*> LaunchCalls;
      this->initTypes(M);

      // For Every Function
      for(auto &F : M) {
        // For Every Basic Block
        for(auto &BB : F) {
          // For Every Instruction
          for(auto &I : BB) {
            // NOTE: I first gather, then operate so that I can delete the
            // instructions and continue to iterate.
            if(isa<CallInst>(I) && I.hasMetadata()) {
              LaunchCalls.push_back(&I);
            }
          }
        }
      }

      for(auto &I : LaunchCalls) {
        Changed |= LowerLaunchCall(&M, cast<CallInst>(I));
      }

      return Changed;
    }

  }; // class HLIRLower
} // namespace

char HLIRLower::ID = 0;
static RegisterPass<HLIRLower> X("hlir", "Lower HLIR to LLIR");
