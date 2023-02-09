//
// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_LOG_CHECK_TEST_IMPL_H_
#define TURBO_LOG_CHECK_TEST_IMPL_H_

// Verify that both sets of macros behave identically by parameterizing the
// entire test file.
#ifndef TURBO_TEST_CHECK
#error TURBO_TEST_CHECK must be defined for these tests to work.
#endif

#include <ostream>
#include <string>

#include "turbo/base/status.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/platform/port.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

// NOLINTBEGIN(misc-definitions-in-headers)

namespace turbo_log_internal {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Not;

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

#if GTEST_HAS_DEATH_TEST

TEST(CHECKDeathTest, TestBasicValues) {
  TURBO_TEST_CHECK(true);

  EXPECT_DEATH(TURBO_TEST_CHECK(false), "Check failed: false");

  int i = 2;
  TURBO_TEST_CHECK(i != 3);  // NOLINT
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(CHECKTest, TestLogicExpressions) {
  int i = 5;
  TURBO_TEST_CHECK(i > 0 && i < 10);
  TURBO_TEST_CHECK(i < 0 || i > 3);
}

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L
TURBO_CONST_INIT const auto global_var_check = [](int i) {
  TURBO_TEST_CHECK(i > 0);  // NOLINT
  return i + 1;
}(3);

TURBO_CONST_INIT const auto global_var = [](int i) {
  TURBO_TEST_CHECK_GE(i, 0);  // NOLINT
  return i + 1;
}(global_var_check);
#endif  // TURBO_INTERNAL_CPLUSPLUS_LANG

TEST(CHECKTest, TestPlacementsInCompoundStatements) {
  // check placement inside if/else clauses
  if (true) TURBO_TEST_CHECK(true);

  if (false)
    ;  // NOLINT
  else
    TURBO_TEST_CHECK(true);

  switch (0)
  case 0:
    TURBO_TEST_CHECK(true);  // NOLINT

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L
  constexpr auto var = [](int i) {
    TURBO_TEST_CHECK(i > 0);  // NOLINT
    return i + 1;
  }(global_var);
  (void)var;
#endif  // TURBO_INTERNAL_CPLUSPLUS_LANG
}

TEST(CHECKTest, TestBoolConvertible) {
  struct Tester {
  } tester;
  TURBO_TEST_CHECK([&]() { return &tester; }());
}

#if GTEST_HAS_DEATH_TEST

TEST(CHECKDeathTest, TestChecksWithSideEffects) {
  int var = 0;
  TURBO_TEST_CHECK([&var]() {
    ++var;
    return true;
  }());
  EXPECT_EQ(var, 1);

  EXPECT_DEATH(TURBO_TEST_CHECK([&var]() {
                 ++var;
                 return false;
               }()) << var,
               "Check failed: .* 2");
}

#endif  // GTEST_HAS_DEATH_TEST

template <int a, int b>
constexpr int sum() {
  return a + b;
}
#define MACRO_ONE 1
#define TEMPLATE_SUM(a, b) sum<a, b>()
#define CONCAT(a, b) a b
#define IDENTITY(x) x

TEST(CHECKTest, TestPassingMacroExpansion) {
  TURBO_TEST_CHECK(IDENTITY(true));
  TURBO_TEST_CHECK_EQ(TEMPLATE_SUM(MACRO_ONE, 2), 3);
  TURBO_TEST_CHECK_STREQ(CONCAT("x", "y"), "xy");
}

#if GTEST_HAS_DEATH_TEST

TEST(CHECKTest, TestMacroExpansionInMessage) {
  auto MessageGen = []() { TURBO_TEST_CHECK(IDENTITY(false)); };
  EXPECT_DEATH(MessageGen(), HasSubstr("IDENTITY(false)"));
}

TEST(CHECKTest, TestNestedMacroExpansionInMessage) {
  EXPECT_DEATH(TURBO_TEST_CHECK(IDENTITY(false)), HasSubstr("IDENTITY(false)"));
}

TEST(CHECKTest, TestMacroExpansionCompare) {
  EXPECT_DEATH(TURBO_TEST_CHECK_EQ(IDENTITY(false), IDENTITY(true)),
               HasSubstr("IDENTITY(false) == IDENTITY(true)"));
  EXPECT_DEATH(TURBO_TEST_CHECK_GT(IDENTITY(1), IDENTITY(2)),
               HasSubstr("IDENTITY(1) > IDENTITY(2)"));
}

TEST(CHECKTest, TestMacroExpansionStrCompare) {
  EXPECT_DEATH(TURBO_TEST_CHECK_STREQ(IDENTITY("x"), IDENTITY("y")),
               HasSubstr("IDENTITY(\"x\") == IDENTITY(\"y\")"));
  EXPECT_DEATH(TURBO_TEST_CHECK_STRCASENE(IDENTITY("a"), IDENTITY("A")),
               HasSubstr("IDENTITY(\"a\") != IDENTITY(\"A\")"));
}

TEST(CHECKTest, TestMacroExpansionStatus) {
  EXPECT_DEATH(
      TURBO_TEST_CHECK_OK(IDENTITY(turbo::FailedPreconditionError("message"))),
      HasSubstr("IDENTITY(turbo::FailedPreconditionError(\"message\"))"));
}

TEST(CHECKTest, TestMacroExpansionComma) {
  EXPECT_DEATH(TURBO_TEST_CHECK(TEMPLATE_SUM(MACRO_ONE, 2) == 4),
               HasSubstr("TEMPLATE_SUM(MACRO_ONE, 2) == 4"));
}

TEST(CHECKTest, TestMacroExpansionCommaCompare) {
  EXPECT_DEATH(
      TURBO_TEST_CHECK_EQ(TEMPLATE_SUM(2, MACRO_ONE), TEMPLATE_SUM(3, 2)),
      HasSubstr("TEMPLATE_SUM(2, MACRO_ONE) == TEMPLATE_SUM(3, 2)"));
  EXPECT_DEATH(
      TURBO_TEST_CHECK_GT(TEMPLATE_SUM(2, MACRO_ONE), TEMPLATE_SUM(3, 2)),
      HasSubstr("TEMPLATE_SUM(2, MACRO_ONE) > TEMPLATE_SUM(3, 2)"));
}

TEST(CHECKTest, TestMacroExpansionCommaStrCompare) {
  EXPECT_DEATH(TURBO_TEST_CHECK_STREQ(CONCAT("x", "y"), "z"),
               HasSubstr("CONCAT(\"x\", \"y\") == \"z\""));
  EXPECT_DEATH(TURBO_TEST_CHECK_STRNE(CONCAT("x", "y"), "xy"),
               HasSubstr("CONCAT(\"x\", \"y\") != \"xy\""));
}

#endif  // GTEST_HAS_DEATH_TEST

#undef TEMPLATE_SUM
#undef CONCAT
#undef MACRO
#undef ONE

#if GTEST_HAS_DEATH_TEST

TEST(CHECKDeachTest, TestOrderOfInvocationsBetweenCheckAndMessage) {
  int counter = 0;

  auto GetStr = [&counter]() -> std::string {
    return counter++ == 0 ? "" : "non-empty";
  };

  EXPECT_DEATH(TURBO_TEST_CHECK(!GetStr().empty()) << GetStr(),
               HasSubstr("non-empty"));
}

TEST(CHECKTest, TestSecondaryFailure) {
  auto FailingRoutine = []() {
    TURBO_TEST_CHECK(false) << "Secondary";
    return false;
  };
  EXPECT_DEATH(TURBO_TEST_CHECK(FailingRoutine()) << "Primary",
               AllOf(HasSubstr("Secondary"), Not(HasSubstr("Primary"))));
}

TEST(CHECKTest, TestSecondaryFailureInMessage) {
  auto MessageGen = []() {
    TURBO_TEST_CHECK(false) << "Secondary";
    return "Primary";
  };
  EXPECT_DEATH(TURBO_TEST_CHECK(false) << MessageGen(),
               AllOf(HasSubstr("Secondary"), Not(HasSubstr("Primary"))));
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(CHECKTest, TestBinaryChecksWithPrimitives) {
  TURBO_TEST_CHECK_EQ(1, 1);
  TURBO_TEST_CHECK_NE(1, 2);
  TURBO_TEST_CHECK_GE(1, 1);
  TURBO_TEST_CHECK_GE(2, 1);
  TURBO_TEST_CHECK_LE(1, 1);
  TURBO_TEST_CHECK_LE(1, 2);
  TURBO_TEST_CHECK_GT(2, 1);
  TURBO_TEST_CHECK_LT(1, 2);
}

// For testing using CHECK*() on anonymous enums.
enum { CASE_A, CASE_B };

TEST(CHECKTest, TestBinaryChecksWithEnumValues) {
  // Tests using CHECK*() on anonymous enums.
  TURBO_TEST_CHECK_EQ(CASE_A, CASE_A);
  TURBO_TEST_CHECK_NE(CASE_A, CASE_B);
  TURBO_TEST_CHECK_GE(CASE_A, CASE_A);
  TURBO_TEST_CHECK_GE(CASE_B, CASE_A);
  TURBO_TEST_CHECK_LE(CASE_A, CASE_A);
  TURBO_TEST_CHECK_LE(CASE_A, CASE_B);
  TURBO_TEST_CHECK_GT(CASE_B, CASE_A);
  TURBO_TEST_CHECK_LT(CASE_A, CASE_B);
}

TEST(CHECKTest, TestBinaryChecksWithNullptr) {
  const void* p_null = nullptr;
  const void* p_not_null = &p_null;
  TURBO_TEST_CHECK_EQ(p_null, nullptr);
  TURBO_TEST_CHECK_EQ(nullptr, p_null);
  TURBO_TEST_CHECK_NE(p_not_null, nullptr);
  TURBO_TEST_CHECK_NE(nullptr, p_not_null);
}

#if GTEST_HAS_DEATH_TEST

// Test logging of various char-typed values by failing CHECK*().
TEST(CHECKDeathTest, TestComparingCharsValues) {
  {
    char a = ';';
    char b = 'b';
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. 'b'\\)");
    b = 1;
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. char value 1\\)");
  }
  {
    signed char a = ';';
    signed char b = 'b';
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. 'b'\\)");
    b = -128;
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. signed char value -128\\)");
  }
  {
    unsigned char a = ';';
    unsigned char b = 'b';
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. 'b'\\)");
    b = 128;
    EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
                 "Check failed: a == b \\(';' vs. unsigned char value 128\\)");
  }
}

TEST(CHECKDeathTest, TestNullValuesAreReportedCleanly) {
  const char* a = nullptr;
  const char* b = nullptr;
  EXPECT_DEATH(TURBO_TEST_CHECK_NE(a, b),
               "Check failed: a != b \\(\\(null\\) vs. \\(null\\)\\)");

  a = "xx";
  EXPECT_DEATH(TURBO_TEST_CHECK_EQ(a, b),
               "Check failed: a == b \\(xx vs. \\(null\\)\\)");
  EXPECT_DEATH(TURBO_TEST_CHECK_EQ(b, a),
               "Check failed: b == a \\(\\(null\\) vs. xx\\)");

  std::nullptr_t n{};
  EXPECT_DEATH(TURBO_TEST_CHECK_NE(n, nullptr),
               "Check failed: n != nullptr \\(\\(null\\) vs. \\(null\\)\\)");
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(CHECKTest, TestSTREQ) {
  TURBO_TEST_CHECK_STREQ("this", "this");
  TURBO_TEST_CHECK_STREQ(nullptr, nullptr);
  TURBO_TEST_CHECK_STRCASEEQ("this", "tHiS");
  TURBO_TEST_CHECK_STRCASEEQ(nullptr, nullptr);
  TURBO_TEST_CHECK_STRNE("this", "tHiS");
  TURBO_TEST_CHECK_STRNE("this", nullptr);
  TURBO_TEST_CHECK_STRCASENE("this", "that");
  TURBO_TEST_CHECK_STRCASENE(nullptr, "that");
  TURBO_TEST_CHECK_STREQ((std::string("a") + "b").c_str(), "ab");
  TURBO_TEST_CHECK_STREQ(std::string("test").c_str(),
                        (std::string("te") + std::string("st")).c_str());
}

TEST(CHECKTest, TestComparisonPlacementsInCompoundStatements) {
  // check placement inside if/else clauses
  if (true) TURBO_TEST_CHECK_EQ(1, 1);
  if (true) TURBO_TEST_CHECK_STREQ("c", "c");

  if (false)
    ;  // NOLINT
  else
    TURBO_TEST_CHECK_LE(0, 1);

  if (false)
    ;  // NOLINT
  else
    TURBO_TEST_CHECK_STRNE("a", "b");

  switch (0)
  case 0:
    TURBO_TEST_CHECK_NE(1, 0);

  switch (0)
  case 0:
    TURBO_TEST_CHECK_STRCASEEQ("A", "a");

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L
  constexpr auto var = [](int i) {
    TURBO_TEST_CHECK_GT(i, 0);
    return i + 1;
  }(global_var);
  (void)var;

  // CHECK_STR... checks are not supported in constexpr routines.
  // constexpr auto var2 = [](int i) {
  //  TURBO_TEST_CHECK_STRNE("c", "d");
  //  return i + 1;
  // }(global_var);

#if defined(__GNUC__)
  int var3 = (({ TURBO_TEST_CHECK_LE(1, 2); }), global_var < 10) ? 1 : 0;
  (void)var3;

  int var4 = (({ TURBO_TEST_CHECK_STREQ("a", "a"); }), global_var < 10) ? 1 : 0;
  (void)var4;
#endif  // __GNUC__
#endif  // TURBO_INTERNAL_CPLUSPLUS_LANG
}

TEST(CHECKTest, TestDCHECK) {
#ifdef NDEBUG
  TURBO_TEST_DCHECK(1 == 2) << " DCHECK's shouldn't be compiled in normal mode";
#endif
  TURBO_TEST_DCHECK(1 == 1);  // NOLINT(readability/check)
  TURBO_TEST_DCHECK_EQ(1, 1);
  TURBO_TEST_DCHECK_NE(1, 2);
  TURBO_TEST_DCHECK_GE(1, 1);
  TURBO_TEST_DCHECK_GE(2, 1);
  TURBO_TEST_DCHECK_LE(1, 1);
  TURBO_TEST_DCHECK_LE(1, 2);
  TURBO_TEST_DCHECK_GT(2, 1);
  TURBO_TEST_DCHECK_LT(1, 2);

  // Test DCHECK on std::nullptr_t
  const void* p_null = nullptr;
  const void* p_not_null = &p_null;
  TURBO_TEST_DCHECK_EQ(p_null, nullptr);
  TURBO_TEST_DCHECK_EQ(nullptr, p_null);
  TURBO_TEST_DCHECK_NE(p_not_null, nullptr);
  TURBO_TEST_DCHECK_NE(nullptr, p_not_null);
}

TEST(CHECKTest, TestQCHECK) {
  // The tests that QCHECK does the same as CHECK
  TURBO_TEST_QCHECK(1 == 1);  // NOLINT(readability/check)
  TURBO_TEST_QCHECK_EQ(1, 1);
  TURBO_TEST_QCHECK_NE(1, 2);
  TURBO_TEST_QCHECK_GE(1, 1);
  TURBO_TEST_QCHECK_GE(2, 1);
  TURBO_TEST_QCHECK_LE(1, 1);
  TURBO_TEST_QCHECK_LE(1, 2);
  TURBO_TEST_QCHECK_GT(2, 1);
  TURBO_TEST_QCHECK_LT(1, 2);

  // Tests using QCHECK*() on anonymous enums.
  TURBO_TEST_QCHECK_EQ(CASE_A, CASE_A);
  TURBO_TEST_QCHECK_NE(CASE_A, CASE_B);
  TURBO_TEST_QCHECK_GE(CASE_A, CASE_A);
  TURBO_TEST_QCHECK_GE(CASE_B, CASE_A);
  TURBO_TEST_QCHECK_LE(CASE_A, CASE_A);
  TURBO_TEST_QCHECK_LE(CASE_A, CASE_B);
  TURBO_TEST_QCHECK_GT(CASE_B, CASE_A);
  TURBO_TEST_QCHECK_LT(CASE_A, CASE_B);
}

TEST(CHECKTest, TestQCHECKPlacementsInCompoundStatements) {
  // check placement inside if/else clauses
  if (true) TURBO_TEST_QCHECK(true);

  if (false)
    ;  // NOLINT
  else
    TURBO_TEST_QCHECK(true);

  if (false)
    ;  // NOLINT
  else
    TURBO_TEST_QCHECK(true);

  switch (0)
  case 0:
    TURBO_TEST_QCHECK(true);

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L
  constexpr auto var = [](int i) {
    TURBO_TEST_QCHECK(i > 0);  // NOLINT
    return i + 1;
  }(global_var);
  (void)var;

#if defined(__GNUC__)
  int var2 = (({ TURBO_TEST_CHECK_LE(1, 2); }), global_var < 10) ? 1 : 0;
  (void)var2;
#endif  // __GNUC__
#endif  // TURBO_INTERNAL_CPLUSPLUS_LANG
}

class ComparableType {
 public:
  explicit ComparableType(int v) : v_(v) {}

  void MethodWithCheck(int i) {
    TURBO_TEST_CHECK_EQ(*this, i);
    TURBO_TEST_CHECK_EQ(i, *this);
  }

  int Get() const { return v_; }

 private:
  friend bool operator==(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ == rhs.v_;
  }
  friend bool operator!=(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ != rhs.v_;
  }
  friend bool operator<(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ < rhs.v_;
  }
  friend bool operator<=(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ <= rhs.v_;
  }
  friend bool operator>(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ > rhs.v_;
  }
  friend bool operator>=(const ComparableType& lhs, const ComparableType& rhs) {
    return lhs.v_ >= rhs.v_;
  }
  friend bool operator==(const ComparableType& lhs, int rhs) {
    return lhs.v_ == rhs;
  }
  friend bool operator==(int lhs, const ComparableType& rhs) {
    return lhs == rhs.v_;
  }

  friend std::ostream& operator<<(std::ostream& out, const ComparableType& v) {
    return out << "ComparableType{" << v.Get() << "}";
  }

  int v_;
};

TEST(CHECKTest, TestUserDefinedCompOp) {
  TURBO_TEST_CHECK_EQ(ComparableType{0}, ComparableType{0});
  TURBO_TEST_CHECK_NE(ComparableType{1}, ComparableType{2});
  TURBO_TEST_CHECK_LT(ComparableType{1}, ComparableType{2});
  TURBO_TEST_CHECK_LE(ComparableType{1}, ComparableType{2});
  TURBO_TEST_CHECK_GT(ComparableType{2}, ComparableType{1});
  TURBO_TEST_CHECK_GE(ComparableType{2}, ComparableType{2});
}

TEST(CHECKTest, TestCheckInMethod) {
  ComparableType v{1};
  v.MethodWithCheck(1);
}

TEST(CHECKDeathTest, TestUserDefinedStreaming) {
  ComparableType v1{1};
  ComparableType v2{2};

  EXPECT_DEATH(
      TURBO_TEST_CHECK_EQ(v1, v2),
      HasSubstr(
          "Check failed: v1 == v2 (ComparableType{1} vs. ComparableType{2})"));
}

}  // namespace turbo_log_internal

// NOLINTEND(misc-definitions-in-headers)

#endif  // TURBO_LOG_CHECK_TEST_IMPL_H_
