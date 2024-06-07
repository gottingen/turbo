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

#include <turbo/meta/internal/if_constexpr.h>

#include <utility>

#include <gtest/gtest.h>

namespace {

struct Empty {};
struct HasFoo {
  int foo() const { return 1; }
};

TEST(IfConstexpr, Basic) {
  int i = 0;
  turbo::utility_internal::IfConstexpr<false>(
      [&](const auto& t) { i = t.foo(); }, Empty{});
  EXPECT_EQ(i, 0);

  turbo::utility_internal::IfConstexpr<false>(
      [&](const auto& t) { i = t.foo(); }, HasFoo{});
  EXPECT_EQ(i, 0);

  turbo::utility_internal::IfConstexpr<true>(
      [&](const auto& t) { i = t.foo(); }, HasFoo{});
  EXPECT_EQ(i, 1);
}

TEST(IfConstexprElse, Basic) {
  EXPECT_EQ(turbo::utility_internal::IfConstexprElse<false>(
      [&](const auto& t) { return t.foo(); }, [&](const auto&) { return 2; },
      Empty{}), 2);

  EXPECT_EQ(turbo::utility_internal::IfConstexprElse<false>(
      [&](const auto& t) { return t.foo(); }, [&](const auto&) { return 2; },
      HasFoo{}), 2);

  EXPECT_EQ(turbo::utility_internal::IfConstexprElse<true>(
      [&](const auto& t) { return t.foo(); }, [&](const auto&) { return 2; },
      HasFoo{}), 1);
}

struct HasFooRValue {
  int foo() && { return 1; }
};
struct RValueFunc {
  void operator()(HasFooRValue&& t) && { *i = std::move(t).foo(); }

  int* i = nullptr;
};

TEST(IfConstexpr, RValues) {
  int i = 0;
  RValueFunc func = {&i};
  turbo::utility_internal::IfConstexpr<false>(
      std::move(func), HasFooRValue{});
  EXPECT_EQ(i, 0);

  func = RValueFunc{&i};
  turbo::utility_internal::IfConstexpr<true>(
      std::move(func), HasFooRValue{});
  EXPECT_EQ(i, 1);
}

}  // namespace
