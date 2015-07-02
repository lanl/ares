// -*-c++-*-
/**
 * Author: Tyler (Izzy) Cecil
 * Date: Fri Jun 12 17:07:38 MDT 2015
 * File AST.h
 *
 * This file outlines the AST for our toy language. There are three root classes
 * of note: `Expr` `Proto` and `Func`. This is modeled from the LLVM Kaleidoscope
 * tutorial (http://llvm.org/docs/tutorial/LangImpl2.html).
 *
 * TODO: I have not yet begun generating an AST from parsing, but I suspect this
 *       will be modified to have two root objects: `Expr` and `Statement`.
 *       Functions also currently only have one `Expr` in their body... this will
 *       likely need to be a vector.
 *
 * TODO: Classes should have an enum of what class they are, to allow for
 *       runtime class reflection.
 */
#pragma once

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

////////////////////////////////////////////////////////////////
// Root node types
////////////////////////////////////////////////////////////////
// Root Type
struct AST {
  AST(NodeType type) : type(type) {};
  virtual ~AST() {};
  virtual std::string string() = 0;
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
  NumExpr(double val) : Expr(kNum), val(val) {};

  std::string string() {
    std::stringstream sstm;
    sstm << "NumExpr( " << val << ")";
    return sstm.str();
  };

  double val;
};

struct NameExpr : Expr {
  NameExpr(const std::string &name) : Expr(kVar), name(name) {};

  std::string string() {
    std::stringstream sstm;
    sstm << "NameExpr( " << name << ")";
    return sstm.str();
  };


  std::string name;
};

struct BinExpr : Expr {
  BinExpr(BinOp op, Expr *lhs, Expr *rhs)
    : Expr(kBin), op(op), lhs(lhs), rhs(rhs) {};

  ~BinExpr() {delete lhs; delete rhs;};

  std::string string() {
    std::stringstream sstm;
    sstm << "BinExpr( ";
    switch(op) {
    case kAdd : sstm << "+"; break;
    case kSub : sstm << "-"; break;
    case kMul : sstm << "*"; break;
    case kDiv : sstm << "/"; break;
    }
    sstm << ")";
    return sstm.str();
  };


  BinOp op;
  Expr *lhs, *rhs;
};

struct CallExpr : Expr {
  CallExpr(NameExpr* callee, std::vector<Expr*> args)
    : Expr(kCall), callee(callee), args(args) {};

  ~CallExpr() {
    delete callee;
    for(auto arg : args){
      delete arg;
    }
  };

  NameExpr* callee;
  std::vector<Expr*> args;
};

struct Proto : AST {
  Proto(NameExpr* name, const std::vector<NameExpr*> &args)
    : AST(kProto), name(name), args(args) {};

  ~Proto() {
    delete name;
    for(auto arg : args) {
      delete arg;
    }
  };

  std::string string() {
    std::stringstream sstm;
    sstm << "Proto()";
    return sstm.str();
  };

  NameExpr* name;
  std::vector<NameExpr*> args;
};

// Right now functions are single expressions. This is just for testing reasons.
struct Func : AST {
  Func(Proto* proto, Expr* body) : AST(kFunc), proto(proto), body(body) {};

  ~Func() {
    delete proto;
    delete body;
  }

  std::string string() {
    std::stringstream sstm;
    sstm << "Func()";
    return sstm.str();
  };

  Proto* proto;
  Expr* body;
};
