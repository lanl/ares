#include <pegtl.hh>
#include "gtest/gtest.h"

#include "parse.h"

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
  EXPECT_NO_THROW
    (parse< must< parse::name > >(0, &name));
}

TEST_F(ParseTest, Expr_BadName) {
  char* name1 = "for";
  char* name2 = "10name";
  char* name3 = "x+";

  EXPECT_ANY_THROW
    (parse< must< parse::name > >(0, &name1));
  EXPECT_ANY_THROW
    (parse< must< parse::name > >(0, &name2));
  try {
    (parse< must< parse::name, eof > >(0, &name3));
    FAIL();
  } catch (...) {
    SUCCEED();
  }
}

TEST_F(ParseTest, Expr_GoodCall) {
  char* call1 = "f()";
  char* call2 = "f (  )";
  char* call3 = "g( x)";
  char* call4 = "g( x,y, z )";

  EXPECT_NO_THROW
    (parse< must< parse::call > >(0, &call1));
  EXPECT_NO_THROW
    (parse< must< parse::call > >(0, &call2));
  EXPECT_NO_THROW
    (parse< must< parse::call > >(0, &call3));
  EXPECT_NO_THROW
    (parse< must< parse::call > >(0, &call4));
}

TEST_F(ParseTest, Expr_BadCall) {
  char* call1 = "f (x";
  char* call2 = "10 (x)";
  EXPECT_ANY_THROW
    (parse< must< parse::call > >(0, &call1));
  EXPECT_ANY_THROW
    (parse< must< parse::call > >(0, &call2));
}

TEST_F(ParseTest, Expr_GoodIf) {
  char* if1 = "if x then y else z end";
  char* if2 = "if f(x) then f(y) else f(z) end";
  char* if3 = "if if x then y else z end then f else g end";

  EXPECT_NO_THROW
    (parse< must< parse::ifthen_expr > >(0, &if1));
  EXPECT_NO_THROW
    (parse< must< parse::ifthen_expr > >(0, &if2));
  EXPECT_NO_THROW
    (parse< must< parse::ifthen_expr > >(0, &if3));
}

TEST_F(ParseTest, Expr_BadIf) {
  char* if1 = "if x then y else z";
  char* if2 = "if f(x) then f(y) f(z) end";
  char* if3 = "if if x y else z end then f else g end";

  EXPECT_ANY_THROW
    (parse< must< parse::ifthen_expr > >(0, &if1));
  EXPECT_ANY_THROW
    (parse< must< parse::ifthen_expr > >(0, &if2));
  EXPECT_ANY_THROW
    (parse< must< parse::ifthen_expr > >(0, &if3));
}

TEST_F(ParseTest, Expr_GoodBin) {
  char* bin1  = "x";
  char* bin2  = "x+y";
  char* bin3  = "x     +y";
  char* bin4  = "x     +  y";
  char* bin5  = "x+y-z";
  char* bin6  = "x+y-z*10";
  char* bin7  = "x+y-(z*10)";
  char* bin8  = "x+y-(if z then 10 else 11 done)";
  char* bin9  = "((((((X))))))";
  char* bin10 = "10+-2";

  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin1));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin2));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin3));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin4));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin5));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin6));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin7));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin8));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin9));
  EXPECT_NO_THROW
    (parse< must< parse::expr > >(0, &bin10));
}


TEST_F(ParseTest, Expr_BadBin) {
  char* bin1 = "((x)";
  char* bin2 = "x+";
  char* bin3 = "+x";

  EXPECT_ANY_THROW
    (parse< must< parse::expr > >(0, &bin1));
  try {
    (parse< must< parse::expr, eof > >(0, &bin2));
    FAIL();
  } catch (...) {
    SUCCEED();
  }
  EXPECT_ANY_THROW
    (parse< must< parse::expr > >(0, &bin3));
}

TEST_F(ParseTest, Prototype_Good) {
  char* pro1 = "f()";
  char* pro2 = "f (    )";
  char* pro3 = "f(x, y)";

  EXPECT_NO_THROW
    (parse< must< parse::prototype > >(0, &pro1));
  EXPECT_NO_THROW
    (parse< must< parse::prototype > >(0, &pro2));
  EXPECT_NO_THROW
    (parse< must< parse::prototype > >(0, &pro3));
}

TEST_F(ParseTest, Prototype_Bad) {
  char* pro1 = "f(";
  char* pro2 = "10()";
  char* pro3 = "f(10, x+y, z)";

  EXPECT_ANY_THROW
    (parse< must< parse::prototype > >(0, &pro1));
  EXPECT_ANY_THROW
    (parse< must< parse::prototype > >(0, &pro2));
  EXPECT_ANY_THROW
    (parse< must< parse::prototype > >(0, &pro3));
}

TEST_F(ParseTest, Func_Good) {
  char* func1 = "func id(x) = x;";
  char* func2 = "func wif(c,    t , f ) = if c then t else f end;";
  char* func3 = "func noArg() = 10 + if c then t else f end;";

  EXPECT_NO_THROW
    (parse< must< parse::func > >(0, &func1));
  EXPECT_NO_THROW
    (parse< must< parse::func > >(0, &func2));
  EXPECT_NO_THROW
    (parse< must< parse::func > >(0, &func3));
}

TEST_F(ParseTest, Func_Bad) {
  char* func1 = "func id(x) = x";
  char* func2 = "func wif(10,    t , f ) = if c then t else f end;";
  char* func3 = "func noArg()  10 + if c then t else f end;";

  EXPECT_ANY_THROW
    (parse< must< parse::func > >(0, &func1));
  EXPECT_ANY_THROW
    (parse< must< parse::func > >(0, &func2));
  EXPECT_ANY_THROW
    (parse< must< parse::func > >(0, &func3));
}

TEST_F(ParseTest, Task_Good) {
  char* task1 = "task id(x) = x;";
  char* task2 = "task wif(c,    t , f ) = if c then t else f end;";
  char* task3 = "task noArg() = 10 + if c then t else f end;";

  EXPECT_NO_THROW
    (parse< must< parse::task > >(0, &task1));
  EXPECT_NO_THROW
    (parse< must< parse::task > >(0, &task2));
  EXPECT_NO_THROW
    (parse< must< parse::task > >(0, &task3));
}

TEST_F(ParseTest, Task_Bad) {
  char* task1 = "task id(x) = x";
  char* task2 = "task wif(10,    t , f ) = if c then t else f end;";
  char* task3 = "task noArg()  10 + if c then t else f end;";

  EXPECT_ANY_THROW
    (parse< must< parse::task > >(0, &task1));
  EXPECT_ANY_THROW
    (parse< must< parse::task > >(0, &task2));
  EXPECT_ANY_THROW
    (parse< must< parse::task > >(0, &task3));
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
