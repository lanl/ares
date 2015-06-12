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

enum BinOp {
  kAdd,
  kSub,
  kMul,
  kDiv
};

class Expr {
public:
  virtual ~Expr();
};


class NumExpr : public Expr {
private:
  double val;
public:
  NumExpr(double val) : val(val) {}
};


class VarExpr : public Expr {
private:
  std::string name;
public:
  VarExpr(const std::string &name) : name(name) {}
};


class BinExpr : public Expr {
private:
  BinOp op;
  Expr *lhs, *rhs;
public:
  BinExpr(BinOp op, Expr *lhs, Expr *rhs) : op(op), lhs(lhs), rhs(rhs) {}
};


class CallExpr : public Expr {
private:
  std::string callee;
  std::vector<Expr*> args;
public:
  CallExpr(const std::string &callee, std::vector<Expr*> args)
    : callee(callee), args(args) {}
};


class Proto {
private:
  std::string name;
  std::vector<std::string> args;
public:
  Proto(const std::string &name, const std::vector<std::string> &args)
    : name(name), args(args) {}
};

class Func {
private:
  Proto *proto;
  Expr *body;
public:
  Func(Proto *proto, Expr *body) : proto(proto), body(body) {}
};
