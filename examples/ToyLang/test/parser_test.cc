#include <pegtl.hh>
#include "gtest/gtest.h"

#include "parse.h"

////////////////////////////////////////////////////////////////
// The problem: Function macros consume input before variadic templates.
// The solution: These REALLY DUMB MARCROS WHY IS C++ TERRIBLE?
////////////////////////////////////////////////////////////////
#define EXPECT_BAD_S try {
#define EXPECT_BAD_E FAIL(); } catch (...) { SUCCEED(); }
#define EXPECT_BAD_E_(message) FAIL() << message; } catch (...) { SUCCEED(); }

#define EXPECT_GOOD_S try {
#define EXPECT_GOOD_E SUCCEED(); } catch (...) { FAIL(); }
#define EXPECT_GOOD_E_(message) SUCCEED(); } catch (...) { FAIL() << message; }

namespace {
  class ParseTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
  };
}

using namespace pegtl;

TEST_F(ParseTest, Expr_GoodName) {
  char* name = "mYna_me2";

  EXPECT_GOOD_S
    (parse< must< parse::name, eof > >(0, &name));
  EXPECT_GOOD_E;
}

TEST_F(ParseTest, Expr_BadName) {
  char* name1 = "for";
  char* name2 = "10name";
  char* name3 = "x+";

  EXPECT_BAD_S
    (parse< must< parse::name, eof > >(0, &name1));
  EXPECT_BAD_E;

  EXPECT_BAD_S
    (parse< must< parse::name, eof > >(0, &name2));
  EXPECT_BAD_E;

  EXPECT_BAD_S
    (parse< must< parse::name, eof > >(0, &name3));
  EXPECT_BAD_E;
}

TEST_F(ParseTest, Expr_GoodCall) {
  char* call1 = "f()";
  char* call2 = "f (  )";
  char* call3 = "g( x)";
  char* call4 = "g( x,y, z )";

  EXPECT_GOOD_S
    (parse< must< parse::call, eof > >(0, &call1));
  EXPECT_GOOD_E;

  EXPECT_GOOD_S
    (parse< must< parse::call, eof > >(0, &call2));
  EXPECT_GOOD_E;

  EXPECT_GOOD_S
    (parse< must< parse::call, eof > >(0, &call3));
  EXPECT_GOOD_E;

  EXPECT_GOOD_S
    (parse< must< parse::call, eof > >(0, &call4));
  EXPECT_GOOD_E;
}

TEST_F(ParseTest, Expr_BadCall) {
  char* call1 = "f (x";
  char* call2 = "10 (x)";

  EXPECT_BAD_S
    (parse< must< parse::call, eof > >(0, &call1));
  EXPECT_BAD_E;

  EXPECT_BAD_S
    (parse< must< parse::call, eof > >(0, &call2));
  EXPECT_BAD_E;
}

TEST_F(ParseTest, Expr_GoodIf) {
  char* if1 = "if x then y else z end";
  char* if2 = "if f(x) then f(y) else f(z) end";
  char* if3 = "if if x then y else z end then f else g end";

  EXPECT_GOOD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if1));
  EXPECT_GOOD_E;

  EXPECT_GOOD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if2));
  EXPECT_GOOD_E;

  EXPECT_GOOD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if3));
  EXPECT_GOOD_E;
}

TEST_F(ParseTest, Expr_BadIf) {
  char* if1 = "if x then y else z";
  char* if2 = "if f(x) then f(y) f(z) end";
  char* if3 = "if if x y else z end then f else g end";

  EXPECT_BAD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if1));
  EXPECT_BAD_E;

  EXPECT_BAD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if2));
  EXPECT_BAD_E;

  EXPECT_BAD_S
    (parse< must< parse::ifthen_expr, eof > >(0, &if3));
  EXPECT_BAD_E;
}

TEST_F(ParseTest, Expr_GoodBin) {
  char* bin1  = "x";
  char* bin2  = "x+y";
  char* bin3  = "x     +y";
  char* bin4  = "x     +  y";
  char* bin5  = "x+y-z";
  char* bin6  = "x+y-z*10";
  char* bin7  = "x+y-(z*10)";
  char* bin8  = "x+y-(if z then 10 else 11 end)";
  char* bin9  = "((((((X))))))";
  char* bin10 = "10+-2";
  char* bin11  = "x + y - z";

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin1));
  EXPECT_GOOD_E_(bin1);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin2));
  EXPECT_GOOD_E_(bin2);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin3));
  EXPECT_GOOD_E_(bin3);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin4));
  EXPECT_GOOD_E_(bin4);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin5));
  EXPECT_GOOD_E_(bin5);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin6));
  EXPECT_GOOD_E_(bin6);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin7));
  EXPECT_GOOD_E_(bin7);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin8));
  EXPECT_GOOD_E_(bin8);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin9));
  EXPECT_GOOD_E_(bin9);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin10));
  EXPECT_GOOD_E_(bin10);

  EXPECT_GOOD_S
    (parse< must< parse::expr, eof > >(0, &bin11));
  EXPECT_GOOD_E_(bin11);
}


TEST_F(ParseTest, Expr_BadBin) {
  char* bin1 = "((x)";
  char* bin2 = "x+";
  char* bin3 = "+x";

  EXPECT_BAD_S
    (parse< must< parse::expr, eof > >(0, &bin1));
  EXPECT_BAD_E_(bin1);

  EXPECT_BAD_S
    (parse< must< parse::expr, eof > >(0, &bin2));
  EXPECT_BAD_E_(bin2);

  EXPECT_BAD_S
    (parse< must< parse::expr, eof > >(0, &bin3));
  EXPECT_BAD_E_(bin3);
}

TEST_F(ParseTest, Prototype_Good) {
  char* pro1 = "f()";
  char* pro2 = "f (    )";
  char* pro3 = "f(x, y)";

  EXPECT_GOOD_S
    (parse< must< parse::prototype, eof > >(0, &pro1));
  EXPECT_GOOD_E_(pro1);

  EXPECT_GOOD_S
    (parse< must< parse::prototype, eof > >(0, &pro2));
  EXPECT_GOOD_E_(pro2);

  EXPECT_GOOD_S
    (parse< must< parse::prototype, eof > >(0, &pro3));
  EXPECT_GOOD_E_(pro3);
}

TEST_F(ParseTest, Prototype_Bad) {
  char* pro1 = "f(";
  char* pro2 = "10()";
  char* pro3 = "f(10, x+y, z)";

  EXPECT_BAD_S
    (parse< must< parse::prototype, eof > >(0, &pro1));
  EXPECT_BAD_E_(pro1);

  EXPECT_BAD_S
    (parse< must< parse::prototype, eof > >(0, &pro2));
  EXPECT_BAD_E_(pro2);

  EXPECT_BAD_S
    (parse< must< parse::prototype, eof > >(0, &pro3));
  EXPECT_BAD_E_(pro3);
}

TEST_F(ParseTest, Func_Good) {
  char* func1 = "func id(x) = x;";
  char* func2 = "func wif(c,    t , f ) = if c then t else f end;";
  char* func3 = "func noArg() = 10 + if c then t else f end;";

  EXPECT_GOOD_S
    (parse< must< parse::func, eof > >(0, &func1));
  EXPECT_GOOD_E_(func1);

  EXPECT_GOOD_S
    (parse< must< parse::func, eof > >(0, &func2));
  EXPECT_GOOD_E_(func2);

  EXPECT_GOOD_S
    (parse< must< parse::func, eof > >(0, &func3));
  EXPECT_GOOD_E_(func3);
}

TEST_F(ParseTest, Func_Bad) {
  char* func1 = "func id(x) = x";
  char* func2 = "func wif(10,    t , f ) = if c then t else f end;";
  char* func3 = "func noArg()  10 + if c then t else f end;";

  EXPECT_BAD_S
    (parse< must< parse::func, eof > >(0, &func1));
  EXPECT_BAD_E_(func1);

  EXPECT_BAD_S
    (parse< must< parse::func, eof > >(0, &func2));
  EXPECT_BAD_E_(func2);

  EXPECT_BAD_S
    (parse< must< parse::func, eof > >(0, &func3));
  EXPECT_BAD_E_(func3);
}

TEST_F(ParseTest, Task_Good) {
  char* task1 = "task id(x) = x;";
  char* task2 = "task wif(c,    t , f ) = if c then t else f end;";
  char* task3 = "task noArg() = 10 + if c then t else f end;";

  EXPECT_GOOD_S
    (parse< must< parse::task, eof > >(0, &task1));
  EXPECT_GOOD_E_(task1);

  EXPECT_GOOD_S
    (parse< must< parse::task, eof > >(0, &task2));
  EXPECT_GOOD_E_(task2);

  EXPECT_GOOD_S
    (parse< must< parse::task, eof > >(0, &task3));
  EXPECT_GOOD_E_(task3);
}

TEST_F(ParseTest, Task_Bad) {
  char* task1 = "task id(x) = x";
  char* task2 = "task wif(10,    t , f ) = if c then t else f end;";
  char* task3 = "task noArg()  10 + if c then t else f end;";

  EXPECT_BAD_S
    (parse< must< parse::task, eof > >(0, &task1));
  EXPECT_BAD_E_(task1);

  EXPECT_BAD_S
    (parse< must< parse::task, eof > >(0, &task2));
  EXPECT_BAD_E_(task2);

  EXPECT_BAD_S
    (parse< must< parse::task, eof > >(0, &task3));
  EXPECT_BAD_E_(task3);
}

TEST_F(ParseTest, Grammer_Good) {
  char* prog =
    "extern print(x);"
    "task fact(x) = if x < 1 then 1 else fact(x-1)*x end;"
    "func id(x) = x;"
    "func main() = print(fact(10));";
  EXPECT_NO_THROW
    (parse< parse::grammar >(0, &prog));
}
