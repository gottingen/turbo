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

#ifndef TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_MEMBERS_TEST_H_
#define TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_MEMBERS_TEST_H_

#include <type_traits>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/meta/type_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

template <class UnordMap>
class MembersTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(MembersTest);

template <typename T>
void UseType() {}

TYPED_TEST_P(MembersTest, Typedefs) {
  EXPECT_TRUE((std::is_same<std::pair<const typename TypeParam::key_type,
                                      typename TypeParam::mapped_type>,
                            typename TypeParam::value_type>()));
  EXPECT_TRUE((turbo::conjunction<
               turbo::negation<std::is_signed<typename TypeParam::size_type>>,
               std::is_integral<typename TypeParam::size_type>>()));
  EXPECT_TRUE((turbo::conjunction<
               std::is_signed<typename TypeParam::difference_type>,
               std::is_integral<typename TypeParam::difference_type>>()));
  EXPECT_TRUE((std::is_convertible<
               decltype(std::declval<const typename TypeParam::hasher&>()(
                   std::declval<const typename TypeParam::key_type&>())),
               size_t>()));
  EXPECT_TRUE((std::is_convertible<
               decltype(std::declval<const typename TypeParam::key_equal&>()(
                   std::declval<const typename TypeParam::key_type&>(),
                   std::declval<const typename TypeParam::key_type&>())),
               bool>()));
  EXPECT_TRUE((std::is_same<typename TypeParam::allocator_type::value_type,
                            typename TypeParam::value_type>()));
  EXPECT_TRUE((std::is_same<typename TypeParam::value_type&,
                            typename TypeParam::reference>()));
  EXPECT_TRUE((std::is_same<const typename TypeParam::value_type&,
                            typename TypeParam::const_reference>()));
  EXPECT_TRUE((std::is_same<typename std::allocator_traits<
                                typename TypeParam::allocator_type>::pointer,
                            typename TypeParam::pointer>()));
  EXPECT_TRUE(
      (std::is_same<typename std::allocator_traits<
                        typename TypeParam::allocator_type>::const_pointer,
                    typename TypeParam::const_pointer>()));
}

TYPED_TEST_P(MembersTest, SimpleFunctions) {
  EXPECT_GT(TypeParam().max_size(), 0);
}

TYPED_TEST_P(MembersTest, BeginEnd) {
  TypeParam t = {typename TypeParam::value_type{}};
  EXPECT_EQ(t.begin(), t.cbegin());
  EXPECT_EQ(t.end(), t.cend());
  EXPECT_NE(t.begin(), t.end());
  EXPECT_NE(t.cbegin(), t.cend());
}

REGISTER_TYPED_TEST_SUITE_P(MembersTest, Typedefs, SimpleFunctions, BeginEnd);

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_UNORDERED_MAP_MEMBERS_TEST_H_
