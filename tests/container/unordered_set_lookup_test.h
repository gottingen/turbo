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

#ifndef TURBO_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_
#define TURBO_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/container/hash_generator_testing.h>
#include <tests/container/hash_policy_testing.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

template <class UnordSet>
class LookupTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(LookupTest);

TYPED_TEST_P(LookupTest, Count) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values)
    EXPECT_EQ(0, m.count(v)) << ::testing::PrintToString(v);
  m.insert(values.begin(), values.end());
  for (const auto& v : values)
    EXPECT_EQ(1, m.count(v)) << ::testing::PrintToString(v);
}

TYPED_TEST_P(LookupTest, Find) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values)
    EXPECT_TRUE(m.end() == m.find(v)) << ::testing::PrintToString(v);
  m.insert(values.begin(), values.end());
  for (const auto& v : values) {
    typename TypeParam::iterator it = m.find(v);
    static_assert(std::is_same<const typename TypeParam::value_type&,
                               decltype(*it)>::value,
                  "");
    static_assert(std::is_same<const typename TypeParam::value_type*,
                               decltype(it.operator->())>::value,
                  "");
    EXPECT_TRUE(m.end() != it) << ::testing::PrintToString(v);
    EXPECT_EQ(v, *it) << ::testing::PrintToString(v);
  }
}

TYPED_TEST_P(LookupTest, EqualRange) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& v : values) {
    auto r = m.equal_range(v);
    ASSERT_EQ(0, std::distance(r.first, r.second));
  }
  m.insert(values.begin(), values.end());
  for (const auto& v : values) {
    auto r = m.equal_range(v);
    ASSERT_EQ(1, std::distance(r.first, r.second));
    EXPECT_EQ(v, *r.first);
  }
}

REGISTER_TYPED_TEST_SUITE_P(LookupTest, Count, Find, EqualRange);

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_UNORDERED_SET_LOOKUP_TEST_H_
