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

#include <turbo/container/internal/hash_policy_traits.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <new>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/container/internal/container_memory.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

using ::testing::MockFunction;
using ::testing::Return;
using ::testing::ReturnRef;

using Alloc = std::allocator<int>;
using Slot = int;

struct PolicyWithoutOptionalOps {
  using slot_type = Slot;
  using key_type = Slot;
  using init_type = Slot;

  static std::function<Slot&(Slot*)> element;
  static int apply(int v) { return apply_impl(v); }
  static std::function<int(int)> apply_impl;
  static std::function<Slot&(Slot*)> value;

  template <class Hash>
  static constexpr HashSlotFn get_hash_slot_fn() {
    return nullptr;
  }
};

std::function<int(int)> PolicyWithoutOptionalOps::apply_impl;
std::function<Slot&(Slot*)> PolicyWithoutOptionalOps::value;

struct Test : ::testing::Test {
  Test() {
    PolicyWithoutOptionalOps::apply_impl = [&](int a1) -> int {
      return apply.Call(a1);
    };
    PolicyWithoutOptionalOps::value = [&](Slot* a1) -> Slot& {
      return value.Call(a1);
    };
  }

  std::allocator<int> alloc;
  int a = 53;
  MockFunction<int(int)> apply;
  MockFunction<Slot&(Slot*)> value;
};

TEST_F(Test, apply) {
  EXPECT_CALL(apply, Call(42)).WillOnce(Return(1337));
  EXPECT_EQ(1337, (hash_policy_traits<PolicyWithoutOptionalOps>::apply(42)));
}

TEST_F(Test, value) {
  int b = 0;
  EXPECT_CALL(value, Call(&a)).WillOnce(ReturnRef(b));
  EXPECT_EQ(&b, &hash_policy_traits<PolicyWithoutOptionalOps>::value(&a));
}

struct Hash {
  size_t operator()(Slot a) const { return static_cast<size_t>(a) * 5; }
};

struct PolicyNoHashFn {
  using slot_type = Slot;
  using key_type = Slot;
  using init_type = Slot;

  static size_t* apply_called_count;

  static Slot& element(Slot* slot) { return *slot; }
  template <typename Fn>
  static size_t apply(const Fn& fn, int v) {
    ++(*apply_called_count);
    return fn(v);
  }

  template <class Hash>
  static constexpr HashSlotFn get_hash_slot_fn() {
    return nullptr;
  }
};

size_t* PolicyNoHashFn::apply_called_count;

struct PolicyCustomHashFn : PolicyNoHashFn {
  template <class Hash>
  static constexpr HashSlotFn get_hash_slot_fn() {
    return &TypeErasedApplyToSlotFn<Hash, int>;
  }
};

TEST(HashTest, PolicyNoHashFn_get_hash_slot_fn) {
  size_t apply_called_count = 0;
  PolicyNoHashFn::apply_called_count = &apply_called_count;

  Hash hasher;
  Slot value = 7;
  auto* fn = hash_policy_traits<PolicyNoHashFn>::get_hash_slot_fn<Hash>();
  EXPECT_NE(fn, nullptr);
  EXPECT_EQ(fn(&hasher, &value), hasher(value));
  EXPECT_EQ(apply_called_count, 1);
}

TEST(HashTest, PolicyCustomHashFn_get_hash_slot_fn) {
  size_t apply_called_count = 0;
  PolicyNoHashFn::apply_called_count = &apply_called_count;

  Hash hasher;
  Slot value = 7;
  auto* fn = hash_policy_traits<PolicyCustomHashFn>::get_hash_slot_fn<Hash>();
  EXPECT_EQ(fn, PolicyCustomHashFn::get_hash_slot_fn<Hash>());
  EXPECT_EQ(fn(&hasher, &value), hasher(value));
  EXPECT_EQ(apply_called_count, 0);
}

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
