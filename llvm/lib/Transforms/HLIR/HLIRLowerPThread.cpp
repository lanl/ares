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
  static const unsigned int RET_OFFSET = 0;
  static const unsigned int TID_OFFSET = 1;
  static const unsigned int SEM_OFFSET = 2;
  static const unsigned int ARG_OFFSET = 3;

  /// Common Functions
  Function *pthread_create;
  Function *pthread_exit;
  Function *pthread_join;

  Function *sem_init;
  Function *sem_wait;
  Function *sem_post;
  Function *sem_destroy;

  /// Common Type
  PointerType *PthreadAttrPtrTy;
  Type *PThreadTy;
  Type *SemTy;

  /// Maps a function to a wrapped version of that function which is
  /// able to be called by llvm.
  std::map<Function *, Function *> FuncToWrapFunc;
  std::map<Function *, StructType *> FuncToStruct;
  std::map<Type *, StructType *> TyToFutureTy;

  /// Maps the original value to its future equivalent, so that futures
  /// can be forced.
  std::map<Instruction *, Value *> OrigToFuture;

  bool InitLower(Module &M) { return false; }

  /// Make a declaration of `pthread_create` if necessary. Returns whether or
  /// not the module was changed.
  ///
  /// TODO: Currently always makes the declaration.
  bool initPthreadCreate(Module *M) {
    /*
      typedef long pthread_t;
     */
    this->PThreadTy = Type::getInt64Ty(M->getContext());

    /*
      struct pthread_attr_t {
        long;
        char[48];
      }
    */
    Type *PThreadAttrs[2] = {
        Type::getInt64Ty(M->getContext()),
        ArrayType::get(Type::getInt8Ty(M->getContext()), 48)};
    this->PthreadAttrPtrTy = PointerType::get(
        StructType::create(PThreadAttrs, "union.pthread_attr_t"), 0);

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
    Type *PthreadArgsTy[4] = {PointerType::get(PThreadTy, 0), PthreadAttrPtrTy,
                              StartFTy, Type::getInt8PtrTy(M->getContext())};
    FunctionType *FTy = FunctionType::get(Type::getInt32Ty(M->getContext()),
                                          PthreadArgsTy, false);
    this->pthread_create =
        Function::Create(FTy, Function::ExternalLinkage, "pthread_create", M);

    /*
     void pthread_exit(void *retval);
    */
    FTy = FunctionType::get(Type::getVoidTy(M->getContext()), VoidPtrArgTy,
                            false);
    this->pthread_exit =
        Function::Create(FTy, Function::ExternalLinkage, "pthread_exit", M);

    /*
      int pthread_join(pthread_t thread, void **retval);
    */
    Type *JoinArgsTy[2] = {
        PThreadTy, PointerType::get(Type::getInt8PtrTy(M->getContext()), 0)};
    FTy =
        FunctionType::get(Type::getInt32Ty(M->getContext()), JoinArgsTy, false);
    this->pthread_join =
        Function::Create(FTy, Function::ExternalLinkage, "pthread_join", M);

    /*
      struct sem_t {
        long;
        char[24];
      };
    */
    Type *SemComps[2] = {Type::getInt64Ty(M->getContext()),
                         ArrayType::get(Type::getInt8Ty(M->getContext()), 24)};
    this->SemTy = StructType::create(SemComps, "union.sem_t");

    /*
      int sem_init(sem_t*, int, unsigned int);
    */
    Type *SemInitArgsTy[3] = {PointerType::get(SemTy, 0),
                              Type::getInt32Ty(M->getContext()),
                              Type::getInt32Ty(M->getContext())};
    FTy = FunctionType::get(Type::getInt32Ty(M->getContext()), SemInitArgsTy,
                            false);
    this->sem_init =
        Function::Create(FTy, Function::ExternalLinkage, "sem_init", M);

    /*
      int sem_wait(sem_t*);
    */
    Type *SemWaitArgsTy[1] = {PointerType::get(SemTy, 0)};
    FTy = FunctionType::get(Type::getInt32Ty(M->getContext()), SemWaitArgsTy,
                            false);
    this->sem_wait =
        Function::Create(FTy, Function::ExternalLinkage, "sem_wait", M);

    /*
      int sem_post(sem_t*);
    */
    Type *SemPostArgsTy[1] = {PointerType::get(SemTy, 0)};
    FTy = FunctionType::get(Type::getInt32Ty(M->getContext()), SemPostArgsTy,
                            false);
    this->sem_post =
        Function::Create(FTy, Function::ExternalLinkage, "sem_post", M);

    /*
      int sem_destroy(sem_t*);
     */
    Type *SemDestroyArgTy[1] = {PointerType::get(SemTy, 0)};
    FTy = FunctionType::get(Type::getInt32Ty(M->getContext()), SemDestroyArgTy,
                            false);
    this->sem_destroy =
        Function::Create(FTy, Function::ExternalLinkage, "sem_destroy", M);

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

    if (this->FuncToStruct.find(F) == this->FuncToStruct.end()) {
      std::vector<Type *> Members;

      if (StructType::isValidElementType(F->getReturnType())) {
        Members.push_back(F->getReturnType());
      } else {
        Members.push_back(Type::getInt32Ty(F->getContext()));
      }

      Members.push_back(this->PThreadTy);
      Members.push_back(this->SemTy);

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

  /// Simply declares a wrapper function for F with an entry block, and
  /// returns it.
  ///
  /// Used almost exclusively by `GetWrapperFunction`.
  Function *DeclareWrapFunc(Function *F, Module *M) {
    Type *Arg[1] = {Type::getInt8PtrTy(F->getContext())};
    Function *WF = Function::Create(
        FunctionType::get(Type::getInt8PtrTy(F->getContext()), Arg, false),
        Function::ExternalLinkage, "hlir.pthread.wrapped." + F->getName(), M);
    BasicBlock::Create(M->getContext(), "entry", WF, 0);

    return WF;
  }

  /// From the first (and only) argument from a wrapper function, unloads
  /// it into stack memory, and bitcasts it to the known struct type.
  ///
  /// TODO: Copy semantics, and copy strait into the correct type.
  ///
  /// Used almost exclusively by `GetWrapperFunction`.
  Value *LoadPackedArgs(Function *WF, StructType *Ty) {
    IRBuilder<> B(&WF->getEntryBlock());

    AllocaInst *PtrArg = B.CreateAlloca(Type::getInt8PtrTy(WF->getContext()));
    B.CreateStore(WF->arg_begin(), PtrArg);
    Value *PackedArgs = B.CreateLoad(PtrArg);
    PackedArgs = B.CreateBitCast(PackedArgs, PointerType::get(Ty, 0));

    return PackedArgs;
  }

  /// Inside a wrapper function, this function will unpack all arguments.
  /// It will then return an ArrayRef of the unpacked args which can be
  /// used for a function call.
  ///
  /// Used almost exclusively by `GetWrapperFunction`.
  std::vector<Value *> UnpackArgs(Function *WF, StructType *WrapTy,
                                  Value *PackedArgs) {
    std::vector<Value *> UnpackedArgs;
    IRBuilder<> B(&WF->getEntryBlock());

    for (unsigned int ElemIndex = ARG_OFFSET;
         ElemIndex < WrapTy->elements().size(); ElemIndex++) {

      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(WF->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(WF->getContext()), ElemIndex)};
      Value *ElemPtr = B.CreateGEP(PackedArgs, GEPIndex);
      UnpackedArgs.push_back(B.CreateLoad(ElemPtr));
    }

    return UnpackedArgs;
  }

  /// Signal parent that this thread has finished unpacking its arguments.
  void UnlockCopyMutex(IRBuilder<> B, Value *TaskPtr) {
    Value *GEPIndex[2] = {
        ConstantInt::get(Type::getInt64Ty(TaskPtr->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(TaskPtr->getContext()), SEM_OFFSET)};

    Value *SemInitArgs[1] = {B.CreateGEP(TaskPtr, GEPIndex)};
    B.CreateCall(this->sem_post, SemInitArgs);
  }

  /// Given a wrapper function and a function to wrap, will actually call the
  /// to-be-wrapped function. If needed, the result will be stored in the first
  /// element to the structure pointed to by RetPtr.
  ///
  /// Used almost exclusively by `GetWrapperFunction`.
  void WrapFuncCall(IRBuilder<> B, Function *F, ArrayRef<Value *> UnpackedArgs,
                    Value *RetPtr) {
    Value *RetVal = B.CreateCall(F, UnpackedArgs);

    // If need be, store the return.
    if (StructType::isValidElementType(F->getReturnType())) {
      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(F->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(F->getContext()), RET_OFFSET)};
      B.CreateStore(RetVal, B.CreateGEP(RetPtr, GEPIndex));
    }
  }

  /// Declare, construct, and return a Function wrapped to be launched by a
  /// pthread. Saves functions in the `FuncToWrapFunc` map.
  ///
  /// Functions will have the following structure.
  ///  void *f(<ret> (*func)(<args>), void* wrap_struct) {
  ///   *wrap_struct = func(wrap_struct.x, wrap_struct.y, ...);
  ///   return 0;
  /// }
  Function *GetWrapperFunction(Module *M, Function *F, StructType *WrapTy) {
    Function *WF;

    if (this->FuncToWrapFunc.find(F) == this->FuncToWrapFunc.end()) {
      // clang-format off
      WF                                = DeclareWrapFunc(F, M);
      Value *PackedArgs                 = LoadPackedArgs(WF, WrapTy);
      std::vector<Value *> UnpackedArgs = UnpackArgs(WF, WrapTy, PackedArgs);
      // clang-format on

      IRBuilder<> B(&WF->getEntryBlock());
      UnlockCopyMutex(B, PackedArgs);
      WrapFuncCall(B, F, ArrayRef<Value *>(UnpackedArgs), PackedArgs);
      B.CreateRet(
          ConstantPointerNull::get(Type::getInt8PtrTy(M->getContext())));

      this->FuncToWrapFunc.emplace(F, WF);
    } else {
      WF = this->FuncToWrapFunc[F];
    }

    return WF;
  }

  Type *GetFutureType(Type *Orig) {
    Type *Ty;

    if (this->TyToFutureTy.find(Orig) == this->TyToFutureTy.end()) {
      std::vector<Type *> Members;

      if (StructType::isValidElementType(Orig)) {
        Members.push_back(Orig);
      } else {
        Members.push_back(Type::getInt32Ty(Orig->getContext()));
      }

      Members.push_back(this->PThreadTy);
      Members.push_back(this->SemTy);
      Ty = StructType::create(ArrayRef<Type *>(Members));

      this->TyToFutureTy.emplace(Orig, Ty);
    } else {
      Ty = this->TyToFutureTy[Orig];
    }

    return Ty;
  }

  /// Given a call instruction, will wrap up the call into a pthread launch.
  ///
  /// TODO: Reduce the number of arguments this takes.
  void LaunchWrapper(CallInst *I, Function *WF, StructType *Ty, Value *ArgPtr,
                     IRBuilder<> &B) {
    int ArgId = 0;
    for (auto &Arg : I->arg_operands()) {
      Value *GEPIndex[2] = {
          ConstantInt::get(Type::getInt64Ty(I->getContext()), 0),
          ConstantInt::get(Type::getInt32Ty(I->getContext()),
                           ArgId + ARG_OFFSET)};
      B.CreateStore(Arg, B.CreateGEP(ArgPtr, GEPIndex));
      ArgId++;
    }

    Value *GEPIndex[2] = {
        ConstantInt::get(Type::getInt64Ty(I->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(I->getContext()), TID_OFFSET)};

    Value *PThreadArgs[4] = {
        B.CreateGEP(ArgPtr, GEPIndex),
        ConstantPointerNull::get(PthreadAttrPtrTy), WF,
        B.CreateBitCast(ArgPtr, Type::getInt8PtrTy(I->getContext()))};
    B.CreateCall(this->pthread_create, PThreadArgs);
  }

  /// Initializes a mutex, representing whether or not the task has finished
  /// copying its arguments.
  void InitCopyMutex(IRBuilder<> B, Value *TaskPtr) {
    Value *GEPIndex[2] = {
        ConstantInt::get(Type::getInt64Ty(TaskPtr->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(TaskPtr->getContext()), SEM_OFFSET)};

    Value *SemInitArgs[3] = {
        B.CreateGEP(TaskPtr, GEPIndex),
        ConstantInt::get(Type::getInt32Ty(TaskPtr->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(TaskPtr->getContext()), 0)};
    B.CreateCall(this->sem_init, SemInitArgs);
  }

  /// Waits for semaphore signal from task, signaling that the arguments have
  // been copied, and that the process may now continue safely.
  void WaitForCopyMutex(IRBuilder<> B, Value *TaskPtr) {
    Value *GEPIndex[2] = {
        ConstantInt::get(Type::getInt64Ty(TaskPtr->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(TaskPtr->getContext()), SEM_OFFSET)};

    Value *SemInitArgs[1] = {B.CreateGEP(TaskPtr, GEPIndex)};
    B.CreateCall(this->sem_wait, SemInitArgs);
  }

  /// Given a function call to lower, will build/lookup a wrapper function, and
  /// launch a pthread instead. Then this function will find all uses of the old
  /// return value, and replace them with futures.
  bool LowerLaunchCall(Module *M, CallInst *I) {
    if (!this->pthread_create) {
      initPthreadCreate(M);
    }

    Function *F = I->getCalledFunction();
    StructType *Ty = GetFuncStruct(F);
    Type *FutureTy = GetFutureType(F->getReturnType());
    Function *WF = GetWrapperFunction(M, F, Ty);

    IRBuilder<> B(I);
    // Value *ThreadPtr = B.CreateAlloca(Type::getInt64Ty(I->getContext()));
    Value *ArgPtr = B.CreateAlloca(Ty);
    Value *Future = B.CreateBitCast(ArgPtr, PointerType::get(FutureTy, 0));

    InitCopyMutex(B, ArgPtr);
    LaunchWrapper(I, WF, Ty, ArgPtr, B);
    WaitForCopyMutex(B, ArgPtr);

    if (I->getNumUses() > 0) {
      this->OrigToFuture[I] = Future;
    }

    return true;
  }

  /// Forces F before instruction I, and replaces the use of O in I with the
  /// new forced value. Will return the new forced value so that it may replace
  /// O in other places.
  Value *AddForceAtUse(Value *O, Value *F, Instruction *I) {
    IRBuilder<> B(I);

    Value *GEPIndex[2] = {
        ConstantInt::get(Type::getInt64Ty(I->getContext()), 0),
        ConstantInt::get(Type::getInt32Ty(I->getContext()), TID_OFFSET)};
    Value *JoinArgs[2] = {B.CreateLoad(B.CreateGEP(F, GEPIndex)),
                          ConstantPointerNull::get(PointerType::get(
                              Type::getInt8PtrTy(I->getContext()), 0))};
    B.CreateCall(this->pthread_join, JoinArgs);

    GEPIndex[1] =
        ConstantInt::get(Type::getInt32Ty(I->getContext()), RET_OFFSET);
    Value *RetVal = B.CreateLoad(B.CreateGEP(F, GEPIndex));

    I->replaceUsesOfWith(O, RetVal);
    return RetVal;
  }

  Value *GetIncomingFuture(PHINode *POrig, unsigned int i) {
    Value *F = nullptr;

    // IM STILL WORKING ON THIS. THE HOPE IS THIS WILL GET A FUTURE VALUE FROM THE ORIGINAL PHI,
    // AND RETURN IT (OR IN THE FUTURE FIX IT IF ITS NOT THERE)
    // THEN, WHEN THIS IS DONE I SHOULD BE ABLE TO MAKE NEW PHI NODES PROPERLY... FINGERS CROSSED!

    if (this->.find(Orig) == this->TyToFutureTy.end()) {

    } else {
    }
    return F;
  }

  bool MakeFuturePhis(Module *M) {
    bool Changed = false;
    std::set<Value *> Origs;
    std::set<PHINode *> PHIs;

    // TODO: This is dumb. I need to spend some time making this more C++y
    for (auto &F : this->OrigToFuture) {
      Origs.insert(F.first);
    }

    PHIs = getFutureUnions(Origs);

    // Add the Phis to our map (and set changed to true)
    for (auto &P : PHIs) {
      Changed = true;
      this->OrigToFuture[P] = PHINode::Create(PointerType::get(P->getType(), 0),
                                              P->getNumIncomingValues(), "", P);
    }

    // Flesh out the new Phi nodes by looking up everything in the OrigToFuture
    // map.
    // Note that this will fail when the future was joined with a non-future. We
    // don't support that yet, but when we do, the code would go here.
    for (auto &P : PHIs) {
      for (unsigned int i = 0; i < P->getNumIncomingValues(); i++) {
        PHINode *FPhi = cast<PHINode>(this->OrigToFuture[P]);

        FPhi->setIncomingValue(i, this->OrigToFuture[P->getIncomingValue(i)]);
        FPhi->setIncomingBlock(i, P->getIncomingBlock(i));
      }
    }

    return Changed;
  }

  bool ForceFutures(Module *M) {
    bool Changed = MakeFuturePhis(M);

    for (auto &Future : this->OrigToFuture) {
      std::vector<Instruction *> Uses;

      for (User *U : Future.first->users()) {
        if (Instruction *Inst = dyn_cast<Instruction>(U)) {
          if (!is_a<PHINode>(U)) {
            Uses.push_back(Inst);
          }
        }
      }

      for (auto Inst : Uses) {
        AddForceAtUse(Future.first, Future.second, Inst);
        Changed |= true;
      }

      Future.first->dump();
      Future.first->eraseFromParent();
    }

    return Changed;
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
