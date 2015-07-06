// -*-c++-*-
/**
 * Author: Tyler (Izzy) Cecil
 * Date: Fri Jun 12 17:07:38 MDT 2015
 * File AST.h
 *
 * This file outlines the AST for our toy language. There are three root classes
 * of note: `Expr` `Proto` and `Func`. This is modeled from the LLVM Kaleidoscope
 * tutorial (http://llvm.org/docs/tutorial/LangImpl2.HTML).
 *
 */
#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

using namespace llvm;

enum NodeType {
  kNum,
  kVar,
  kBin,
  kCall,
  kProto,
  kFunc,
};

enum BinOp {
  kAdd,
  kSub,
  kMul,
  kDiv,
};

namespace Codegen {
  Value *errorv(const char *str) { std::cerr << str; return 0; }

  static Module* module;
  static IRBuilder<> b(getGlobalContext());
  static std::map<std::string, Value*> valueTable;
}; // namespace


/**
 * Generates a prefix string for printing ASTs, that will take care of indentation
 * and semantic labeling.
 */
std::string prefix(const std::string& label, int depth) {
    std::stringstream sstm;
    while(depth--) {
      sstm << "\t";
    }
    sstm << label;
    return sstm.str();
}

////////////////////////////////////////////////////////////////
// Root node types
////////////////////////////////////////////////////////////////
// Root Type
struct AST {
  AST(NodeType type) : type(type) {};
  virtual ~AST() {};

  virtual void print(const std::string& label, int depth) = 0;
  NodeType type;
};

// Root Expression Type
struct Expr    : AST {
  Expr(NodeType type) : AST(type) {};
  virtual ~Expr() {};
  virtual Value* codegen() = 0;
};

// Root Statement Type
struct Stat : AST {
  Stat(NodeType type) : AST(type) {};
  virtual ~Stat() {};
};

////////////////////////////////////////////////////////////////
// Expressions
////////////////////////////////////////////////////////////////
struct NumExpr : Expr {
  NumExpr() : Expr(kNum), val() {};
  NumExpr(double val) : Expr(kNum), val(val) {};

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "NumExpr( " << val << " )" << std::endl;
  };

  Value* codegen() {
    return ConstantFP::get(getGlobalContext(), APFloat(val));
  }

  double val;
};

struct NameExpr : Expr {
  NameExpr() : Expr(kVar), name() {};
  NameExpr(const std::string &name) : Expr(kVar), name(name) {};

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "NameExpr( " << name << " )" << std::endl;
  };

  Value* codegen() {
    // Look this variable up in the function.
    Value *V = Codegen::valueTable[name];
    return V ? V : Codegen::errorv("Unknown variable name");
  }

  std::string name;
};

struct BinExpr : Expr {
  BinExpr() : Expr(kBin), lhs(nullptr), rhs(nullptr) {};
  BinExpr(BinOp op, Expr *lhs, Expr *rhs)
    : Expr(kBin), op(op), lhs(lhs), rhs(rhs) {};

  ~BinExpr() {delete lhs; delete rhs;};

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "BinExpr( ";
    switch(op) {
    case kAdd : std::cout << "+"; break;
    case kSub : std::cout << "-"; break;
    case kMul : std::cout << "*"; break;
    case kDiv : std::cout << "/"; break;
    }
    std::cout << " )" << std::endl;

    lhs->print("LHS -> ", depth + 1);
    rhs->print("RHS -> ", depth + 1);
  };

  Value* codegen() {
    Value *l = lhs->codegen();
    Value *r = rhs->codegen();
    if (l == 0 || r == 0) return 0;

    switch (op) {
    case kAdd: return Codegen::b.CreateFAdd(l, r, "addtmp");
    case kSub: return Codegen::b.CreateFSub(l, r, "subtmp");
    case kMul: return Codegen::b.CreateFMul(l, r, "multmp");
    case kDiv: return Codegen::b.CreateFDiv(l, r, "divtmp");
    default: return Codegen::errorv("invalid binary operator");
    }
  }

  BinOp op;
  Expr *lhs, *rhs;
};

struct CallExpr : Expr {
  CallExpr() : Expr(kCall), callee(nullptr), args() {};
  CallExpr(NameExpr* callee, std::vector<Expr*> args)
    : Expr(kCall), callee(callee), args(args) {};

  ~CallExpr() {
    delete callee;
    for(auto arg : args){
      delete arg;
    }
  };

  Value* codegen() {
    // Look up the name in the global module table.
    Function *func = Codegen::module->getFunction(callee->name);
    if (func == 0) {
      return Codegen::errorv("Unknown function referenced");
    }

    // If argument mismatch error.
    if (func->arg_size() != args.size()) {
      return Codegen::errorv("Incorrect # arguments passed");
    }

    std::vector<Value*> argsv;
    for(auto arg : args) {
      argsv.push_back(arg->codegen());
    }

    return Codegen::b.CreateCall(func, argsv, "calltmp");
  }

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "CallExpr()" << std::endl;
    callee->print("CALLEE -> ", depth + 1);
    for(auto arg : args) {
      arg->print("ARG -> ", depth + 1);
    }
  }

  NameExpr* callee;
  std::vector<Expr*> args;
};

struct Proto : AST {
  Proto() : AST(kProto), name(nullptr), args() {};
  Proto(NameExpr* name, const std::vector<NameExpr*> &args)
    : AST(kProto), name(name), args(args) {};

  ~Proto() {
    delete name;
    for(auto arg : args) {
      delete arg;
    }
  }

    Function* codegen() {
      // Make the function type:  double(double,double) etc.
      std::vector<Type*> doubles(args.size(),
                                 Type::getDoubleTy(getGlobalContext()));
      FunctionType *ft = FunctionType::get(Type::getDoubleTy(getGlobalContext()),
                                           doubles, false);
      Function *f = Function::Create(ft, Function::ExternalLinkage,
                                     name->name, Codegen::module);

      // If F conflicted, there was already something named 'Name'.  If it has a
      // body, don't allow redefinition or reextern.
      if (f->getName() != name->name) {
        // Delete the one we just made and get the existing one.
        f->eraseFromParent();
        f = Codegen::module->getFunction(name->name);
      }

      // If F already has a body, reject this.
      if (!f->empty()) {
        Codegen::errorv("redefinition of function");
        return 0;
      }

      // If F took a different number of args, reject.
      if (f->arg_size() != args.size()) {
        Codegen::errorv("redefinition of function with different # args");
        return 0;
      }

      // Set names for all arguments.
      unsigned idx = 0;
      for (Function::arg_iterator ai = f->arg_begin(); idx != args.size();
           ++ai, ++idx) {
        ai->setName(args[idx]->name);

        // Add arguments to variable symbol table.
        Codegen::valueTable[args[idx]->name] = ai;
      }

      return f;
    }

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "Proto()" << std::endl;
    name->print("FUNC NAME -> ", depth + 1);
    for(auto arg : args) {
      arg->print("ARG -> ", depth + 1);
    }
  };

  NameExpr* name;
  std::vector<NameExpr*> args;
};

// Right now functions are single expressions. This is just for testing reasons.
struct Func : AST {
  Func() : AST(kFunc), proto(nullptr), body(nullptr) {};
  Func(Proto* proto, Expr* body) : AST(kFunc), proto(proto), body(body) {};

  ~Func() {
    delete proto;
    delete body;
  }

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "Func()" << std::endl;
    proto->print("PROTO -> ", depth + 1);
    body->print("BODY -> ", depth + 1);
  }

  Function* codegen() {
    Codegen::valueTable.clear();

    Function *func = proto->codegen();

    if (func == 0) {
      return 0;
    }

    // Create a new basic block to start insertion into.
    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", func);
    Codegen::b.SetInsertPoint(bb);

    if (Value *retval = body->codegen()) {
      // Finish off the function.
      Codegen::b.CreateRet(retval);

      // Validate the generated code, checking for consistency.
      verifyFunction(*func);

      return func;
    }

    // Error reading body, remove function.
    func->eraseFromParent();
    return 0;
  }


  Proto* proto;
  Expr* body;
};
