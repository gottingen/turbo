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

#include <turbo/container/internal/node_slot_policy.h>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/container/internal/hash_policy_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

using ::testing::Pointee;

struct Policy : node_slot_policy<int&, Policy> {
  using key_type = int;
  using init_type = int;

  template <class Alloc>
  static int* new_element(Alloc* alloc, int value) {
    return new int(value);
  }

  template <class Alloc>
  static void delete_element(Alloc* alloc, int* elem) {
    delete elem;
  }
};

using NodePolicy = hash_policy_traits<Policy>;

struct NodeTest : ::testing::Test {
  std::allocator<int> alloc;
  int n = 53;
  int* a = &n;
};

TEST_F(NodeTest, ConstructDestroy) {
  NodePolicy::construct(&alloc, &a, 42);
  EXPECT_THAT(a, Pointee(42));
  NodePolicy::destroy(&alloc, &a);
}

TEST_F(NodeTest, transfer) {
  int s = 42;
  int* b = &s;
  NodePolicy::transfer(&alloc, &a, &b);
  EXPECT_EQ(&s, a);
  EXPECT_TRUE(NodePolicy::transfer_uses_memcpy());
}

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
