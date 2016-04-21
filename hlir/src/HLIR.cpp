/*
 * ###########################################################################
 * Copyright (c) 2015-2016, Los Alamos National Security, LLC.
 * All rights reserved.
 *
 *  Copyright 2015-2016. Los Alamos National Security, LLC. This software was
 *  produced under U.S. Government contract ??? (LA-CC-15-056) for Los
 *  Alamos National Laboratory (LANL), which is operated by Los Alamos
 *  National Security, LLC for the U.S. Department of Energy. The
 *  U.S. Government has rights to use, reproduce, and distribute this
 *  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY,
 *  LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY
 *  FOR THE USE OF THIS SOFTWARE.  If software is modified to produce
 *  derivative works, such modified software should be clearly marked,
 *  so as not to confuse it with the version available from LANL.
 *
 *  Additionally, redistribution and use in source and binary forms,
 *  with or without modification, are permitted provided that the
 *  following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Los Alamos National Security, LLC, Los
 *      Alamos National Laboratory, LANL, the U.S. Government, nor the
 *      names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 * ###########################################################################
 *
 * Notes
 *
 * #####
 */

#include "hlir/HLIR.h"

#include <mutex>

using namespace std;
using namespace llvm;
using namespace ares;

namespace{

  using TypeVec = vector<Type*>;

  using ValueVec = vector<Value*>;

  mutex _mutex;

  map<string, HLIRModule*> _moduleNameMap;

  map<Module*, HLIRModule*> _moduleMap;

  size_t _nextId = 0;

  using Guard = lock_guard<mutex>;

  size_t createId(){
    return _nextId++;
  }

  string createName(const string& prefix){
    return prefix + toStr(createId());
  }

  void findExternalValues(Function* f, vector<Instruction*>& v){
    for(BasicBlock& bi : *f){
      for(Instruction& ii : bi){
        for(Value* vi : ii.operands()){
          if(Instruction* ij = dyn_cast<Instruction>(vi)){
            if(ij->getParent()->getParent() != f){
              v.push_back(ij);
            }
          }
        }
      }
    }
  }

} // namespace

HLIRModule* HLIRModule::getModule(Module* module){
  Guard guard(_mutex);

  auto itr = _moduleMap.find(module);
  if(itr == _moduleMap.end()){
    auto m = new HLIRModule(module);
    m->setName(createName("module"));
    _moduleMap[module] = m;
    _moduleNameMap[m->name()] = m;
    return m;
  }

  return itr->second;
}

HLIRParallelFor* HLIRModule::createParallelFor(){
  auto pf = new HLIRParallelFor(this);
  constructs_.push_back(pf);

  string name = createName("pfor");
  pf->setName(name);

  (*this)[name] = pf;
  
  return pf;
}

HLIRParallelReduce* HLIRModule::createParallelReduce(
  const HLIRType& reduceType){
  
  auto r = new HLIRParallelReduce(this, reduceType);
  constructs_.push_back(r);

  string name = createName("reduce");
  r->setName(name);

  (*this)[name] = r;
  
  return r;
}

HLIRTask* HLIRModule::createTask(){
  auto task = new HLIRTask(this);
  constructs_.push_back(task);

  string name = createName("task");
  task->setName(name);

  (*this)[name] = task;
  
  return task;
}

void HLIRModule::lowerParallelFor_(HLIRParallelFor* pf){
  //cerr << "---------- module before" << endl;
  //module_->dump();
  
  auto marker = pf->get<HLIRInstruction>("marker");
  BasicBlock* block = marker->getParent();
  Function* func = block->getParent();
  
  Value* one = ConstantInt::get(i32Ty, 1);      
  
  auto& c = module_->getContext();

  IRBuilder<> b(c);

  TypeVec fields;

  vector<Instruction*> v;
  findExternalValues(pf->body(), v);
  for(Instruction* vi : v){
    fields.push_back(vi->getType());    
  }

  StructType* argsType = StructType::create(c, fields, "struct.func_args");

  auto argsInsertion = pf->get<HLIRInstruction>("argsInsertion");
  b.SetInsertPoint(argsInsertion); 

  Value* argsStructPtr = 
    b.CreateBitCast(pf->args(), PointerType::get(argsType, 0));

  size_t i = 0;
  for(Instruction* vi : v){
    Value* gi = b.CreateStructGEP(argsType, argsStructPtr, i);
    Value* ri = b.CreateLoad(gi, vi->getName());
    
    for(Use& u : vi->uses()){
      User* user = u.getUser();
      auto inst = dyn_cast<Instruction>(user);
      if(inst && inst->getParent()->getParent() == pf->body()){
        user->replaceUsesOfWith(vi, ri);
      }
    }

    ++i;
  }

  b.SetInsertPoint(marker);      

  Value* argsPtr = b.CreateAlloca(argsType);

  for(size_t i = 0; i < v.size(); ++i){
    Value* vi = v[i];
    Value* pi = b.CreateStructGEP(argsType, argsPtr, i);
    b.CreateStore(vi, pi);    
  }

  Function* createSynchFunc = 
    getFunction("__ares_create_synch", {i32Ty}, voidPtrTy);

  Function* queueFunc = 
  getFunction("__ares_queue_func",
              {voidPtrTy, voidPtrTy, voidPtrTy, i32Ty, i32Ty});

  Function* awaitFunc = getFunction("__ares_await_synch", {voidPtrTy});

  FunctionType* ft =
    FunctionType::get(voidTy, {voidPtrTy}, false);

  auto r = pf->range();
  Value* start = toInt32(r[0]->as<HLIRInteger>());
  Value* end = toInt32(r[1]->as<HLIRInteger>());

  Value* n = b.CreateSub(end, start, "n");

  Value* synchPtr = b.CreateCall(createSynchFunc, {n}, "synch.ptr");

  Value* indexPtr = b.CreateAlloca(i32Ty, nullptr, "index.ptr");
  b.CreateStore(start, indexPtr);

  BasicBlock* loopBlock = BasicBlock::Create(c, "pfor.queue.loop", func);
  b.CreateBr(loopBlock);
  b.SetInsertPoint(loopBlock);

  Value* bodyFunc = pf->body();
  
  Value* index = b.CreateLoad(indexPtr);

  b.CreateCall(queueFunc, {synchPtr,
                           b.CreateBitCast(argsPtr, voidPtrTy),
                           b.CreateBitCast(bodyFunc, voidPtrTy),
                           index, one});

  Value* nextIndex = b.CreateAdd(index, one);
  
  b.CreateStore(nextIndex, indexPtr);
  
  Value* cond = b.CreateICmpULT(nextIndex, end);
  
  BasicBlock* exitBlock = BasicBlock::Create(c, "pfor.queue.exit", func);
  
  b.CreateCondBr(cond, loopBlock, exitBlock);
  
  BasicBlock* blockAfter = block->splitBasicBlock(*marker, "pfor.merge");

  block->getTerminator()->removeFromParent();
  
  marker->removeFromParent();
  
  b.SetInsertPoint(exitBlock);

  b.CreateCall(awaitFunc, {synchPtr});
  
  b.CreateBr(blockAfter);
  
  //bodyFunc->dump();
  
  //func->dump();

  //cerr << "---------- final module" << endl;
  //module_->dump();  
}

void HLIRModule::lowerParallelReduce_(HLIRParallelReduce* r){
  using ValueVec = vector<Value*>;
  using TypeVec = vector<llvm::Type*>;

  auto marker = r->get<HLIRInstruction>("marker");

  LLVMContext& c = module_->getContext();
  IRBuilder<> b(c);

  Type* rt = r->reduceType();

  TypeVec params = {voidPtrTy};

  auto bft = FunctionType::get(rt, params, false);

  params = {voidPtrTy};

  auto ft = FunctionType::get(voidTy, params, false);

  auto func =
    Function::Create(ft,
                     llvm::Function::ExternalLinkage,
                     "reduce",
                     module_);

  auto aitr = func->arg_begin();
  aitr->setName("args.ptr");
  Value* argsVoidPtr = aitr;

  BasicBlock* block = BasicBlock::Create(c, "entry", func);

  b.SetInsertPoint(block);

  TypeVec fields;
  fields.push_back(PointerType::get(rt, 0));
  fields.push_back(voidPtrTy);
  fields.push_back(voidPtrTy);
  fields.push_back(i32Ty);
  fields.push_back(i32Ty);
  fields.push_back(i32Ty);
  fields.push_back(PointerType::get(bft, 0));
  fields.push_back(voidPtrTy);

  StructType* argsType = StructType::create(c, fields, "struct.args");

  Type* argsPtrType = PointerType::get(argsType, 0);

  Value* argsPtr = b.CreateBitCast(argsVoidPtr, argsPtrType);

  Value* partialSums = b.CreateStructGEP(nullptr, argsPtr, 0);
  partialSums = b.CreateLoad(partialSums);

  Value* barrier = b.CreateStructGEP(nullptr, argsPtr, 1);
  barrier = b.CreateLoad(barrier);

  Value* synch = b.CreateStructGEP(nullptr, argsPtr, 2);
  synch = b.CreateLoad(synch);

  Value* threadIndex = b.CreateStructGEP(nullptr, argsPtr, 3);
  threadIndex = b.CreateLoad(threadIndex);

  Value* numThreads = b.CreateStructGEP(nullptr, argsPtr, 4);
  numThreads = b.CreateLoad(numThreads);

  Value* size = b.CreateStructGEP(nullptr, argsPtr, 5);
  size = b.CreateLoad(size);

  Value* bodyFunc = b.CreateStructGEP(nullptr, argsPtr, 6);
  bodyFunc = b.CreateLoad(bodyFunc);

  Value* bodyArgs = b.CreateStructGEP(nullptr, argsPtr, 7);
  bodyArgs = b.CreateLoad(bodyArgs);

  Value* zero = ConstantInt::get(i32Ty, 0);
  Value* one = ConstantInt::get(i32Ty, 1);
  Value* two = ConstantInt::get(i32Ty, 2);

  Value* q = b.CreateUDiv(size, numThreads);

  Value* start = b.CreateMul(q, threadIndex);

  Value* cond = b.CreateICmpEQ(threadIndex, b.CreateSub(numThreads, one));

  Value* end = 
    b.CreateSelect(cond, size, b.CreateMul(q, b.CreateAdd(threadIndex, one)));

  Value* rptr = b.CreateAlloca(rt);

  Value* initVal;

  if(rt->isFloatingPointTy()){
    if(r->sum()){
      initVal = ConstantFP::get(rt, 0.0);
    }
    else{
      initVal = ConstantFP::get(rt, 1.0);
    }
  }
  else{
    if(r->sum()){
      initVal = ConstantInt::get(rt, 0);
    }
    else{
      initVal = ConstantInt::get(rt, 1);
    }
  }

  b.CreateStore(initVal, rptr);

  Value* iPtr = b.CreateAlloca(i32Ty);

  b.CreateStore(start, iPtr);

  BasicBlock* condBlock5 = BasicBlock::Create(c, "cond.block", func);
  b.CreateBr(condBlock5);

  b.SetInsertPoint(condBlock5);

  Value* i = b.CreateLoad(iPtr);

  Value* endCond = b.CreateICmpULT(i, end);

  BasicBlock* loopBlock5 = BasicBlock::Create(c, "loop.block", func);

  BasicBlock* mergeBlock5 = BasicBlock::Create(c, "merge.block", func);

  b.CreateCondBr(endCond, loopBlock5, mergeBlock5);

  b.SetInsertPoint(loopBlock5);

  Value* rv = b.CreateLoad(rptr);

  ValueVec args2 = {bodyArgs};

  Value* bi = b.CreateCall(bodyFunc, args2);

  Value* si;

  if(rt->isFloatingPointTy()){
    if(r->sum()){
      si = b.CreateFAdd(rv, bi);
    }
    else{
      si = b.CreateFMul(rv, bi);
    }
  }
  else{
    if(r->sum()){
      si = b.CreateAdd(rv, bi);
    }
    else{
      si = b.CreateMul(rv, bi);
    }
  }

  b.CreateStore(si, rptr);

  b.CreateBr(condBlock5);

  b.SetInsertPoint(mergeBlock5);

  Function* barrierFunc = getFunction("__ares_wait_barrier", {voidPtrTy});

  ValueVec args = {barrier};

  b.CreateCall(barrierFunc, args);

  Value* p2Ptr = b.CreateAlloca(i32Ty);

  b.CreateStore(two, p2Ptr);

  b.CreateStore(one, iPtr);

  BasicBlock* condBlock = BasicBlock::Create(c, "cond.block", func);
  b.CreateBr(condBlock);

  b.SetInsertPoint(condBlock);

  i = b.CreateLoad(iPtr);

  Value* cond2 = b.CreateICmpULE(i, numThreads);

  BasicBlock* loopBlock = BasicBlock::Create(c, "loop.block", func);

  BasicBlock* mergeBlock = BasicBlock::Create(c, "merge.block", func);

  b.CreateCondBr(cond2, loopBlock, mergeBlock);

  b.SetInsertPoint(loopBlock);

  Value* p2 = b.CreateLoad(p2Ptr);

  Value* mod = b.CreateURem(threadIndex, p2);

  Value* cond3 = b.CreateICmpEQ(mod, zero);

  Value* ti1 = b.CreateAdd(threadIndex, i);

  Value* cond4 = b.CreateICmpULT(ti1, numThreads);

  Value* cond5 = b.CreateAnd(cond3, cond4);

  BasicBlock* condBlock2 = BasicBlock::Create(c, "cond.block", func);

  BasicBlock* mergeBlock2 = BasicBlock::Create(c, "merge.block", func);

  b.CreateCondBr(cond5, condBlock2, mergeBlock2);

  b.SetInsertPoint(condBlock2);

  Value* idx1 = b.CreateGEP(partialSums, threadIndex);
  Value* idx2 = b.CreateGEP(partialSums, ti1);
  Value* v1 = b.CreateLoad(idx1);
  Value* v2 = b.CreateLoad(idx2);
  Value* va;

  if(rt->isFloatingPointTy()){
    va = b.CreateFAdd(v1, v2);
  }
  else{
    va = b.CreateAdd(v1, v2);
  }

  b.CreateStore(va, idx1);

  b.CreateBr(mergeBlock2);

  b.SetInsertPoint(mergeBlock2);

  p2 = b.CreateMul(p2, two);
  b.CreateStore(p2, p2Ptr);

  b.CreateCall(barrierFunc, args);

  i = b.CreateMul(i, two);
  b.CreateStore(i, iPtr);

  b.CreateBr(condBlock);

  b.SetInsertPoint(mergeBlock);

  Function* awaitFunc = getFunction("__ares_await_synch", {voidPtrTy});
  args = {synch};

  b.CreateCall(awaitFunc, args);

  b.CreateRetVoid();

  TypeVec captureFields;

  vector<Instruction*> v;
  findExternalValues(r->body(), v);
  for(Instruction* vi : v){
    captureFields.push_back(vi->getType());    
  }

  Type* captureArgsType = StructType::create(c, captureFields, "struct.func_args");

  auto argsInsertion = r->get<HLIRInstruction>("argsInsertion");
  b.SetInsertPoint(argsInsertion); 

  Value* argsStructPtr = 
    b.CreateBitCast(r->args(), PointerType::get(captureArgsType, 0));

  size_t j = 0;
  for(Instruction* vi : v){
    Value* gi = b.CreateStructGEP(captureArgsType, argsStructPtr, j);
    Value* ri = b.CreateLoad(gi, vi->getName());
    
    for(Use& u : vi->uses()){
      User* user = u.getUser();
      auto inst = dyn_cast<Instruction>(user);
      if(inst && inst->getParent()->getParent() == r->body()){
        user->replaceUsesOfWith(vi, ri);
      }
    }

    ++j;
  }

  b.SetInsertPoint(marker);

  Function* parentFunc = marker->getParent()->getParent();

  Value* captureArgsPtr = b.CreateAlloca(captureArgsType);

  for(size_t i = 0; i < v.size(); ++i){
    Value* vi = v[i];
    Value* pi = b.CreateStructGEP(captureArgsType, captureArgsPtr, i);
    b.CreateStore(vi, pi);    
  }

  Function* createSynchFunc = 
    getFunction("__ares_create_synch", {i32Ty}, voidPtrTy);

  Function* createBarrierFunc = 
    getFunction("__ares_create_barrier", {i32Ty}, voidPtrTy);

  Function* queueFunc = 
  getFunction("__ares_queue_func",
              {voidPtrTy, voidPtrTy, voidPtrTy, i32Ty, i32Ty});

  ft = FunctionType::get(voidTy, {voidPtrTy}, false);

  auto range = r->range();
  start = toInt32(range[0]->as<HLIRInteger>());
  end = toInt32(range[1]->as<HLIRInteger>());

  Value* n = b.CreateSub(end, start, "n");

  Value* synchPtr = b.CreateCall(createSynchFunc, {n}, "synch.ptr");

  Value* barrierPtr = b.CreateCall(createSynchFunc, {n}, "synch.ptr");

  Value* indexPtr = b.CreateAlloca(i32Ty, nullptr, "index.ptr");
  b.CreateStore(start, indexPtr);

  numThreads = ConstantInt::get(i32Ty, 8);

  Value* partialSumsPtr = b.CreateAlloca(rt, numThreads);

  loopBlock = BasicBlock::Create(c, "pfor.queue.loop", parentFunc);
  b.CreateBr(loopBlock);
  b.SetInsertPoint(loopBlock);

  Value* reduceArgs = b.CreateAlloca(argsType);

  Value* index = b.CreateLoad(indexPtr);

  Value* argsIdx = b.CreateStructGEP(argsType, reduceArgs, 0);
  b.CreateStore(partialSumsPtr, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 1);
  b.CreateStore(barrierPtr, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 2);
  b.CreateStore(synchPtr, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 3);
  b.CreateStore(index, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 4);
  b.CreateStore(numThreads, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 5);
  b.CreateStore(n, argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 6);
  b.CreateStore(r->body(), argsIdx);

  argsIdx = b.CreateStructGEP(argsType, reduceArgs, 7);
  Value* captureArgsVoidPtr = b.CreateBitCast(captureArgsPtr, voidPtrTy);
  b.CreateStore(captureArgsVoidPtr, argsIdx);

  b.CreateCall(queueFunc, {synchPtr,
                           b.CreateBitCast(reduceArgs, voidPtrTy),
                           b.CreateBitCast(r->body(), voidPtrTy),
                           index, one});

  Value* nextIndex = b.CreateAdd(index, one);
  
  b.CreateStore(nextIndex, indexPtr);
  
  cond = b.CreateICmpULT(nextIndex, end);
  
  BasicBlock* exitBlock = BasicBlock::Create(c, "pfor.queue.exit", parentFunc);
  
  b.CreateCondBr(cond, loopBlock, exitBlock);

  block = marker->getParent();
  
  BasicBlock* blockAfter = block->splitBasicBlock(*marker, "pfor.merge");

  block->getTerminator()->removeFromParent();
  
  marker->removeFromParent();
  
  b.SetInsertPoint(exitBlock);

  b.CreateCall(awaitFunc, {synchPtr});
  
  b.CreateBr(blockAfter);

  //func->dump();
}

void HLIRModule::lowerTask_(HLIRTask* task){
  auto& b = builder();
  auto& c = context();

  DataLayout layout(module_);

  Function* func = task->function();
  Function* wrapperFunc = task->wrapperFunction();

  for(auto itr = func->use_begin(), itrEnd = func->use_end();
    itr != itrEnd; ++itr){
    if(CallInst* ci = dyn_cast<CallInst>(itr->getUser())){
      BasicBlock* parentBlock = ci->getParent();
      Function* parentFunc = parentBlock->getParent();
      
      if(parentFunc == wrapperFunc){
        continue;
      }

      b.SetInsertPoint(ci);

      TypeVec fields;
      fields.push_back(voidPtrTy);
      fields.push_back(i32Ty);
      fields.push_back(func->getReturnType());

      for(auto pitr = func->arg_begin(), pitrEnd = func->arg_end();
        pitr != pitrEnd; ++pitr){
        fields.push_back(pitr->getType());
      }

      StructType* argsType = StructType::create(c, fields, "struct.func_args");

      size_t size = layout.getTypeAllocSize(argsType);

      Function* allocFunc = getFunction("__ares_alloc", {i64Ty}, voidPtrTy);

      ValueVec args = {ConstantInt::get(i64Ty, size)};

      Value* argsVoidPtr = b.CreateCall(allocFunc, args, "args.void.ptr");

      Value* argsPtr = 
        b.CreateBitCast(argsVoidPtr, PointerType::get(argsType, 0), "args.ptr");

      Value* depth = b.CreateStructGEP(nullptr, argsPtr, 1, "depth.ptr");
      depth = b.CreateLoad(depth, "depth");

      size_t idx = 3;
      for(auto& arg : ci->arg_operands()){
        Value* argPtr = b.CreateStructGEP(nullptr, argsPtr, idx, "arg.ptr");
        b.CreateStore(arg, argPtr);
        ++idx;
      }

      Function* queueFunc = 
        getFunction("__ares_task_queue", {voidPtrTy, voidPtrTy});

      Value* funcVoidPtr = b.CreateBitCast(wrapperFunc, voidPtrTy, "funcVoidPtr");

      args = {funcVoidPtr, argsVoidPtr};
      b.CreateCall(queueFunc, args);

      for(auto itr = ci->use_begin(), itrEnd = ci->use_end();
        itr != itrEnd; ++itr){

        if(Instruction* i = dyn_cast<Instruction>(itr->getUser())){
          b.SetInsertPoint(i);

          Function* awaitFunc = 
            getFunction("__ares_task_await_future", {voidPtrTy});

          args = {argsVoidPtr};
          b.CreateCall(awaitFunc, args);

          Value* retPtr = b.CreateStructGEP(nullptr, argsPtr, 2, "retPtr");
          Value* retVal = b.CreateLoad(retPtr, "retVal"); 

          ci->replaceAllUsesWith(retVal);

          break;
        }
      }

      ci->eraseFromParent();

      //parentFunc->dump();
    }
  }
}

bool HLIRModule::lowerToIR_(){
  for(HLIRConstruct* c : constructs_){
    if(auto pfor = dynamic_cast<HLIRParallelFor*>(c)){
      lowerParallelFor_(pfor);
    }
    else if(auto r = dynamic_cast<HLIRParallelReduce*>(c)){
      lowerParallelReduce_(r);
    }
    else if(auto task = dynamic_cast<HLIRTask*>(c)){
      lowerTask_(task);
    }
    else{
      assert(false && "unknown HLIR construct");
    }
  }

  return true;
}

HLIRParallelFor::HLIRParallelFor(HLIRModule* module)
  : HLIRConstruct(module){

  auto& b = module_->builder();
  auto& c = module_->context();
    
  TypeVec params = {module_->voidPtrTy};
  
  auto funcType = FunctionType::get(module_->voidTy, params, false);

  Function* func =
    Function::Create(funcType,
                     llvm::Function::ExternalLinkage,
                     "hlir.parallel_for.body",
                     module_->module());

  Function* finishFunc = 
    module_->getFunction("__ares_finish_func", {module_->voidPtrTy});

  auto aitr = func->arg_begin();
  aitr->setName("args.ptr");
  Value* argsVoidPtr = aitr++;
    
  BasicBlock* entry = BasicBlock::Create(c, "entry", func);
  b.SetInsertPoint(entry);
    
  TypeVec fields = {module_->voidPtrTy, module_->i32Ty, module_->voidPtrTy};
  StructType* argsType = StructType::create(c, fields, "struct.func_args");
    
  Value* argsPtr = b.CreateBitCast(argsVoidPtr, llvm::PointerType::get(argsType, 0), "args.ptr");

  Value* synchPtr = b.CreateStructGEP(argsType, argsPtr, 0);
  synchPtr = b.CreateLoad(synchPtr, "synch.ptr");

  Value* indexPtr = b.CreateStructGEP(argsType, argsPtr, 1, "index.ptr");
  
  Value* funcArgsPtr = b.CreateStructGEP(argsType, argsPtr, 2, "funcArgs.ptr");
  funcArgsPtr = b.CreateLoad(funcArgsPtr);
   
  Instruction* placeholder = module_->createNoOp();

  Value* synchVoidPtr = b.CreateBitCast(synchPtr, module_->voidPtrTy);

  b.CreateCall(finishFunc, {argsVoidPtr});

  (*this)["index"] = HLIRValue(indexPtr);
  (*this)["insertion"] = HLIRInstruction(ReturnInst::Create(c, entry)); 
  (*this)["args"] = HLIRValue(funcArgsPtr);
  (*this)["argsInsertion"] = HLIRInstruction(placeholder); 

  HLIRFunction f(func);
  (*this)["body"] = f;  
}

HLIRFunction& HLIRParallelFor::body(){
  return get<HLIRFunction>("body");
}

HLIRParallelReduce::HLIRParallelReduce(HLIRModule* module,
  const HLIRType& reduceType)
  : HLIRConstruct(module){

  auto& b = module_->builder();
  auto& c = module_->context();
    
  TypeVec params = {module_->voidPtrTy};
  
  auto funcType = FunctionType::get(reduceType, params, false);

  Function* func =
    Function::Create(funcType,
                     llvm::Function::ExternalLinkage,
                     "hlir.parallel_reduce.body",
                     module_->module());

  Function* finishFunc = 
    module_->getFunction("__ares_finish_func", {module_->voidPtrTy});

  auto aitr = func->arg_begin();  
  aitr->setName("args.ptr");
  Value* argsVoidPtr = aitr++;
    
  BasicBlock* entry = BasicBlock::Create(c, "entry", func);
  b.SetInsertPoint(entry);

  Instruction* reduceVar = b.CreateAlloca(reduceType);

  TypeVec fields = {module_->voidPtrTy, module_->i32Ty, module_->voidPtrTy};
  StructType* argsType = StructType::create(c, fields, "struct.func_args");
    
  Value* argsPtr = b.CreateBitCast(argsVoidPtr, llvm::PointerType::get(argsType, 0), "args.ptr");

  Value* synchPtr = b.CreateStructGEP(argsType, argsPtr, 0);
  synchPtr = b.CreateLoad(synchPtr, "synch.ptr");

  Value* indexPtr = b.CreateStructGEP(argsType, argsPtr, 1, "index.ptr");
  
  Value* funcArgsPtr = b.CreateStructGEP(argsType, argsPtr, 2, "funcArgs.ptr");
  funcArgsPtr = b.CreateLoad(funcArgsPtr);
   
  Instruction* placeholder = module_->createNoOp();

  Value* synchVoidPtr = b.CreateBitCast(synchPtr, module_->voidPtrTy);

  b.CreateCall(finishFunc, {argsVoidPtr});

  Instruction* retVal = b.CreateLoad(reduceVar);

  Instruction* ret = ReturnInst::Create(c, retVal, entry);

  (*this)["entry"] = HLIRInstruction(reduceVar);
  (*this)["index"] = HLIRValue(indexPtr);
  (*this)["insertion"] = HLIRInstruction(retVal); 
  (*this)["args"] = HLIRValue(funcArgsPtr);
  (*this)["argsInsertion"] = HLIRInstruction(placeholder); 
  (*this)["reduceVar"] = HLIRValue(reduceVar);
  (*this)["reduceType"] = reduceType;

  HLIRFunction f(func);
  (*this)["body"] = f;  
}

HLIRFunction& HLIRParallelReduce::body(){
  return get<HLIRFunction>("body");
}

void HLIRTask::setFunction(const HLIRFunction& func){
  auto& b = module_->builder();
  auto& c = module_->context();

  (*this)["function"] = func;

  TypeVec params = {module_->voidPtrTy};

  auto funcType = FunctionType::get(module_->voidTy, params, false);

  Function* wrapperFunc =
    Function::Create(funcType,
                     llvm::Function::ExternalLinkage,
                     "hlir.task_wrapper",
                     module_->module());

  auto aitr = wrapperFunc->arg_begin();
  aitr->setName("args.ptr");
  Value* argsVoidPtr = aitr++;

  BasicBlock* entry = BasicBlock::Create(c, "entry", wrapperFunc);
  b.SetInsertPoint(entry);

  TypeVec fields;
  fields.push_back(module_->voidPtrTy);
  fields.push_back(module_->i32Ty);
  fields.push_back(func->getReturnType());

  for(auto pitr = func->arg_begin(), pitrEnd = func->arg_end();
    pitr != pitrEnd; ++pitr){
    fields.push_back(pitr->getType());
  }

  StructType* argsType = StructType::create(c, fields, "struct.func_args");

  ValueVec args;
  Value* argsPtr = 
    b.CreateBitCast(argsVoidPtr, PointerType::get(argsType, 0), "argsPtr");

  size_t idx = 3;
  for(auto pitr = func->arg_begin(), pitrEnd = func->arg_end();
    pitr != pitrEnd; ++pitr){
    Value* arg = b.CreateStructGEP(nullptr, argsPtr, idx, "arg.ptr");
    arg = b.CreateLoad(arg, "arg");
    args.push_back(arg);
    ++idx;
  }

  Value* ret = b.CreateCall(func, args, "ret");
  Value* retPtr = b.CreateStructGEP(nullptr, argsPtr, 2, "retPtr");
  b.CreateStore(ret, retPtr);

  Function* releaseFunc = 
    module_->getFunction("__ares_task_release_future", {module_->voidPtrTy});

  args = {argsVoidPtr};
  b.CreateCall(releaseFunc, args);

  b.CreateRetVoid();

  (*this)["wrapperFunction"] = HLIRFunction(wrapperFunc);
}
