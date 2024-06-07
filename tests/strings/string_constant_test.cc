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

#include <turbo/strings/internal/string_constant.h>

#include <turbo/meta/type_traits.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using turbo::strings_internal::MakeStringConstant;

struct Callable {
  constexpr std::string_view operator()() const {
    return std::string_view("Callable", 8);
  }
};

TEST(StringConstant, Traits) {
  constexpr auto str = MakeStringConstant(Callable{});
  using T = decltype(str);

  EXPECT_TRUE(std::is_empty<T>::value);
  EXPECT_TRUE(std::is_trivial<T>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<T>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<T>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<T>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<T>::value);
}

TEST(StringConstant, MakeFromCallable) {
  constexpr auto str = MakeStringConstant(Callable{});
  using T = decltype(str);
  EXPECT_EQ(Callable{}(), T::value);
  EXPECT_EQ(Callable{}(), str());
}

TEST(StringConstant, MakeFromStringConstant) {
  // We want to make sure the StringConstant itself is a valid input to the
  // factory function.
  constexpr auto str = MakeStringConstant(Callable{});
  constexpr auto str2 = MakeStringConstant(str);
  using T = decltype(str2);
  EXPECT_EQ(Callable{}(), T::value);
  EXPECT_EQ(Callable{}(), str2());
}

}  // namespace
