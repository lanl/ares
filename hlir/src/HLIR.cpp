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

  std::string createName(const std::string& prefix){
    return prefix + toStr(createId());
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
  b.SetInsertPoint(marker);      

  Function* createSynchFunc = getFunction("__ares_queue_func", {i32Ty});

  Function* queueFunc = getFunction("__ares_queue_func",
                                    {voidPtrTy, voidPtrTy, i32Ty, i32Ty});

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

  b.CreateCall(queueFunc, {b.CreateBitCast(bodyFunc, voidPtrTy),
                           synchPtr, index, one});

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
  
  bodyFunc->dump();
  
  //cerr << "---------- final module" << endl;
  //module_->dump();  
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
    
  TypeVec fields = {module_->voidPtrTy, module_->i32Ty};
  StructType* argsType = StructType::create(c, fields, "struct.func_args");
    
  Value* argsPtr = b.CreateBitCast(argsVoidPtr, llvm::PointerType::get(argsType, 0));
  Value* synchPtr = b.CreateStructGEP(nullptr, argsPtr, 0);
  Value* indexPtr = b.CreateStructGEP(nullptr, argsPtr, 1);
    
  b.CreateCall(finishFunc, {synchPtr});

  (*this)["index"] = HLIRValue(indexPtr);
  (*this)["insertion"] = HLIRInstruction(ReturnInst::Create(c, entry)); 

  HLIRFunction f(func);
  (*this)["body"] = f;  
}

HLIRFunction& HLIRParallelFor::body(){
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
