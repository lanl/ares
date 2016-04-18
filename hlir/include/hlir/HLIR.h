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

#ifndef __ARES_HLIR_H__
#define __ARES_HLIR_H__

#include <string>
#include <map>
#include <vector>
#include <cassert>
#include <sstream>
#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "HLIRError.h"

#define ndump(X) std::cout << __FILE__ << ":" << __LINE__ << ": " << \
__PRETTY_FUNCTION__ << ": " << #X << " = " << X << std::endl

#define nlog(X) std::cout << __FILE__ << ":" << __LINE__ << ": " << \
__PRETTY_FUNCTION__ << ": " << X << std::endl

namespace ares{

  template<typename T>
  inline std::string toStr(const T& v){
    std::stringstream ostr;
    ostr << v;
    return ostr.str();
  }

  inline std::string __indentTo(size_t n){
    std::string ret;
    for(size_t i = 0; i < n; ++i){
      ret += "  ";
    }
    return ret;
  }

  class HLIRNode{
  public:
    HLIRNode()
      : parent_(nullptr){}

    virtual ~HLIRNode(){}

    virtual void output(std::ostream& ostr, size_t level=0) const = 0;

    virtual HLIRNode* copy() const = 0;

    std::string str() const{
      std::stringstream ostr;
      output(ostr);
      return ostr.str();
    }

    template<class T>
    T& as(){
      auto r = dynamic_cast<T*>(this);
      if(!r){
        HLIR_ERROR("invalid cast");
      }
      return *r;
    }

    template<class T>
    const T& as() const{
      auto r = dynamic_cast<const T*>(this);
      if(!r){
        HLIR_ERROR("invalid cast");
      }
      return *r;
    }

    virtual bool isRecursive() const{
      return false;
    }

    virtual bool hasValue() const{
      return true;
    }

    void setParent(HLIRNode* parent){
      parent_ = parent;
    }

    HLIRNode* parent() const{
      return parent_;
    }

  private:
    HLIRNode* parent_;
  };

  inline std::ostream& operator<<(std::ostream& ostr, const HLIRNode& n){
    n.output(ostr);
    return ostr;
  }

  template<typename T>
  class HLIRScalar : public HLIRNode{
  public:
    HLIRScalar(){}

    HLIRScalar(const T& value)
      : val_(value){}

    HLIRScalar(const HLIRScalar<T>& value)
      : val_(value.val_){}

    virtual ~HLIRScalar(){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      ostr << val_; 
    }

    virtual HLIRScalar* copy() const override{
      return new HLIRScalar<T>(*this);
    }

    T val() const{
      return val_;
    }

    operator const T&() const{
      return val_;
    }

  protected:
    T val_;
  };

  class HLIRBoolean : public HLIRScalar<bool>{
  public:
    using Super = HLIRScalar<bool>;

    HLIRBoolean(bool value) : Super(value){}

    HLIRBoolean(llvm::Value* value){
      auto ci = llvm::dyn_cast<llvm::ConstantInt>(value);
      if(!ci){
        HLIR_ERROR("not a boolean");
      }
      val_ = ci->getSExtValue();
    }

    virtual HLIRBoolean* copy() const override{
      return new HLIRBoolean(*this);
    }
  };

  class HLIRInteger : public HLIRScalar<int64_t>{
  public:
    using Super = HLIRScalar<int64_t>;

    HLIRInteger(const HLIRInteger& x) : Super(x){}

    template<typename T>
    HLIRInteger(T value) : Super(value){}

    HLIRInteger(llvm::Value* value){
      auto ci = llvm::dyn_cast<llvm::ConstantInt>(value);
      if(!ci){
        HLIR_ERROR("not an integer");
      }
      val_ = ci->getSExtValue();
    }

    virtual HLIRInteger* copy() const override{
      return new HLIRInteger(*this);
    }

    static HLIRInteger nullValue(){
      return std::numeric_limits<int64_t>::max();
    }

    virtual bool hasValue() const override{
      return val_ != std::numeric_limits<int64_t>::max();
    }

  };

  class HLIRFloating : public HLIRScalar<double>{
  public:
    using Super = HLIRScalar<double>;

    HLIRFloating(double value) : Super(value){}

    HLIRFloating(float value) : Super(value){}

    HLIRFloating(llvm::Value* value){
      auto cf = llvm::dyn_cast<llvm::ConstantFP>(value);
      if(!cf){
        HLIR_ERROR("not an floating point value");
      }
      val_ = cf->getValueAPF().convertToDouble();
    }

    static HLIRFloating nullValue(){
      return std::numeric_limits<double>::quiet_NaN();
    }

    virtual bool hasValue() const override{
      return val_ == val_;
    }

  };

  class HLIRString : public HLIRScalar<std::string>{
  public:
    using Super = HLIRScalar<std::string>;

    HLIRString(const HLIRString& x) : Super(x){}

    HLIRString(const std::string& value) : Super(value){}

    HLIRString(const char* value) : Super(value){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      ostr << "\"" << val_ << "\""; 
    }

    virtual HLIRString* copy() const override{
      return new HLIRString(*this);
    }

    static HLIRString nullValue(){
      return "";
    }

    virtual bool hasValue() const override{
      return !val_.empty();
    }
  };

  class HLIRSymbol : public HLIRScalar<std::string>{
  public:
    using Super = HLIRScalar<std::string>;

    HLIRSymbol(const HLIRSymbol& x) : Super(x){}

    HLIRSymbol(const std::string& value) : Super(value){}

    bool operator<(const HLIRSymbol& s) const{
      return val_ < s.val_;
    }

    virtual HLIRSymbol* copy() const override{
      return new HLIRSymbol(*this);
    }

    static HLIRString nullValue(){
      return "";
    }

    virtual bool hasValue() const override{
      return !val_.empty();
    }
  };
  
  template<class T>
  class HLIRContainer : public HLIRNode{
  public:
    HLIRContainer(T* ptr)
      : ptr_(ptr){}

    HLIRContainer(const HLIRContainer& c)
      : ptr_(c.ptr_){}

    T* val() const{
      return ptr_;
    }

    T* operator->(){
      return ptr_;
    }

    const T* operator->() const{
      return ptr_;
    }

    T* operator*(){
      return ptr_;
    }

    const T* operator*() const{
      return ptr_;
    }

    HLIRContainer& operator=(const HLIRContainer& c){
      ptr_ = c.ptr_;
      return *this;
    }

    operator T*(){
      return ptr_;
    }

    operator T*() const{
      return ptr_;
    }

    virtual bool hasValue() const override{
      return ptr_;
    }

  protected:
    T* ptr_;
  };

  class HLIRFunction : public HLIRContainer<llvm::Function>{
  public:
    using Super = HLIRContainer<llvm::Function>;

    HLIRFunction(llvm::Function* function) : Super(function){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string str;
      llvm::raw_string_ostream sstr(str);
      ptr_->print(sstr);
      ostr << "<<<function: " << sstr.str() << ">>>" << std::endl; 
    }

    virtual HLIRFunction* copy() const override{
      return new HLIRFunction(ptr_);
    }

    static HLIRFunction nullValue(){
      return nullptr;
    }

  };

  class HLIRValue : public HLIRContainer<llvm::Value>{
  public:
    using Super = HLIRContainer<llvm::Value>;

    HLIRValue(llvm::Value* value) : Super(value){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string str;
      llvm::raw_string_ostream sstr(str);
      ptr_->print(sstr);
      ostr << "<<<value:" << sstr.str() << ">>>"; 
    }

    virtual HLIRValue* copy() const override{
      return new HLIRValue(ptr_);
    }

    static HLIRValue nullValue(){
      return nullptr;
    }
  };

  class HLIRType : public HLIRContainer<llvm::Type>{
  public:
    using Super = HLIRContainer<llvm::Type>;

    HLIRType(llvm::Type* type) : Super(type){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string str;
      llvm::raw_string_ostream sstr(str);
      ptr_->print(sstr);
      ostr << "<<<type:" << sstr.str() << ">>>"; 
    }

    virtual HLIRType* copy() const override{
      return new HLIRType(ptr_);
    }

    static HLIRType nullValue(){
      return nullptr;
    }
  };

  class HLIRInstruction : public HLIRContainer<llvm::Instruction>{
  public:
    using Super = HLIRContainer<llvm::Instruction>;

    HLIRInstruction(llvm::Instruction* inst) : Super(inst){}

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string str;
      llvm::raw_string_ostream sstr(str);
      ptr_->print(sstr);
      ostr << "<<<instruction:" << sstr.str() << ">>>"; 
    }

    virtual HLIRInstruction* copy() const override{
      return new HLIRInstruction(ptr_);
    }

    static HLIRInstruction nullValue(){
      return nullptr;
    }
  };

  class HLIRNodeFactory{
  public:
    static HLIRNode* create(HLIRNode* node){
      return node;
    }

    static HLIRNode* create(const HLIRNode& node){
      return node.copy();
    }

    template<class T,
             class = typename std::enable_if<
               std::is_integral<T>::value>::type>
    static HLIRNode* create(T value){
      return new HLIRInteger(value);
    }

    static HLIRNode* create(float value){
      return new HLIRFloating(value);
    }

    static HLIRNode* create(double value){
      return new HLIRFloating(value);
    }

    static HLIRNode* create(const char* value){
      if(isSymbol(value)){
        return new HLIRSymbol(value);
      }
      else{
        return new HLIRString(value);
      }
    }

    static HLIRNode* create(const std::string& value){
      if(isSymbol(value)){
        return new HLIRSymbol(value);
      }
      else{
        return new HLIRString(value);
      }
    }

    static HLIRValue* create(llvm::Value* value){
      return new HLIRValue(value);
    }
    
    static HLIRInstruction* create(llvm::Instruction* value){
      return new HLIRInstruction(value);
    }

    static bool isSymbol(const char* str){
      bool match = false;
      
      size_t i = 0;
      for(;;){
        switch(str[i]){
          case '\0':
            return match;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if(match){
              break;
            }
            return false;
          case 'a':
          case 'b':
          case 'c':
          case 'd':
          case 'e':
          case 'f':
          case 'g':
          case 'h':
          case 'i':
          case 'j':
          case 'k':
          case 'l':
          case 'm':
          case 'n':
          case 'o':
          case 'p':
          case 'q':
          case 'r':
          case 's':
          case 't':
          case 'u':
          case 'v':
          case 'w':
          case 'x':
          case 'y':
          case 'z':
          case 'A':
          case 'B':
          case 'C':
          case 'D':
          case 'E':
          case 'F':
          case 'G':
          case 'H':
          case 'I':
          case 'J':
          case 'K':
          case 'L':
          case 'M':
          case 'N':
          case 'O':
          case 'P':
          case 'Q':
          case 'R':
          case 'S':
          case 'T':
          case 'U':
          case 'V':
          case 'W':
          case 'X':
          case 'Y':
          case 'Z':
            match = true;
            break;
          case '_':
            break;
          default:
            return false;
        }
        ++i;
      }
      
      return match;
    }

    static bool isSymbol(const std::string& str){
      return isSymbol(str.c_str());
    }
  };

  class HLIRMap : public HLIRNode{
  public:
    class Proxy_{
    public:
      friend class HLIRMap;

      template<class T>
      HLIRNode& operator=(const T& value){
        HLIRNode* v = HLIRNodeFactory::create(value);
        map_.put_(key_, v);
        return *v;
      }

      HLIRNode& operator=(HLIRNode* ptr){
        map_.put_(key_, ptr);
        return *ptr;
      }

      operator HLIRNode&(){
        return *map_.get_(key_); 
      }

      HLIRNode* operator->(){
        return map_.get_(key_);
      }

    private:
      Proxy_(HLIRMap& map, const HLIRSymbol& key)
        : map_(map),
          key_(key){}

      HLIRMap& map_;
      HLIRSymbol key_;
    };

    class ConstProxy_{
    public:
      friend class HLIRMap;

      operator const HLIRNode&() const{
        return *map_.get_(key_); 
      }

      const HLIRNode* operator->() const{
        return map_.get_(key_);
      }

    private:
      ConstProxy_(const HLIRMap& map, const HLIRSymbol& key)
        : map_(map),
          key_(key){}

      const HLIRMap& map_;
      HLIRSymbol key_;
    };
    
    friend class Proxy_;

    HLIRMap(){}

    HLIRMap(const HLIRMap& map)
      : map_(map.map_){}

    template<class T>
    T& get(const std::string& key){
      return get_(key)->as<T>();
    }

    template<class T>
    const T& get(const std::string& key) const{
      return get_(key)->as<T>();
    }

    Proxy_ operator[](const std::string& key){
      return Proxy_(*this, key); 
    }

    ConstProxy_ operator[](const std::string& key) const{
      return ConstProxy_(*this, key); 
    }

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string indent = __indentTo(level);

      ostr << "{";

      for(auto& itr : map_){
        HLIRNode* val = itr.second;
        if(val->hasValue()){
          ostr << std::endl << indent << "  " << itr.first << ": ";
          itr.second->output(ostr, level + 1);
        }
      }

      ostr << std::endl << indent << "}";
    }

    virtual HLIRMap* copy() const override{
      return new HLIRMap(*this);
    }

    virtual bool isRecursive() const override{
      return true;
    }

  private:
    using Map_ = std::map<HLIRSymbol, HLIRNode*>;

    Map_ map_;

    void put_(const HLIRSymbol& symbol, HLIRNode* value){
      value->setParent(this);
      map_[symbol] = value;
    }

    HLIRNode* get_(const HLIRSymbol& symbol){
      auto itr = map_.find(symbol);

      if(itr == map_.end()){
        HLIR_ERROR("invalid key: " + symbol.str());
      }

      return itr->second;
    }

    const HLIRNode* get_(const HLIRSymbol& symbol) const{
      auto itr = map_.find(symbol);

      if(itr == map_.end()){
        HLIR_ERROR("invalid key: " + symbol.str());
      }

      return itr->second;
    }
  };

  class HLIRVector : public HLIRNode{
  public:
    class Proxy_{
    public:
      friend class HLIRVector;

      template<class T>
      HLIRNode& operator=(const T& value){
        HLIRNode* v = HLIRNodeFactory::create(value);
        vector_.put_(index_, v);
        return *v;
      }

      HLIRNode& operator=(HLIRNode* ptr){
        vector_.put_(index_, ptr);
        return *ptr;
      }

      operator HLIRNode&(){
        return *vector_.get_(index_); 
      }

      HLIRNode* operator->(){
        return vector_.get_(index_);
      }

    private:
      Proxy_(HLIRVector& vector, size_t index)
        : vector_(vector),
          index_(index){}

      HLIRVector& vector_;
      size_t index_;
    };

    class ConstProxy_{
    public:
      friend class HLIRVector;

      operator const HLIRNode&() const{
        return *vector_.get_(index_); 
      }

    private:
      ConstProxy_(const HLIRVector& vector, size_t index)
        : vector_(vector),
          index_(index){}

      const HLIRVector& vector_;
      size_t index_;
    };
    
    friend class Proxy_;

    HLIRVector(){}

    HLIRVector(const HLIRVector& vector)
      : vector_(vector.vector_){}

    virtual HLIRVector* copy() const override{
      return new HLIRVector(*this);
    }

    Proxy_ operator[](size_t index){
      return Proxy_(*this, index); 
    }

    ConstProxy_ operator[](size_t index) const{
      return ConstProxy_(*this, index); 
    }

    template<class T>
    HLIRVector& operator<<(const T& value){
      auto n = HLIRNodeFactory::create(value);
      n->setParent(this);

      vector_.push_back(n);
      return *this;
    }

    HLIRVector& operator<<(HLIRNode* ptr){
      ptr->setParent(this);
      vector_.push_back(ptr);
      return *this;
    }

    virtual void output(std::ostream& ostr, size_t level=0) const override{
      std::string indent = __indentTo(level);

      ostr << "[";

      bool first = true;
      for(HLIRNode* vi : vector_){
        if(first){
          first = false;
        }
        else{
          ostr << std::endl;
        }
        ostr << indent;
        if(vi->hasValue()){
          vi->output(ostr, level + 1);
        }
        else{
          ostr << "<<<NULL>>>";
        }
      }

      ostr << std::endl << indent << "]";
    }

    virtual bool isRecursive() const override{
      return true;
    }

  private:
    using Vector_ = std::vector<HLIRNode*>;

    Vector_ vector_;

    void put_(size_t index, HLIRNode* value){
      while(index >= vector_.size()){
        vector_.push_back(nullptr);
      }
      vector_[index] = value;
    }

    HLIRNode* get_(size_t index){
      if(index >= vector_.size()){
        HLIR_ERROR("invalid index: " + toStr(index));
      }

      return vector_[index];
    }

    const HLIRNode* get_(size_t index) const{
      if(index >= vector_.size()){
        HLIR_ERROR("invalid index: " + toStr(index));
      }

      return vector_[index];
    }
  };

  class HLIRConstruct;
  class HLIRParallelFor;
  class HLIRParallelReduce;
  class HLIRTask;

  class HLIRModule : public HLIRMap{
  public:
    llvm::Type* voidTy;
    llvm::IntegerType* boolTy;
    llvm::IntegerType* i8Ty;
    llvm::IntegerType* i16Ty;
    llvm::IntegerType* i32Ty;
    llvm::IntegerType* i64Ty;
    llvm::Type* floatTy;
    llvm::Type* doubleTy;
    llvm::PointerType* voidPtrTy;

    static HLIRModule* getModule(llvm::Module* module);

    void setName(const HLIRString& name){
      (*this)["name"] = name;
    }

    auto& name() const{
      return get<HLIRString>("name");
    }

    void setVersion(const HLIRString& version){
      (*this)["version"] = version;
    }

    auto& version() const{
      return get<HLIRString>("version");
    }

    void setLanguage(const HLIRString& language){
      (*this)["language"] = language;
    }

    auto& language() const{
      return get<HLIRString>("language");
    }

    llvm::Function* getIntrinsic(const std::string& name){
      std::string fullName = "hlir." + name;

      auto f = module_->getFunction(fullName);
      if(f){
        return f;
      }

      f = llvm::Function::Create(llvm::FunctionType::get(voidTy, false),
                                 llvm::GlobalValue::ExternalLinkage,
                                 fullName, module_);
      return f;
    }

    llvm::Instruction* createNoOp(){
      return builder_.CreateAlloca(boolTy, nullptr);
    }

    HLIRParallelFor* createParallelFor();

    HLIRParallelReduce* createParallelReduce(const HLIRType& reduceType);

    HLIRTask* createTask();

    llvm::Module* module(){
      return module_;
    }

    llvm::LLVMContext& context(){
      return context_;
    }

    auto& builder(){
      return builder_;
    }

    llvm::Function*
    getFunction(const std::string& funcName,
                const std::vector<llvm::Type*>& argTypes,
                llvm::Type* retType=nullptr){
  
      llvm::Function* func = module_->getFunction(funcName);
  
      if(func){
        return func;
      }
      
      llvm::FunctionType* funcType =
      llvm::FunctionType::get(retType ? retType : voidTy,
                              argTypes, false);
      
      func =
      llvm::Function::Create(funcType,
                             llvm::Function::ExternalLinkage,
                             funcName,
                             module_);
      
      return func;
    }

    bool lowerToIR_();

    void lowerParallelFor_(HLIRParallelFor* pfor);

    void lowerParallelReduce_(HLIRParallelReduce* reduce);

    void lowerTask_(HLIRTask* task);

    llvm::Value* toInt8(const HLIRInteger& i){
      return llvm::ConstantInt::get(i8Ty, i);
    }

    llvm::Value* toInt16(const HLIRInteger& i){
      return llvm::ConstantInt::get(i16Ty, i);
    }

    llvm::Value* toInt32(const HLIRInteger& i){
      return llvm::ConstantInt::get(i32Ty, i);
    }

    llvm::Value* toInt64(const HLIRInteger& i){
      return llvm::ConstantInt::get(i64Ty, i);
    }

  private:
    HLIRModule(llvm::Module* module)
      : module_(module),
        context_(module_->getContext()),
        builder_(context_){

      voidTy = llvm::Type::getVoidTy(context_);
      boolTy = llvm::Type::getInt1Ty(context_);
      i8Ty = llvm::Type::getInt8Ty(context_);
      i16Ty = llvm::Type::getInt16Ty(context_);
      i32Ty = llvm::Type::getInt32Ty(context_);
      i64Ty = llvm::Type::getInt64Ty(context_);
      floatTy = llvm::Type::getFloatTy(context_);
      doubleTy = llvm::Type::getDoubleTy(context_);

      voidPtrTy = llvm::PointerType::get(i8Ty, 0);
    }

    llvm::Module* module_;
    llvm::LLVMContext& context_;
    llvm::IRBuilder<> builder_;

    std::vector<HLIRConstruct*> constructs_;
  };

  class HLIRTaskParam : public HLIRMap{
  public:
    HLIRTaskParam(){
      (*this)["read"] = false;
      (*this)["write"] = false;
      (*this)["partitioner"] = HLIRFunction::nullValue();
    }
    
    void setRead(const HLIRBoolean& flag){
      (*this)["read"] = flag;
    }

    auto& read() const{
      return get<HLIRBoolean>("read");
    }

    void setWrite(const HLIRBoolean& flag){
      (*this)["write"] = flag;
    }

    auto& write() const{
      return get<HLIRBoolean>("write");
    }

    void setPartitioner(const HLIRFunction& func){
      (*this)["partitioner"] = func;
    }

    auto& partitioner() const{
      return get<HLIRFunction>("partitioner");
    }

  };

  class HLIRConstruct : public HLIRMap{
  public:
    HLIRConstruct(HLIRModule* module)
      : module_(module){}

    virtual std::string intrinsic() const{
      HLIR_ERROR("invalid intrinsic");
    }

    template<bool P, class C, class I>
    void insert(llvm::IRBuilder<P, C, I>& builder){
      (*this)["marker"] = 
        builder.CreateCall(module_->getIntrinsic(intrinsic()));
    }

  protected:
    HLIRModule* module_;
  };

  class HLIRTask : public HLIRConstruct{
  public:
    HLIRTask(HLIRModule* module)
      : HLIRConstruct(module){
      auto ret = new HLIRTaskParam;
      ret->setRead(false);
      ret->setWrite(true);

      (*this)["return"] = ret;
      (*this)["parameters"] = HLIRVector();
      (*this)["function"] = HLIRFunction::nullValue();
    }

    HLIRTaskParam& getReturn(){
      return get<HLIRTaskParam>("return");
    }

    HLIRTaskParam& addParam(){
      HLIRTaskParam* param = new HLIRTaskParam;
      auto& params = get<HLIRVector>("parameters") << param;
      params << param;
      return *param;
    }

    void setFunction(const HLIRFunction& func);

    auto& function() const{
      return get<HLIRFunction>("function");
    }

    auto& wrapperFunction() const{
      return get<HLIRFunction>("wrapperFunction");
    }

    void setName(const HLIRString& name){
      (*this)["name"] = name;
    }

    auto& name() const{
      return get<HLIRString>("name");
    }
  };

  class HLIRFuture : public HLIRConstruct{
  public:
    HLIRFuture(HLIRModule* module, HLIRValue& value)
      : HLIRConstruct(module){}

    void await(){}

    HLIRValue await(const HLIRValue& microsends){}
  };

  class HLIRBuffer : public HLIRConstruct{
  public:
    HLIRBuffer(HLIRModule* module)
      : HLIRConstruct(module){}

    void init(const HLIRValue& buffer,
              const HLIRValue& size){}
  };

  class HLIRTeam : public HLIRConstruct{
  public:
    HLIRTeam(HLIRModule* module)
      : HLIRConstruct(module){}
  };

  class HLIRSend : public HLIRConstruct{
  public:
    virtual std::string intrinsic() const override{
      return "send";
    }

    HLIRSend(HLIRModule* module)
      : HLIRConstruct(module){}

    void setFuture(const HLIRFuture& future){
      (*this)["future"] = future;
    }

    auto& future() const{
      return get<HLIRFuture>("future");
    }

    void setTeam(const HLIRTeam& team){
      (*this)["team"] = team;
    }

    auto& team() const{
      return get<HLIRTeam>("team");
    }

    void setBuffer(const HLIRBuffer& buffer){
      (*this)["buffer"] = buffer;
    }

    auto& buffer() const{
      return get<HLIRBuffer>("buffer");
    }
  };

  class HLIRReceive : public HLIRConstruct{
  public:
    virtual std::string intrinsic() const override{
      return "receive";
    }

    HLIRReceive(HLIRModule* module)
      : HLIRConstruct(module){}

    void setFuture(const HLIRFuture& future){
      (*this)["future"] = future;
    }

    auto& future() const{
      return get<HLIRFuture>("future");
    }

    void setTeam(const HLIRTeam& team){
      (*this)["team"] = team;
    }

    auto& team() const{
      return get<HLIRTeam>("team");
    }

    void setBuffer(const HLIRBuffer& buffer){
      (*this)["buffer"] = buffer;
    }

    auto& buffer() const{
      return get<HLIRBuffer>("buffer");
    }
  };

  class HLIRBarrier : public HLIRConstruct{
  public:
    virtual std::string intrinsic() const override{
      return "barrier";
    }

    HLIRBarrier(HLIRModule* module)
      : HLIRConstruct(module){}

    void setTeam(const HLIRTeam& team){
      (*this)["team"] = team;
    }

    auto& team() const{
      return get<HLIRTeam>("team");
    }
  };

  class HLIRParallelFor : public HLIRConstruct{
  public:
    virtual std::string intrinsic() const override{
      return "parallel_for";
    }

    void setName(const HLIRString& name){
      (*this)["name"] = name;
    }

    auto& name() const{
      return get<HLIRString>("name");
    }

    auto& index() const{
      return get<HLIRValue>("index");
    }

    auto& insertion() const{
      return get<HLIRInstruction>("insertion");
    }

    auto& argsInsertion() const{
      return get<HLIRInstruction>("argsInsertion");
    }

    auto& args() const{
      return get<HLIRValue>("args");
    }

    HLIRFunction& body();

    void setRange(const HLIRInteger& start, const HLIRInteger& end){
      if(start > end){
        HLIR_ERROR("invalid range");
      }

      (*this)["range"] = HLIRVector() << start << end;
    }

    auto& range() const{
      return get<HLIRVector>("range");
    }

  private:
    friend class HLIRModule;
    friend class HLIRPass;

    HLIRParallelFor(HLIRModule* module);

    auto& callMarker() const{
      return get<HLIRInstruction>("callMarker");
    }
  };

  class HLIRParallelReduce : public HLIRConstruct{
  public:
    virtual std::string intrinsic() const override{
      return "parallel_reduce";
    }

    void setName(const HLIRString& name){
      (*this)["name"] = name;
    }

    auto& name() const{
      return get<HLIRString>("name");
    }

    auto& index() const{
      return get<HLIRValue>("index");
    }

    auto& reduceType() const{
      return get<HLIRType>("reduceType");
    }

    auto& sum() const{
      return get<HLIRBoolean>("sum");
    }

    void setSum(const HLIRBoolean& flag){
      (*this)["sum"] = flag;
    }

    auto& reduceVar() const{
      return get<HLIRValue>("reduceVar");
    }

    auto& insertion() const{
      return get<HLIRInstruction>("insertion");
    }

    auto& entry() const{
      return get<HLIRInstruction>("entry");
    }

    auto& argsInsertion() const{
      return get<HLIRInstruction>("argsInsertion");
    }

    auto& args() const{
      return get<HLIRValue>("args");
    }

    HLIRFunction& body();

    void setRange(const HLIRInteger& start, const HLIRInteger& end){
      if(start > end){
        HLIR_ERROR("invalid range");
      }

      (*this)["range"] = HLIRVector() << start << end;
    }

    auto& range() const{
      return get<HLIRVector>("range");
    }

  private:
    friend class HLIRModule;
    friend class HLIRPass;

    HLIRParallelReduce(HLIRModule* module, const HLIRType& reduceType);

    auto& callMarker() const{
      return get<HLIRInstruction>("callMarker");
    }
  };

} // namespace ares

#endif // __ARES_HLIR_H__
