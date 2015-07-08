#include "gtest/gtest.h"

namespace {
  class ParseTests : public ::testing::Test {
  protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
  }
}

TEST_F(ParseTests, DummyTest) {
  ASSERT_TRUE(true);
}
