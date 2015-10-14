/*
 * ###########################################################################
 * Copyright (c) 2015, Los Alamos National Security, LLC.
 * All rights reserved.
 *
 *  Copyright 2015. Los Alamos National Security, LLC. This software was
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

HLIRModule* HLIRModule::createModule(Module* module){
  Guard guard(_mutex);

  auto m = new HLIRModule(module);
  m->setName(createName("module"));
  _moduleMap[module] = m;
  _moduleNameMap[m->name()] = m;
  return m;
}

HLIRModule* HLIRModule::getModule(const string& name){
  Guard guard(_mutex);

  auto itr = _moduleNameMap.find(name);
  if(itr == _moduleNameMap.end()){
    return nullptr;
  }

  return itr->second;
}

HLIRModule* HLIRModule::getModule(Module* module){
  Guard guard(_mutex);

  auto itr = _moduleMap.find(module);
  if(itr == _moduleMap.end()){
    return nullptr;
  }

  return itr->second;
}

HLIRParallelFor* HLIRModule::createParallelFor(){
  string name = createName("pfor");
  auto pf = new HLIRParallelFor(this);
  pf->setName(name);

  (*this)[name] = pf;
  
  return pf;
}

HLIRParallelFor::HLIRParallelFor(HLIRModule* module)
  : module_(module){

  auto& b = module_->builder();
  auto& c = module_->context();

  TypeVec params = {module_->i32Ty};
  
  auto funcType = FunctionType::get(module_->voidTy, params, false);

  Function* func =
    Function::Create(funcType,
                     llvm::Function::ExternalLinkage,
                     "hlir.parallel_for.body",
                     module_->module());

  auto aitr = func->arg_begin();
  aitr->setName("i");

  BasicBlock* entry = BasicBlock::Create(c, "entry", func);
  b.SetInsertPoint(entry);
  Instruction* index = b.CreateAlloca(module_->i32Ty, 0, "index.ptr");
  b.CreateStore(aitr, index);

  (*this)["index"] = HLIRValue(index); 
  (*this)["insertion"] = HLIRInstruction(ReturnInst::Create(c, entry)); 

  HLIRFunction f(func);
  (*this)["body"] = f;  
}

HLIRFunction& HLIRParallelFor::body(){
  return get<HLIRFunction>("body");
}

