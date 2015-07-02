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
#include <string>
#include <vector>

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

  double val;
};

struct NameExpr : Expr {
  NameExpr() : Expr(kVar), name() {};
  NameExpr(const std::string &name) : Expr(kVar), name(name) {};

  void print(const std::string& label, int depth) {
    std::cout << prefix(label, depth) << "NameExpr( " << name << " )" << std::endl;
  };

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
  };

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
  };

  Proto* proto;
  Expr* body;
};
