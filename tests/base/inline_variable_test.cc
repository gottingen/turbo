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

#include <type_traits>

#include <turbo/base/internal/inline_variable.h>
#include <tests/base/inline_variable_testing.h>

#include <gtest/gtest.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace inline_variable_testing_internal {
namespace {

TEST(InlineVariableTest, Constexpr) {
  static_assert(inline_variable_foo.value == 5, "");
  static_assert(other_inline_variable_foo.value == 5, "");
  static_assert(inline_variable_int == 5, "");
  static_assert(other_inline_variable_int == 5, "");
}

TEST(InlineVariableTest, DefaultConstructedIdentityEquality) {
  EXPECT_EQ(get_foo_a().value, 5);
  EXPECT_EQ(get_foo_b().value, 5);
  EXPECT_EQ(&get_foo_a(), &get_foo_b());
}

TEST(InlineVariableTest, DefaultConstructedIdentityInequality) {
  EXPECT_NE(&inline_variable_foo, &other_inline_variable_foo);
}

TEST(InlineVariableTest, InitializedIdentityEquality) {
  EXPECT_EQ(get_int_a(), 5);
  EXPECT_EQ(get_int_b(), 5);
  EXPECT_EQ(&get_int_a(), &get_int_b());
}

TEST(InlineVariableTest, InitializedIdentityInequality) {
  EXPECT_NE(&inline_variable_int, &other_inline_variable_int);
}

TEST(InlineVariableTest, FunPtrType) {
  static_assert(
      std::is_same<void(*)(),
                   std::decay<decltype(inline_variable_fun_ptr)>::type>::value,
      "");
}

}  // namespace
}  // namespace inline_variable_testing_internal
TURBO_NAMESPACE_END
}  // namespace turbo
