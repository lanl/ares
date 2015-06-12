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
#include <string>

#include <pegtl.hh>

#include "AST.h"
#include "parse.h"

int main( int argc, char * argv[] )
{
  if ( argc > 1 ) {
    std::cout << argv[1] << std::endl;
    pegtl::parse< parse::grammar,  parse::action >(1, argv);
    std::cout << "That was a valid sentence!\n";
  }
}
