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

#ifndef TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_LOOKUP_TEST_H_
#define TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_LOOKUP_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/container/hash_generator_testing.h>
#include <tests/container/hash_policy_testing.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

template <class UnordMap>
class LookupTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(LookupTest);

TYPED_TEST_P(LookupTest, At) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m(values.begin(), values.end());
  for (const auto& p : values) {
    const auto& val = m.at(p.first);
    EXPECT_EQ(p.second, val) << ::testing::PrintToString(p.first);
  }
}

TYPED_TEST_P(LookupTest, OperatorBracket) {
  using T = hash_internal::GeneratedType<TypeParam>;
  using V = typename TypeParam::mapped_type;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& p : values) {
    auto& val = m[p.first];
    EXPECT_EQ(V(), val) << ::testing::PrintToString(p.first);
    val = p.second;
  }
  for (const auto& p : values)
    EXPECT_EQ(p.second, m[p.first]) << ::testing::PrintToString(p.first);
}

TYPED_TEST_P(LookupTest, Count) {
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& p : values)
    EXPECT_EQ(0, m.count(p.first)) << ::testing::PrintToString(p.first);
  m.insert(values.begin(), values.end());
  for (const auto& p : values)
    EXPECT_EQ(1, m.count(p.first)) << ::testing::PrintToString(p.first);
}

TYPED_TEST_P(LookupTest, Find) {
  using std::get;
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& p : values)
    EXPECT_TRUE(m.end() == m.find(p.first))
        << ::testing::PrintToString(p.first);
  m.insert(values.begin(), values.end());
  for (const auto& p : values) {
    auto it = m.find(p.first);
    EXPECT_TRUE(m.end() != it) << ::testing::PrintToString(p.first);
    EXPECT_EQ(p.second, get<1>(*it)) << ::testing::PrintToString(p.first);
  }
}

TYPED_TEST_P(LookupTest, EqualRange) {
  using std::get;
  using T = hash_internal::GeneratedType<TypeParam>;
  std::vector<T> values;
  std::generate_n(std::back_inserter(values), 10,
                  hash_internal::Generator<T>());
  TypeParam m;
  for (const auto& p : values) {
    auto r = m.equal_range(p.first);
    ASSERT_EQ(0, std::distance(r.first, r.second));
  }
  m.insert(values.begin(), values.end());
  for (const auto& p : values) {
    auto r = m.equal_range(p.first);
    ASSERT_EQ(1, std::distance(r.first, r.second));
    EXPECT_EQ(p.second, get<1>(*r.first)) << ::testing::PrintToString(p.first);
  }
}

REGISTER_TYPED_TEST_SUITE_P(LookupTest, At, OperatorBracket, Count, Find,
                            EqualRange);

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_LOOKUP_TEST_H_
