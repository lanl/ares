// -*-c++-*-
/**
 * Author: Tyler (Izzy) Cecil
 * Date: Fri Jun 12 16:58:31 MDT 2015
 * File: parse.h
 *
 * This file provides a parser for our toy language using the [PEGTL]
 * (https://github.com/ColinH/PEGTL) library. I've set up the language to be
 * C-like (for ease of understanding the grammar).
 *
 * Two structs are of utmost importance: `grammar` and `action`. `grammar` defines
 * our grammar. `action` is a templated structure which defines what to do when
 * a match is made. Specializing this struct for various parsers will allow
 * various actions to occur for each matched section.
 *
 * TODO: Setup the action interface.
 * TODO: Finish the grammar
 */
#pragma once

#include <string>
#include <iostream>

#include <pegtl.hh>

namespace parse {
  using namespace pegtl;

  ////////////////////////////////////////////////////////////////
  // Keywords
  ////////////////////////////////////////////////////////////////
  // Note that 'elseif' precedes 'else' in order to prevent only matching
  // the "else" part of an "elseif" and running into an error in the
  // 'keyword' rule.
  struct str_and     : string< 'a', 'n', 'd' > {};
  struct str_break   : string< 'b', 'r', 'e', 'a', 'k' > {};
  struct str_do      : string< 'd', 'o' > {};
  struct str_else    : string< 'e', 'l', 's', 'e' > {};
  struct str_elseif  : string< 'e', 'l', 's', 'e', 'i', 'f' > {};
  struct str_end     : string< 'e', 'n', 'd' > {};
  struct str_extern  : string< 'e', 'x', 't', 'e', 'r', 'n' > {};
  struct str_false   : string< 'f', 'a', 'l', 's', 'e' > {};
  struct str_for     : string< 'f', 'o', 'r' > {};
  struct str_func    : string< 'f', 'u', 'n', 'c' > {};
  struct str_goto    : string< 'g', 'o', 't', 'o' > {};
  struct str_if      : string< 'i', 'f' > {};
  struct str_in      : string< 'i', 'n' > {};
  struct str_not     : string< 'n', 'o', 't' > {};
  struct str_or      : string< 'o', 'r' > {};
  struct str_return  : string< 'r', 'e', 't', 'u', 'r', 'n' > {};
  struct str_then    : string< 't', 'h', 'e', 'n' > {};
  struct str_true    : string< 't', 'r', 'u', 'e' > {};
  struct str_while   : string< 'w', 'h', 'i', 'l', 'e' > {};

  struct str_keyword : sor< str_and, str_break, str_do, str_else, str_elseif,
                            str_end, str_extern, str_false, str_for, str_func,
                            str_goto, str_if, str_in, str_not, str_or,
                            str_return, str_then, str_true, str_while > {};

  template< typename Key >
  struct key : seq< Key, not_at< identifier_other > > {};

  struct key_and     : key< str_and >    {};
  struct key_break   : key< str_break >  {};
  struct key_do      : key< str_do >     {};
  struct key_else    : key< str_else >   {};
  struct key_elseif  : key< str_elseif > {};
  struct key_end     : key< str_end >    {};
  struct key_extern  : key< str_extern > {};
  struct key_false   : key< str_false >  {};
  struct key_for     : key< str_for >    {};
  struct key_func    : key< str_func >   {};
  struct key_goto    : key< str_goto >   {};
  struct key_if      : key< str_if >     {};
  struct key_in      : key< str_in >     {};
  struct key_not     : key< str_not >    {};
  struct key_or      : key< str_or >     {};
  struct key_return  : key< str_return > {};
  struct key_then    : key< str_then >   {};
  struct key_true    : key< str_true >   {};
  struct key_while   : key< str_while >  {};

  struct keyword : key< str_keyword > {};

  ////////////////////////////////////////////////////////////////
  // literals and identifiers
  ////////////////////////////////////////////////////////////////
  struct num : seq <
    opt< sor< one< '+' >, one< '-' > > >,
    sor<
      seq< one< '.' >, plus< digit > >,
      seq< plus< digit >, opt< one< '.' >, star< digit > > > > >{};

  struct name : seq< not_at< keyword >, identifier > {};

  ////////////////////////////////////////////////////////////////
  // Operators
  // Lower the number, lower in the tree
  ////////////////////////////////////////////////////////////////
  struct op_add : one< '+' > {};
  struct op_sub : one< '-' > {};
  struct op_mul : one< '*' > {};
  struct op_div : one< '/' > {};
  struct op_not : one< '!' > {};
  struct op_eq  : string< '=', '=' > {};
  struct op_ass : one< '=' > {};
  struct op_neq : string< '!', '=' > {};
  struct op_lte : string< '<', '=' > {};
  struct op_lt  : one< '<' > {};
  struct op_gte : string< '>', '=' > {};
  struct op_gt  : one< '>' > {};

  struct bin_op_1 : failure {};
  struct bin_op_2 : failure {};
  struct bin_op_3 : sor< op_mul, op_div >{};
  struct bin_op_4 : sor< op_add, op_sub >{};
  struct bin_op_5 : sor< op_eq, op_neq, op_lte, op_gte, op_lt, op_gt > {};
  struct bin_op_6 : sor< op_ass > {};

  ////////////////////////////////////////////////////////////////
  // Expressions
  // Lower the number, lower in the tree
  ////////////////////////////////////////////////////////////////
  struct expr;
  struct call : seq< pad< name, space >,
                     one< '(' >,
                     pad_opt< list< expr, one< ',' >, space >, space >,
                     one< ')' > >{};
  struct expr_0 : pad<
    sor< call, num, name,
         seq< one< '(' >,  pad< expr, space >, one< ')' > > >,
    space > {};
  struct expr_1 : sor< seq< expr_0, pad< bin_op_1, space >, expr_1 >, expr_0 > {};
  struct expr_2 : sor< seq< expr_1, pad< bin_op_2, space >, expr_2 >, expr_1 > {};
  struct expr_3 : sor< seq< expr_2, pad< bin_op_3, space >, expr_3 >, expr_2 > {};
  struct expr_4 : sor< seq< expr_3, pad< bin_op_4, space >, expr_4 >, expr_3 > {};
  struct expr_5 : sor< seq< expr_4, pad< bin_op_5, space >, expr_5 >, expr_4 > {};
  struct expr   : sor< seq< name, pad< bin_op_6, space >, expr >, expr_5 > {};

  ////////////////////////////////////////////////////////////////
  // Functions and Blocks
  ////////////////////////////////////////////////////////////////
  struct prototype
    : seq< pad< name, space >,
           one< '(' >,
           pad_opt< list< name, one< ',' >, space >, space >,
           one< ')' > > {};

  struct statement;

  struct block : seq<
    one< '{' >,
    star< pad< sor< statement, seq< expr, one< ';' > > >, space > >,
    one< '}' > > {};

  struct func : seq< key_func, pad< prototype, space >, block > {};


  ////////////////////////////////////////////////////////////////
  // Statements
  ////////////////////////////////////////////////////////////////
  struct ret_stat : seq< key_return, pad< expr, space>,  one< ';' > > {};
  struct extern_stat : seq< key_extern, pad< prototype, space>, one< ';' > > {};

  struct if_stat : seq< key_if,
                        pad< one< '(' >, space >,
                        expr,
                        pad< one< ')' >, space >,
                        block, space > {};

  struct statement : sor< ret_stat, extern_stat, if_stat > {};

  ////////////////////////////////////////////////////////////////
  // Root Grammar Node
  ////////////////////////////////////////////////////////////////
  struct grammar
    : must< star< pad < sor< func, extern_stat >, space > > , eof > {};

  template< typename Rule >
  struct action
    : pegtl::nothing< Rule > {};

} // namespace parse
