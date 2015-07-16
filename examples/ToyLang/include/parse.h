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

#include <deque>
#include <stack>
#include <string>
#include <iostream>

#include <pegtl.hh>

#include "AST.h"

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

  struct expr : seq< expr_0, star< seq< bin_op, expr_0 > > > {};

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

  /**
   * This structure keeps track of the state of parsing. For this structure
   * to work, it is vital the grammar has no backtracking.
   *
   * `funcs` and `externs` are lists of all completed functions/tasks and
   * externed functions. When parsing is complete, these lists will be
   * used for further processing.
   *
   * protoCur contains the prototype for the current function or extern.
   *
   * exprStack is the main workspace for parsing. It acts as a stack of "contexts".
   * Any expression construct that can be nested will push a new context onto
   * the stack. Every context is essentially a list of expressions. When
   * whatever pushed the context onto the stack finishes parsing, it will
   * know how to interpret that list of expressions.
   *
   * The typical pattern is
   *   1) Identify the need for a new context, and push new context.
   *   2) Parse expressions and put them into the context list.
   *   3) Finish this context, make a new AST structure, free context, and
   *      add new AST structure to the parent context.
   */
  struct parse_state {
    std::vector< Func* > funcs;
    std::vector< Proto* > externs;

    Proto* protoCur;

    std::stack< std::deque<Expr*>* > exprStack;
  };

  /**
   * Rule: DEFAULT
   * Do nothing, unless stated otherwise.
   */
  template< typename Rule >
  struct build_ast
    : pegtl::nothing< Rule > {};

  /**
   * Rule: num
   * Place a new NumExpr into the context list.
   */
  template <> struct build_ast < num > {
    static void apply( const pegtl::input & in, parse_state &state) {
      state.exprStack.top()->push_back(new NumExpr(stod(in.string())));
    }
  };

  /**
   * Rule: name
   * Place a new NameExpr into the context list.
   *
   * Note that this name is not the start of a call. It COULD be used by a
   * prototype.
   */
  template <> struct build_ast < name > {
    static void apply( const pegtl::input & in, parse_state &state) {
      state.exprStack.top()->push_back(new NameExpr(in.string()));
    }
  };

  /**
   * Rule: call (Removes Context)
   * Front of context list is name of callee. The rest of the list is
   * argument expressions. Collect this information, and remove the top
   * context, and place a new CallExpr in the next context list.
   */
  template <> struct build_ast < call > {
    static void apply(const pegtl::input & in, parse_state &state) {
      std::vector<Expr*> args;
      NameExpr* name = static_cast<NameExpr*>(state.exprStack.top()->front());
      state.exprStack.top()->pop_front();

      while(!state.exprStack.top()->empty()) {
        args.push_back(state.exprStack.top()->back());
        state.exprStack.top()->pop_back();
      }

      // No longer need this expr context
      delete state.exprStack.top();
      state.exprStack.pop();

      state.exprStack.top()->push_back(new CallExpr(name, args));
    }
  };


  /**
   * Rule: call_name (Makes Context)
   * Make a new Context, and push a NameExpr onto that context.
   */
  template <> struct build_ast < call_name > {
    static void apply( const pegtl::input & in, parse_state &state) {
      state.exprStack.push(new std::deque<Expr*>);
      state.exprStack.top()->push_back(new NameExpr(in.string()));
    }
  };

  /**
   * Rule: bin_op
   * Make a new BinExpr, and put it onto the current context list, with no
   * lhs or rhs
   */
  template <> struct build_ast < bin_op > {
    static void apply( const pegtl::input & in, parse_state &state) {
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

    state.exprStack.top()->push_back(new BinExpr(op));
    }
  };

  /**
   * Rule: prototype (Removes Context)
   * Assumes first element of Context list is function name, and the rest
   * are args. Removes context, and updates `protoCur`.
   */
  template <> struct build_ast < prototype > {
    static void apply( const pegtl::input & in, parse_state &state) {
      // I need something to push the INITIAL expr context for this
      std::vector<NameExpr*> args;
      NameExpr* name = static_cast<NameExpr*>(state.exprStack.top()->front());
      state.exprStack.top()->pop_front();

      while(!state.exprStack.top()->empty()) {
        args.push_back(static_cast<NameExpr*>(state.exprStack.top()->back()));
        state.exprStack.top()->pop_back();
      }

      // No longer need this expr context
      delete state.exprStack.top();
      state.exprStack.pop();

      state.protoCur = new Proto(name, args);
    }
  };

  /**
   * Rule: extern_stat
   * Moves `protoCur` into `externs` list.
   */
  template <> struct build_ast < extern_stat > {
    static void apply( const pegtl::input & in, parse_state &state) {
      state.externs.push_back(state.protoCur);
    }
  };

  /**
   * Rule: func (Removes Context)
   * Takes first (presumably *only*) expression from context, and assumes that
   * is the body. Constructs new function from this body and `protoCur`, and puts
   * it into the `funcs` list.
   */
  template <> struct build_ast < func > {
    static void apply( const pegtl::input & in, parse_state &state) {
      Expr* body = state.exprStack.top()->front();
      state.exprStack.top()->pop_front();
      delete state.exprStack.top();
      state.exprStack.pop();

      state.funcs.push_back(new Func(false, state.protoCur, body));
    }
  };

  /**
   * Rule: task (Removes Context)
   * Takes first (presumably *only*) expression from context, and assumes that
   * is the body. Constructs new function from this body and `protoCur`, and puts
   * it into the `funcs` list.
   */
  template <> struct build_ast < task > {
    static void apply( const pegtl::input & in, parse_state &state) {
      Expr* body = state.exprStack.top()->front();
      state.exprStack.top()->pop_front();
      delete state.exprStack.top();
      state.exprStack.pop();

      state.funcs.push_back(new Func(true, state.protoCur, body));
    }
  };

  /**
   * Rule: ifthen_expr (Removes Context)
   * Assumes first element in context list is condition, second truth, third
   * false. Constructs new IfExpr, removes context, and puts IfExpr into
   * next context list.
   */
  template <> struct build_ast < ifthen_expr > {
    static void apply( const pegtl::input & in, parse_state &state) {
      Expr* c = (*state.exprStack.top())[0];
      Expr* t = (*state.exprStack.top())[1];
      Expr* f = (*state.exprStack.top())[2];

      delete state.exprStack.top();
      state.exprStack.pop();

      state.exprStack.top()->push_back(new IfExpr(t, f, c));
    }
  };

  /**
   * Rule: expr
   * Two cases: Either the last expression was atomic, or a binop. We look at
   * the current context. If there are at least three elements, we inspect the
   * second to last. If it is an unfinished binop, we finish it, and put it
   * back into the context list. This process is repeated until all expressions
   * in the context seem good.
   *
   * Note: this does not mean there will be ONE expression left in the ctx.
   *       Calls and Ifs require there be multiple expressions.
   */
  template <> struct build_ast < expr > {
    static void apply( const pegtl::input & in, parse_state &state) {
      auto ctx = state.exprStack.top();

      // As long as there is at least three things in the ctx, it may be
      // a bin op
      while(ctx->size() >= 3) {
        Expr* r = ctx->back();
        ctx->pop_back();
        Expr* b = ctx->back();
        ctx->pop_back();
        Expr* l = ctx->back();
        ctx->pop_back();
        BinExpr* bin;

        if(b->type == kBin
           && ((bin = static_cast<BinExpr*>(b))->lhs == nullptr) ) {
          // An unfinished binop was found. Finish and put back
          // into context.
          bin->lhs = l;
          bin->rhs = r;
          ctx->push_back(bin);
        } else {
          // No unfinished binop was found. There is no need to keep
          // looking. Put everything back and give up.
          ctx->push_back(l);
          ctx->push_back(b);
          ctx->push_back(r);
          break;
        }
      }
    }
  };

  /**
   * Rule: key_func (Makes Two Contexts)
   * Puts two contexts onto the stack. One for the prototype, and another for the body.
   */
  template <> struct build_ast < key_func > {
    static void apply( const pegtl::input & in, parse_state &state) {
      // We need to push a context for the prototype AND a context for the
      // body.
      state.exprStack.push(new std::deque<Expr*>());
      state.exprStack.push(new std::deque<Expr*>());
    }
  };

  /**
   * Rule: key_task (Makes Two Contexts)
   * Puts two contexts onto the stack. One for the prototype, and another for the body.
   */
  template <> struct build_ast < key_task > {
    static void apply( const pegtl::input & in, parse_state &state) {
      // We need to push a context for the prototype AND a context for the
      // body.
      state.exprStack.push(new std::deque<Expr*>());
      state.exprStack.push(new std::deque<Expr*>());
    }
  };

  /**
   * Rule: key_extern (Makes Context)
   * Puts a context on the stack, for prototype.
   */
  template <> struct build_ast < key_extern > {
    static void apply( const pegtl::input & in, parse_state &state) {
      // We need to push a context for the prototype
      state.exprStack.push(new std::deque<Expr*>());
    }
  };

  /**
   * Rule: key_if (Makes Context)
   * Puts a context on the stack, which will eventually contain three expressions.
   * This context will be consumed by ifthen_expr.
   */
  template <> struct build_ast < key_if > {
    static void apply( const pegtl::input & in, parse_state &state) {
      // We need to push a context for the if/then/else expr
      state.exprStack.push(new std::deque<Expr*>());
    }
  };

} // namespace parse
