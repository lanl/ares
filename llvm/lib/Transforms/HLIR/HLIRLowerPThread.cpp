//===- HLIRLowerPThread.cpp - Lower HLIR into pthreads  -------------------===//
//
//                     Project Ares
//
// This file is distributed under the expression permission of Los Alamos
// National laboratory. It is Licensed under the BSD-3 license. For more
// information, see LICENSE.md in the Ares root directory.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
#include <list>
#include <map>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "llvm/Transforms/HLIR/HLIRLower.h"

using namespace llvm;

namespace {

class HLIRLowerPThread : public HLIRLower {
private:
  /// Common Functions
  Function *pthread_create;
  Function *pthread_exit;
  Function *pthread_join;

  /// Common Type
  PointerType *PthreadAttrPtrTy;

  /// Maps a function to a wrapped version of that function which is
  /// able to be called by llvm.
  std::map<Function *, Function *> FuncToWrapFunc;
  std::map<Function *, StructType *> FuncToStruct;

  bool InitLower(Module &M) { return false; }

  /// Make a declaration of `pthread_create` if necessary. Returns whether or
  /// not the module was changed.
  ///
  /// TODO: Currently always makes the declaration.
  bool initPthreadCreate(Module *M) {
    /*
      struct pthread_attr_t {
        long;
        char[48];
      }
    */
    Type *StructComps[2] = {
        Type::getInt64Ty(M->getContext()),
        ArrayType::get(Type::getInt8Ty(M->getContext()), 48)};
    PthreadAttrPtrTy = PointerType::get(
        StructType::create(StructComps, "union.pthread_attr_t"), 0);

    /*
       void (*start)(void *args)
    */
    Type *VoidPtrArgTy[1] = {Type::getInt8PtrTy(M->getContext())};
    Type *StartFTy =
        PointerType::get(FunctionType::get(Type::getInt8PtrTy(M->getContext()),
                                           VoidPtrArgTy, false),
                         0);

    /*
      int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
      void *(*start_routine) (void *), void *arg);
    */
    Type *PthreadArgsTy[4] = {Type::getInt64PtrTy(M->getContext()),
                              PthreadAttrPtrTy, StartFTy,
                              Type::getInt8PtrTy(M->getContext())};
    FunctionType *CreateTy = FunctionType::get(
        Type::getInt32Ty(M->getContext()), PthreadArgsTy, false);
    this->pthread_create = Function::Create(CreateTy, Function::ExternalLinkage,
                                            "pthread_create", M);

    /*
     void pthread_exit(void *retval);
   */
    FunctionType *ExitTy = FunctionType::get(Type::getVoidTy(M->getContext()),
                                             VoidPtrArgTy, false);
    this->pthread_exit =
        Function::Create(ExitTy, Function::ExternalLinkage, "pthread_exit", M);

    /*
      int pthread_join(pthread_t thread, void **retval);
    */
    Type *JoinArgsTy[2] = {
        Type::getInt64Ty(M->getContext()),
        PointerType::get(Type::getInt8PtrTy(M->getContext()), 0)};
    FunctionType *JoinTy =
        FunctionType::get(Type::getInt32Ty(M->getContext()), JoinArgsTy, false);
    this->pthread_join =
        Function::Create(JoinTy, Function::ExternalLinkage, "pthread_join", M);

    return true;
  }

  /// Get or create a wrapper struct for a given function. Saves structs in the
  /// `FuncToStruct` map.
  ///
  /// Struct will have the following shape:
  ///     struct {
  ///       <return type | int>;
  ///       Semaphore;
  ///       arg1;
  ///       arg2;
  ///       ...
  ///     }
  StructType *GetFuncStruct(Function *F) {
    StructType *WrapTy = NULL;

    if (this->FuncToWrapFunc.find(F) == this->FuncToWrapFunc.end()) {
      std::vector<Type *> Members;

      if (StructType::isValidElementType(F->getReturnType())) {
        Members.push_back(F->getReturnType());
      } else {
        Members.push_back(Type::getInt32Ty(F->getContext()));
      }

      for (auto param : F->getFunctionType()->params()) {
        Members.push_back(param);
      }

      WrapTy = StructType::create(ArrayRef<Type *>(Members));
      this->FuncToStruct.emplace(F, WrapTy);
    } else {
      WrapTy = FuncToStruct[F];
    }

    return WrapTy;
  }

  /// Declare and return a structure with elements equal to the argument types
  /// of a function. The first element will be the return value, unless the
  /// return value is void.
  StructType *makeArgStructType(Function *Func) {
    StructType *WrapTy = NULL;
    std::vector<Type *> StructContent;

    if (Func->getFunctionType()->getReturnType() !=
        Type::getVoidTy(Func->getContext())) {
      StructContent.push_back(Func->getFunctionType()->getReturnType());
    }

    for (auto param : Func->getFunctionType()->params()) {
      StructContent.push_back(param);
    }

    WrapTy = StructType::create(ArrayRef<Type *>(StructContent));
    return WrapTy;
  }

  /// Declare, construct, and return a Function wrapped to be launched by a
  /// pthread.
  Function *makeWrapperFunction(Module *M, Function *Func, StructType *WrapTy) {
    // Actually make the function
    Type *Arg[1] = {Type::getInt8PtrTy(M->getContext())};
    FunctionType *FuncTy =
        FunctionType::get(Type::getInt8PtrTy(M->getContext()), Arg, false);
    Function *WrappedFunc =
        Function::Create(FuncTy, Function::ExternalLinkage,
                         "hlir.pthread.wrapped." + Func->getName(), M);

    BasicBlock *Block =
        BasicBlock::Create(Func->getContext(), "entry", WrappedFunc, 0);
    IRBuilder<> B(Block);
    std::vector<Value *> UnpackedArgs;
    Value *PackedArg;

    // Get the input struct
    AllocaInst *PtrArg = B.CreateAlloca(Type::getInt8PtrTy(M->getContext()));
    B.CreateStore(WrappedFunc->arg_begin(), PtrArg);
    PackedArg = B.CreateLoad(PtrArg);
    PackedArg = B.CreateBitCast(PackedArg, PointerType::get(WrapTy, 0));

    int ElemIndex = 0;
    for (auto _ : WrapTy->elements().slice(1)) {
      // Those types are NECESSARY
      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(Func->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(Func->getContext()),
                           ElemIndex + 1)};
      Value *ElemPtr = B.CreateGEP(PackedArg, GEPIndex);
      // Value *Elem = B.CreateLoad(ElemPtr);
      UnpackedArgs.push_back(B.CreateLoad(ElemPtr));
      ElemIndex++;
    }

    // Call The function we are wrapping
    Value *RetVal = B.CreateCall(Func, ArrayRef<Value *>(UnpackedArgs));

    // If need be, store the return.
    if (StructType::isValidElementType(Func->getReturnType())) {
      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(Func->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(Func->getContext()), 0)};
      B.CreateStore(RetVal, B.CreateGEP(PackedArg, GEPIndex));
    }

    B.CreateRet(ConstantPointerNull::get(Type::getInt8PtrTy(M->getContext())));
    return WrappedFunc;
  }

  bool LowerLaunchCall(Module *M, CallInst *I) {
    IRBuilder<> B(I);
    Function *F = I->getCalledFunction();
    Function *WrappedF;
    StructType *WrappedArgTy;

    // Get wrapped function, or wrap function if it does not currently have
    // a wrapped form
    if (this->FuncToWrapFunc.find(F) == this->FuncToWrapFunc.end()) {
      WrappedArgTy = GetFuncStruct(F);
      this->FuncToStruct.emplace(F, WrappedArgTy);
      this->FuncToWrapFunc.emplace(F, makeWrapperFunction(M, F, WrappedArgTy));
    }

    WrappedF = this->FuncToWrapFunc[F];
    WrappedArgTy = this->GetFuncStruct(F);
    if (!this->pthread_create) {
      initPthreadCreate(M);
    }

    // Alloc a thread, pack args, launch pthread, remove old instruction.
    Value *ThreadPtr = B.CreateAlloca(Type::getInt64Ty(M->getContext()));
    Value *ArgPtr = B.CreateAlloca(WrappedArgTy);
    int ArgId = 0;
    for (auto &Arg : I->arg_operands()) {
      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(M->getContext()), ArgId + 1)};
      B.CreateStore(Arg, B.CreateGEP(ArgPtr, GEPIndex));
      ArgId++;
    }

    Value *PThreadArgs[4] = {
        ThreadPtr, ConstantPointerNull::get(PthreadAttrPtrTy), WrappedF,
        B.CreateBitCast(ArgPtr, Type::getInt8PtrTy(M->getContext()))};
    B.CreateCall(this->pthread_create, PThreadArgs);

    // Before we erase I, we need to find the first of use of it, and invoke
    // a force. We need to keep in mind that this may be in another block...
    // Not... not really sure what to do if that is the case? Could join
    // for EVERY possible place?
    // FOR NOW I ASSUME THAT IT CAN ONLY FORCE WITHIN THIS BB
    // Note: I'm not enforcing that... I'm assuming it.
    for (User *U : I->users()) {
      if (Instruction *Inst = dyn_cast<Instruction>(U)) {
        // This builder inserts before the first use.
        IRBuilder<> ForceRet(Inst);

        // FIRST, wait for the thread
        Value *JoinArgs[2] = {ForceRet.CreateLoad(ThreadPtr),
                              ConstantPointerNull::get(PointerType::get(
                                  Type::getInt8PtrTy(M->getContext()), 0))};
        ForceRet.CreateCall(this->pthread_join, JoinArgs);

        // NEXT: get the ret out of the struct
        // (I know the return is there because there was a use.)
        Value *GEPIndex[2] = {
            ConstantInt::get(Type::getInt64Ty(M->getContext()), 0),
            ConstantInt::get(Type::getInt32Ty(M->getContext()), 0)};
        Value *RetVal =
            ForceRet.CreateLoad(ForceRet.CreateGEP(ArgPtr, GEPIndex));

        // LAST: Replace all uses with the true return value
        I->replaceAllUsesWith(RetVal);
        break;
      }
    }

    I->eraseFromParent();
    return true;
  }

public:
  static char ID;
  HLIRLowerPThread()
      : HLIRLower(ID), pthread_create(nullptr), pthread_exit(nullptr),
        pthread_join(nullptr){};

}; // class HLIRLower
} // namespace

char HLIRLowerPThread::ID = 0;
static RegisterPass<HLIRLowerPThread> X("hlir.pthread",
                                        "Lower HLIR to LLIR with pthreads");
