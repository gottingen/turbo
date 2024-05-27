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

#include <turbo/types/compare.h>

#include <gtest/gtest.h>
#include <turbo/base/casts.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

// This is necessary to avoid a bunch of lint warnings suggesting that we use
// EXPECT_EQ/etc., which doesn't work in this case because they convert the `0`
// to an int, which can't be converted to the unspecified zero type.
bool Identity(bool b) { return b; }

TEST(Compare, PartialOrdering) {
  EXPECT_TRUE(Identity(partial_ordering::less < 0));
  EXPECT_TRUE(Identity(0 > partial_ordering::less));
  EXPECT_TRUE(Identity(partial_ordering::less <= 0));
  EXPECT_TRUE(Identity(0 >= partial_ordering::less));
  EXPECT_TRUE(Identity(partial_ordering::equivalent == 0));
  EXPECT_TRUE(Identity(0 == partial_ordering::equivalent));
  EXPECT_TRUE(Identity(partial_ordering::greater > 0));
  EXPECT_TRUE(Identity(0 < partial_ordering::greater));
  EXPECT_TRUE(Identity(partial_ordering::greater >= 0));
  EXPECT_TRUE(Identity(0 <= partial_ordering::greater));
  EXPECT_TRUE(Identity(partial_ordering::unordered != 0));
  EXPECT_TRUE(Identity(0 != partial_ordering::unordered));
  EXPECT_FALSE(Identity(partial_ordering::unordered < 0));
  EXPECT_FALSE(Identity(0 < partial_ordering::unordered));
  EXPECT_FALSE(Identity(partial_ordering::unordered <= 0));
  EXPECT_FALSE(Identity(0 <= partial_ordering::unordered));
  EXPECT_FALSE(Identity(partial_ordering::unordered > 0));
  EXPECT_FALSE(Identity(0 > partial_ordering::unordered));
  EXPECT_FALSE(Identity(partial_ordering::unordered >= 0));
  EXPECT_FALSE(Identity(0 >= partial_ordering::unordered));
  const partial_ordering values[] = {
      partial_ordering::less, partial_ordering::equivalent,
      partial_ordering::greater, partial_ordering::unordered};
  for (const auto& lhs : values) {
    for (const auto& rhs : values) {
      const bool are_equal = &lhs == &rhs;
      EXPECT_EQ(lhs == rhs, are_equal);
      EXPECT_EQ(lhs != rhs, !are_equal);
    }
  }
}

TEST(Compare, WeakOrdering) {
  EXPECT_TRUE(Identity(weak_ordering::less < 0));
  EXPECT_TRUE(Identity(0 > weak_ordering::less));
  EXPECT_TRUE(Identity(weak_ordering::less <= 0));
  EXPECT_TRUE(Identity(0 >= weak_ordering::less));
  EXPECT_TRUE(Identity(weak_ordering::equivalent == 0));
  EXPECT_TRUE(Identity(0 == weak_ordering::equivalent));
  EXPECT_TRUE(Identity(weak_ordering::greater > 0));
  EXPECT_TRUE(Identity(0 < weak_ordering::greater));
  EXPECT_TRUE(Identity(weak_ordering::greater >= 0));
  EXPECT_TRUE(Identity(0 <= weak_ordering::greater));
  const weak_ordering values[] = {
      weak_ordering::less, weak_ordering::equivalent, weak_ordering::greater};
  for (const auto& lhs : values) {
    for (const auto& rhs : values) {
      const bool are_equal = &lhs == &rhs;
      EXPECT_EQ(lhs == rhs, are_equal);
      EXPECT_EQ(lhs != rhs, !are_equal);
    }
  }
}

TEST(Compare, StrongOrdering) {
  EXPECT_TRUE(Identity(strong_ordering::less < 0));
  EXPECT_TRUE(Identity(0 > strong_ordering::less));
  EXPECT_TRUE(Identity(strong_ordering::less <= 0));
  EXPECT_TRUE(Identity(0 >= strong_ordering::less));
  EXPECT_TRUE(Identity(strong_ordering::equal == 0));
  EXPECT_TRUE(Identity(0 == strong_ordering::equal));
  EXPECT_TRUE(Identity(strong_ordering::equivalent == 0));
  EXPECT_TRUE(Identity(0 == strong_ordering::equivalent));
  EXPECT_TRUE(Identity(strong_ordering::greater > 0));
  EXPECT_TRUE(Identity(0 < strong_ordering::greater));
  EXPECT_TRUE(Identity(strong_ordering::greater >= 0));
  EXPECT_TRUE(Identity(0 <= strong_ordering::greater));
  const strong_ordering values[] = {
      strong_ordering::less, strong_ordering::equal, strong_ordering::greater};
  for (const auto& lhs : values) {
    for (const auto& rhs : values) {
      const bool are_equal = &lhs == &rhs;
      EXPECT_EQ(lhs == rhs, are_equal);
      EXPECT_EQ(lhs != rhs, !are_equal);
    }
  }
  EXPECT_TRUE(Identity(strong_ordering::equivalent == strong_ordering::equal));
}

TEST(Compare, Conversions) {
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::less) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::less) < 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::less) <= 0));
  EXPECT_TRUE(Identity(
      implicit_cast<partial_ordering>(weak_ordering::equivalent) == 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::greater) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::greater) > 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(weak_ordering::greater) >= 0));

  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::less) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::less) < 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::less) <= 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::equal) == 0));
  EXPECT_TRUE(Identity(
      implicit_cast<partial_ordering>(strong_ordering::equivalent) == 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::greater) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::greater) > 0));
  EXPECT_TRUE(
      Identity(implicit_cast<partial_ordering>(strong_ordering::greater) >= 0));

  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::less) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::less) < 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::less) <= 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::equal) == 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::equivalent) == 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::greater) != 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::greater) > 0));
  EXPECT_TRUE(
      Identity(implicit_cast<weak_ordering>(strong_ordering::greater) >= 0));
}

struct WeakOrderingLess {
  template <typename T>
  turbo::weak_ordering operator()(const T& a, const T& b) const {
    return a < b ? turbo::weak_ordering::less
                 : a == b ? turbo::weak_ordering::equivalent
                          : turbo::weak_ordering::greater;
  }
};

TEST(CompareResultAsLessThan, SanityTest) {
  EXPECT_FALSE(turbo::compare_internal::compare_result_as_less_than(false));
  EXPECT_TRUE(turbo::compare_internal::compare_result_as_less_than(true));

  EXPECT_TRUE(
      turbo::compare_internal::compare_result_as_less_than(weak_ordering::less));
  EXPECT_FALSE(turbo::compare_internal::compare_result_as_less_than(
      weak_ordering::equivalent));
  EXPECT_FALSE(turbo::compare_internal::compare_result_as_less_than(
      weak_ordering::greater));
}

TEST(DoLessThanComparison, SanityTest) {
  std::less<int> less;
  WeakOrderingLess weak;

  EXPECT_TRUE(turbo::compare_internal::do_less_than_comparison(less, -1, 0));
  EXPECT_TRUE(turbo::compare_internal::do_less_than_comparison(weak, -1, 0));

  EXPECT_FALSE(turbo::compare_internal::do_less_than_comparison(less, 10, 10));
  EXPECT_FALSE(turbo::compare_internal::do_less_than_comparison(weak, 10, 10));

  EXPECT_FALSE(turbo::compare_internal::do_less_than_comparison(less, 10, 5));
  EXPECT_FALSE(turbo::compare_internal::do_less_than_comparison(weak, 10, 5));
}

TEST(CompareResultAsOrdering, SanityTest) {
  EXPECT_TRUE(
      Identity(turbo::compare_internal::compare_result_as_ordering(-1) < 0));
  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(-1) == 0));
  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(-1) > 0));
  EXPECT_TRUE(Identity(turbo::compare_internal::compare_result_as_ordering(
                           weak_ordering::less) < 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::less) == 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::less) > 0));

  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(0) < 0));
  EXPECT_TRUE(
      Identity(turbo::compare_internal::compare_result_as_ordering(0) == 0));
  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(0) > 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::equivalent) < 0));
  EXPECT_TRUE(Identity(turbo::compare_internal::compare_result_as_ordering(
                           weak_ordering::equivalent) == 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::equivalent) > 0));

  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(1) < 0));
  EXPECT_FALSE(
      Identity(turbo::compare_internal::compare_result_as_ordering(1) == 0));
  EXPECT_TRUE(
      Identity(turbo::compare_internal::compare_result_as_ordering(1) > 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::greater) < 0));
  EXPECT_FALSE(Identity(turbo::compare_internal::compare_result_as_ordering(
                            weak_ordering::greater) == 0));
  EXPECT_TRUE(Identity(turbo::compare_internal::compare_result_as_ordering(
                           weak_ordering::greater) > 0));
}

TEST(DoThreeWayComparison, SanityTest) {
  std::less<int> less;
  WeakOrderingLess weak;

  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, -1, 0) < 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, -1, 0) == 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, -1, 0) > 0));
  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, -1, 0) < 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, -1, 0) == 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, -1, 0) > 0));

  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 10) < 0));
  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 10) == 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 10) > 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 10) < 0));
  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 10) == 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 10) > 0));

  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 5) < 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 5) == 0));
  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(less, 10, 5) > 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 5) < 0));
  EXPECT_FALSE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 5) == 0));
  EXPECT_TRUE(Identity(
      turbo::compare_internal::do_three_way_comparison(weak, 10, 5) > 0));
}

#ifdef __cpp_inline_variables
TEST(Compare, StaticAsserts) {
  static_assert(partial_ordering::less < 0, "");
  static_assert(partial_ordering::equivalent == 0, "");
  static_assert(partial_ordering::greater > 0, "");
  static_assert(partial_ordering::unordered != 0, "");

  static_assert(weak_ordering::less < 0, "");
  static_assert(weak_ordering::equivalent == 0, "");
  static_assert(weak_ordering::greater > 0, "");

  static_assert(strong_ordering::less < 0, "");
  static_assert(strong_ordering::equal == 0, "");
  static_assert(strong_ordering::equivalent == 0, "");
  static_assert(strong_ordering::greater > 0, "");
}
#endif  // __cpp_inline_variables

}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
