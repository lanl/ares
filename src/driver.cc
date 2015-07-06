/**
 * Author: Tyler (Izzy) Cecil
 * Date: Fri Jun 12 17:15:43 MDT 2015
 * File: driver.cc
 *
 * This is the main file for our toy language compiler. Currently it invokes
 * the parser on the first argument given to it, and returns whether or not
 * the argument was a valid sentence.
 *
 */
#include <iostream>
#include <stack>
#include <string>

#include <pegtl.hh>
#include <pegtl/analyze.hh>

#include "AST.h"
#include "parse.h"

const size_t issues_found = pegtl::analyze< parse::grammar >();

int main( int argc, char * argv[] )
{
  if ( argc > 1 ) {
    std::stack<AST*> parse_stack();

    std::cout << argv[1] << std::endl;
    pegtl::parse< parse::grammar,  parse::build_ast >(1, argv);
  } else {
    std::cout << "ISSUES FOUND: " << issues_found << std::endl;

    // Make and print a test AST. I must do this "bottom up".
    /*
      func f(x, y) { if x then f(x + 10 * y, y) else y}
     */
    NameExpr* f_name = new NameExpr("f");
    NameExpr* x_0 = new NameExpr("x");
    NameExpr* y_0 = new NameExpr("y");

    std::vector<NameExpr*> formals;
    formals.push_back(x_0);
    formals.push_back(y_0);

    Proto* f_proto = new Proto(f_name, formals);

    NameExpr* f_call_name = new NameExpr("f");

    NameExpr* x_1 = new NameExpr("x");
    NameExpr* y_1 = new NameExpr("y");
    NumExpr* ten = new NumExpr(10);
    BinExpr* mult = new BinExpr(kMul, ten, y_1);
    BinExpr* add = new BinExpr(kAdd, x_1, mult);

    NameExpr* y_2 = new NameExpr("y");

    std::vector<Expr*> actuals;
    actuals.push_back(add);
    actuals.push_back(y_2);

    CallExpr* call = new CallExpr(f_call_name, actuals);

    NameExpr* x_2 = new NameExpr("x");
    NameExpr* y_3 = new NameExpr("y");

    IfExpr* ifExpr = new IfExpr(x_2, call, y_3);

    Func* func = new Func(f_proto, call);

    func->print("root: ", 0);

    LLVMContext &Context = getGlobalContext();
    Codegen::module = new Module("Toy", Context);

    llvm::Function* f = func->codegen();
    f->dump();
    delete func;
  }
}
