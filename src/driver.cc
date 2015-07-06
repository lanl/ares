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
  }
}
