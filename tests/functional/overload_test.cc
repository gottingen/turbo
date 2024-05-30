// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/functional/overload.h>

#include <cstdint>
#include <string>
#include <type_traits>

#include <turbo/base/config.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/variant.h>

#if defined(TURBO_INTERNAL_CPLUSPLUS_LANG) && \
    TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L

#include <gtest/gtest.h>

namespace {

TEST(OverloadTest, DispatchConsidersTypeWithAutoFallback) {
  auto overloaded = turbo::Overload{
      [](int v) { return turbo::str_cat("int ", v); },
      [](double v) { return turbo::str_cat("double ", v); },
      [](const char* v) { return turbo::str_cat("const char* ", v); },
      [](auto v) { return turbo::str_cat("auto ", v); },
  };

  EXPECT_EQ("int 1", overloaded(1));
  EXPECT_EQ("double 2.5", overloaded(2.5));
  EXPECT_EQ("const char* hello", overloaded("hello"));
  EXPECT_EQ("auto 1.5", overloaded(1.5f));
}

TEST(OverloadTest, DispatchConsidersNumberOfArguments) {
  auto overloaded = turbo::Overload{
      [](int a) { return a + 1; },
      [](int a, int b) { return a * b; },
      []() -> turbo::string_view { return "none"; },
  };

  EXPECT_EQ(3, overloaded(2));
  EXPECT_EQ(21, overloaded(3, 7));
  EXPECT_EQ("none", overloaded());
}

TEST(OverloadTest, SupportsConstantEvaluation) {
  auto overloaded = turbo::Overload{
      [](int a) { return a + 1; },
      [](int a, int b) { return a * b; },
      []() -> turbo::string_view { return "none"; },
  };

  static_assert(overloaded() == "none");
  static_assert(overloaded(2) == 3);
  static_assert(overloaded(3, 7) == 21);
}

TEST(OverloadTest, PropogatesDefaults) {
  auto overloaded = turbo::Overload{
      [](int a, int b = 5) { return a * b; },
      [](double c) { return c; },
  };

  EXPECT_EQ(21, overloaded(3, 7));
  EXPECT_EQ(35, overloaded(7));
  EXPECT_EQ(2.5, overloaded(2.5));
}

TEST(OverloadTest, AmbiguousWithDefaultsNotInvocable) {
  auto overloaded = turbo::Overload{
      [](int a, int b = 5) { return a * b; },
      [](int c) { return c; },
  };

  static_assert(!std::is_invocable_v<decltype(overloaded), int>);
  static_assert(std::is_invocable_v<decltype(overloaded), int, int>);
}

TEST(OverloadTest, AmbiguousDuplicatesNotInvocable) {
  auto overloaded = turbo::Overload{
      [](int a) { return a; },
      [](int c) { return c; },
  };

  static_assert(!std::is_invocable_v<decltype(overloaded), int>);
}

TEST(OverloadTest, AmbiguousConversionNotInvocable) {
  auto overloaded = turbo::Overload{
      [](uint16_t a) { return a; },
      [](uint64_t c) { return c; },
  };

  static_assert(!std::is_invocable_v<decltype(overloaded), int>);
}

TEST(OverloadTest, AmbiguousConversionWithAutoNotInvocable) {
  auto overloaded = turbo::Overload{
      [](auto a) { return a; },
      [](auto c) { return c; },
  };

  static_assert(!std::is_invocable_v<decltype(overloaded), int>);
}

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L

TEST(OverloadTest, AmbiguousConversionWithAutoAndTemplateNotInvocable) {
  auto overloaded = turbo::Overload{
      [](auto a) { return a; },
      []<class T>(T c) { return c; },
  };

  static_assert(!std::is_invocable_v<decltype(overloaded), int>);
}

TEST(OverloadTest, DispatchConsidersTypeWithTemplateFallback) {
  auto overloaded = turbo::Overload{
      [](int a) { return a; },
      []<class T>(T c) { return c * 2; },
  };

  EXPECT_EQ(7, overloaded(7));
  EXPECT_EQ(14.0, overloaded(7.0));
}

#endif  // TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L

TEST(OverloadTest, DispatchConsidersSfinae) {
  auto overloaded = turbo::Overload{
      [](auto a) -> decltype(a + 1) { return a + 1; },
  };

  static_assert(std::is_invocable_v<decltype(overloaded), int>);
  static_assert(!std::is_invocable_v<decltype(overloaded), std::string>);
}

TEST(OverloadTest, VariantVisitDispatchesCorrectly) {
  turbo::variant<int, double, std::string> v(1);
  auto overloaded = turbo::Overload{
      [](int) -> turbo::string_view { return "int"; },
      [](double) -> turbo::string_view { return "double"; },
      [](const std::string&) -> turbo::string_view { return "string"; },
  };

  EXPECT_EQ("int", turbo::visit(overloaded, v));
  v = 1.1;
  EXPECT_EQ("double", turbo::visit(overloaded, v));
  v = "hello";
  EXPECT_EQ("string", turbo::visit(overloaded, v));
}

TEST(OverloadTest, VariantVisitWithAutoFallbackDispatchesCorrectly) {
  turbo::variant<std::string, int32_t, int64_t> v(int32_t{1});
  auto overloaded = turbo::Overload{
      [](const std::string& s) { return s.size(); },
      [](const auto& s) { return sizeof(s); },
  };

  EXPECT_EQ(4, turbo::visit(overloaded, v));
  v = int64_t{1};
  EXPECT_EQ(8, turbo::visit(overloaded, v));
  v = std::string("hello");
  EXPECT_EQ(5, turbo::visit(overloaded, v));
}

// This API used to be exported as a function, so it should also work fine to
// use parantheses when initializing it.
TEST(OverloadTest, UseWithParentheses) {
  const auto overloaded =
      turbo::Overload([](const std::string& s) { return s.size(); },
                     [](const auto& s) { return sizeof(s); });

  turbo::variant<std::string, int32_t, int64_t> v(int32_t{1});
  EXPECT_EQ(4, turbo::visit(overloaded, v));

  v = int64_t{1};
  EXPECT_EQ(8, turbo::visit(overloaded, v));

  v = std::string("hello");
  EXPECT_EQ(5, turbo::visit(overloaded, v));
}

TEST(OverloadTest, HasConstexprConstructor) {
  constexpr auto overloaded = turbo::Overload{
      [](int v) { return turbo::str_cat("int ", v); },
      [](double v) { return turbo::str_cat("double ", v); },
      [](const char* v) { return turbo::str_cat("const char* ", v); },
      [](auto v) { return turbo::str_cat("auto ", v); },
  };

  EXPECT_EQ("int 1", overloaded(1));
  EXPECT_EQ("double 2.5", overloaded(2.5));
  EXPECT_EQ("const char* hello", overloaded("hello"));
  EXPECT_EQ("auto 1.5", overloaded(1.5f));
}

}  // namespace

#endif
