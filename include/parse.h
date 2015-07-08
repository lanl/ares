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

#include <stack>
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
  //
  // These keywords are not all used. I just have theme here as they may be
  // used in the future.
  //
  struct str_do      : string< 'd', 'o' > {};
  struct str_else    : string< 'e', 'l', 's', 'e' > {};
  struct str_elseif  : string< 'e', 'l', 's', 'e', 'i', 'f' > {};
  struct str_end     : string< 'e', 'n', 'd' > {};
  struct str_extern  : string< 'e', 'x', 't', 'e', 'r', 'n' > {};
  struct str_false   : string< 'f', 'a', 'l', 's', 'e' > {};
  struct str_for     : string< 'f', 'o', 'r' > {};
  struct str_func    : string< 'f', 'u', 'n', 'c' > {};
  struct str_if      : string< 'i', 'f' > {};
  struct str_in      : string< 'i', 'n' > {};
  struct str_return  : string< 'r', 'e', 't', 'u', 'r', 'n' > {};
  struct str_task    : string< 't', 'a', 's', 'k' > {};
  struct str_then    : string< 't', 'h', 'e', 'n' > {};
  struct str_true    : string< 't', 'r', 'u', 'e' > {};

  struct str_keyword : sor< str_do, str_else, str_elseif, str_end, str_extern,
                            str_false, str_for, str_func, str_if, str_in,
                            str_return, str_task, str_then, str_true > {};

  template< typename Key >
  struct key : seq< Key, not_at< identifier_other > > {};

  struct key_do      : key< str_do >     {};
  struct key_else    : key< str_else >   {};
  struct key_elseif  : key< str_elseif > {};
  struct key_end     : key< str_end >    {};
  struct key_extern  : key< str_extern > {};
  struct key_false   : key< str_false >  {};
  struct key_for     : key< str_for >    {};
  struct key_func    : key< str_func >   {};
  struct key_if      : key< str_if >     {};
  struct key_in      : key< str_in >     {};
  struct key_return  : key< str_return > {};
  struct key_task    : key< str_task >   {};
  struct key_then    : key< str_then >   {};
  struct key_true    : key< str_true >   {};

  struct keyword : key< str_keyword > {};

  ////////////////////////////////////////////////////////////////
  // literals and identifiers
  ////////////////////////////////////////////////////////////////
  // I use two different name identifiers so they can have different actions
  // easily.
  struct num : seq <
    opt< sor< one< '+' >, one< '-' > > >,
    sor<
      seq< one< '.' >, plus< digit > >,
      seq< plus< digit >, opt< one< '.' >, star< digit > > > > >{};

  struct name : seq< not_at< keyword >, identifier > {};
  struct call_name : disable< name > {};

  ////////////////////////////////////////////////////////////////
  // Operators
  ////////////////////////////////////////////////////////////////
  struct op_add : one< '+' > {};
  struct op_sub : one< '-' > {};
  struct op_mul : one< '*' > {};
  struct op_div : one< '/' > {};
  struct op_eq  : string< '=', '=' > {};
  struct op_neq : string< '!', '=' > {};
  struct op_lte : string< '<', '=' > {};
  struct op_lt  : one< '<' > {};
  struct op_gte : string< '>', '=' > {};
  struct op_gt  : one< '>' > {};

  struct bin_op : sor< op_add, op_sub, op_mul, op_div, op_eq, op_neq, op_lte,
                       op_lt, op_gte, op_gt > {};
  ////////////////////////////////////////////////////////////////
  // Expressions
  ////////////////////////////////////////////////////////////////
  // There is no operator precedence. Use parentheses.
  struct expr;

  struct call : seq< at< pad< name, space >, one< '(' > >,
                     pad< call_name, space >, one< '(' >,
                     pad_opt< list< expr, one< ',' >, space >, space >,
                     one< ')' > >{};

  struct ifthen_expr : seq< key_if, pad< expr, space >,
                            key_then, pad< expr, space >,
                            key_else, pad< expr, space >,
                            key_end > {};

  struct expr_0 : pad<
    sor< ifthen_expr, call, num, name,
         seq< one< '(' >,  pad< expr, space >, one< ')' > > >,
    space > {};

  struct expr : seq< expr_0, opt< seq< bin_op, expr_0 > > > {};

  ////////////////////////////////////////////////////////////////
  // Functions and Blocks
  ////////////////////////////////////////////////////////////////
  struct prototype
    : seq< pad< name, space >,
           one< '(' >,
           pad_opt< list< name, one< ',' >, space >, space >,
           one< ')' > > {};

  struct func : seq< key_func, pad< prototype, space >,
                     one< '=' >, expr, one<';'> > {};

  struct task : seq< key_task, pad< prototype, space >,
                     one< '=' >, expr, one<';'> > {};
  ////////////////////////////////////////////////////////////////
  // Statements
  ////////////////////////////////////////////////////////////////
  struct extern_stat : seq< key_extern, pad< prototype, space>, one< ';' > > {};

  ////////////////////////////////////////////////////////////////
  // Root Grammar Node
  ////////////////////////////////////////////////////////////////
  struct grammar
    : must< star< pad < sor< func, task, extern_stat >, space > > , eof > {};


  ////////////////////////////////////////////////////////////////
  // Parsing Actions
  ////////////////////////////////////////////////////////////////
  // First we define a class which contains all necessary parsing state.
  // I do this because I am lazy, not because I am clever.
  template< typename Rule >
  struct build_ast
    : pegtl::nothing< Rule > {};

  /*
  struct parse_state {

  };

  template< typename Rule >
  struct build_ast
    : pegtl::nothing< Rule > {};

  template <> struct build_ast < grammar > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call) {
      // Print the AST
      ex.top()->print("AST", 0);
    }
  };

  template <> struct build_ast < num > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call) {
      ex.push(new NumExpr(stod(in.string())));
    }
  };

  template <> struct build_ast < name > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call) {
      ex.push(new NameExpr(in.string()));
    }
  };

  template <> struct build_ast < call_name > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call) {
      call.push(new NameExpr(in.string()));
    }
  };

  template <> struct build_ast < expr_0 > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call) {

    }
  }

  template <> struct build_ast < bin_op > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      BinOp op;
      std::string inS = in.string();

      switch(inS[0]) {
      case '+':
        op = kAdd;
        break;
      case '-':
        op = kSub;
        break;
      case '*':
        op = kMul;
        break;
      case '/':
        op = kDiv;
        break;
      case '=':
        if(inS.compare("==")) {
          op = kEq;
        } else {
          op = kAss;
        }
        break;
      case '!':
        op = kNEq;
        break;
      case '<':
        if(inS.compare("<=")) {
          op = kGTE;
        } else {
          op = kGT;
        }
        break;
      case '>':
        if(inS.compare(">=")) {
          op = kLTE;
        } else {
          op = kLT;
        }
        break;
      }

      ex.push(new BinExpr(op));
    }
  };

  template <> struct build_ast < call > {
    static void apply(const pegtl::input & in, std::stack<AST*> &ex,
                      std::stack<NameExpr*> call) {
      std::vector<Expr*> args;
    }
  };

  template <> struct build_ast < expr > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){

    }
  };

  template <> struct build_ast < ret_stat > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "RET_STAT : " << in.string() << std::endl;
    }
  };

  template <> struct build_ast < prototype > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "PROTOTYPE : " << in.string() << std::endl;
    }
  };

  template <> struct build_ast < extern_stat > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "EXTERN_STAT : " << in.string() << std::endl;
    }
  };

  template <> struct build_ast < block > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "BLOCK : " << in.string() << std::endl;
    }
  };
  template <> struct build_ast < func > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "FUNC : " << in.string() << std::endl;
    }
  };
  template <> struct build_ast < if_stat > {
    static void apply( const pegtl::input & in, std::stack<AST*> &ex,
                       std::stack<NameExpr*> call){
      std::cout << "IF_STAT : " << in.string() << std::endl;
    }
  };
  */
} // namespace parse
