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

#include <turbo/container/flat_hash_set.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/container/internal/container_memory.h>
#include <tests/container/hash_generator_testing.h>
#include <tests/container/test_allocator.h>
#include <tests/container/unordered_set_constructor_test.h>
#include <tests/container/unordered_set_lookup_test.h>
#include <tests/container/unordered_set_members_test.h>
#include <tests/container/unordered_set_modifiers_test.h>
#include <turbo/log/check.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

using ::turbo::container_internal::hash_internal::Enum;
using ::turbo::container_internal::hash_internal::EnumClass;
using ::testing::IsEmpty;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

// Check that turbo::flat_hash_set works in a global constructor.
struct BeforeMain {
  BeforeMain() {
    turbo::flat_hash_set<int> x;
    x.insert(1);
    CHECK(!x.contains(0)) << "x should not contain 0";
    CHECK(x.contains(1)) << "x should contain 1";
  }
};
const BeforeMain before_main;

template <class T>
using Set =
    turbo::flat_hash_set<T, StatefulTestingHash, StatefulTestingEqual, Alloc<T>>;

using SetTypes =
    ::testing::Types<Set<int>, Set<std::string>, Set<Enum>, Set<EnumClass>>;

INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, ConstructorTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, LookupTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, MembersTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(FlatHashSet, ModifiersTest, SetTypes);
/*
TEST(FlatHashSet, EmplaceString) {
  std::vector<std::string> v = {"a", "b"};
  turbo::flat_hash_set<turbo::string_view> hs(v.begin(), v.end());
  EXPECT_THAT(hs, UnorderedElementsAreArray(v));
}*/

TEST(FlatHashSet, BitfieldArgument) {
  union {
    int n : 1;
  };
  n = 0;
  turbo::flat_hash_set<int> s = {n};
  s.insert(n);
  s.insert(s.end(), n);
  s.insert({n});
  s.erase(n);
  s.count(n);
  s.prefetch(n);
  s.find(n);
  s.contains(n);
  s.equal_range(n);
}

TEST(FlatHashSet, MergeExtractInsert) {
  struct Hash {
    size_t operator()(const std::unique_ptr<int>& p) const { return *p; }
  };
  struct Eq {
    bool operator()(const std::unique_ptr<int>& a,
                    const std::unique_ptr<int>& b) const {
      return *a == *b;
    }
  };
  turbo::flat_hash_set<std::unique_ptr<int>, Hash, Eq> set1, set2;
  set1.insert(turbo::make_unique<int>(7));
  set1.insert(turbo::make_unique<int>(17));

  set2.insert(turbo::make_unique<int>(7));
  set2.insert(turbo::make_unique<int>(19));

  EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17)));
  EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(19)));

  set1.merge(set2);

  EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17), Pointee(19)));
  EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

  auto node = set1.extract(turbo::make_unique<int>(7));
  EXPECT_TRUE(node);
  EXPECT_THAT(node.value(), Pointee(7));
  EXPECT_THAT(set1, UnorderedElementsAre(Pointee(17), Pointee(19)));

  auto insert_result = set2.insert(std::move(node));
  EXPECT_FALSE(node);
  EXPECT_FALSE(insert_result.inserted);
  EXPECT_TRUE(insert_result.node);
  EXPECT_THAT(insert_result.node.value(), Pointee(7));
  EXPECT_EQ(**insert_result.position, 7);
  EXPECT_NE(insert_result.position->get(), insert_result.node.value().get());
  EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

  node = set1.extract(turbo::make_unique<int>(17));
  EXPECT_TRUE(node);
  EXPECT_THAT(node.value(), Pointee(17));
  EXPECT_THAT(set1, UnorderedElementsAre(Pointee(19)));

  node.value() = turbo::make_unique<int>(23);

  insert_result = set2.insert(std::move(node));
  EXPECT_FALSE(node);
  EXPECT_TRUE(insert_result.inserted);
  EXPECT_FALSE(insert_result.node);
  EXPECT_EQ(**insert_result.position, 23);
  EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(23)));
}

bool IsEven(int k) { return k % 2 == 0; }

TEST(FlatHashSet, EraseIf) {
  // Erase all elements.
  {
    flat_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int) { return true; }), 5);
    EXPECT_THAT(s, IsEmpty());
  }
  // Erase no elements.
  {
    flat_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int) { return false; }), 0);
    EXPECT_THAT(s, UnorderedElementsAre(1, 2, 3, 4, 5));
  }
  // Erase specific elements.
  {
    flat_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int k) { return k % 2 == 1; }), 3);
    EXPECT_THAT(s, UnorderedElementsAre(2, 4));
  }
  // Predicate is function reference.
  {
    flat_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, IsEven), 2);
    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
  }
  // Predicate is function pointer.
  {
    flat_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, &IsEven), 2);
    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
  }
}

class PoisonSoo {
  int64_t data_;

 public:
  explicit PoisonSoo(int64_t d) : data_(d) { SanitizerPoisonObject(&data_); }
  PoisonSoo(const PoisonSoo& that) : PoisonSoo(*that) {}
  ~PoisonSoo() { SanitizerUnpoisonObject(&data_); }

  int64_t operator*() const {
    SanitizerUnpoisonObject(&data_);
    const int64_t ret = data_;
    SanitizerPoisonObject(&data_);
    return ret;
  }
  template <typename H>
  friend H turbo_hash_value(H h, const PoisonSoo& pi) {
    return H::combine(std::move(h), *pi);
  }
  bool operator==(const PoisonSoo& rhs) const { return **this == *rhs; }
};

TEST(FlatHashSet, PoisonSooBasic) {
  PoisonSoo a(0), b(1);
  flat_hash_set<PoisonSoo> set;
  set.insert(a);
  EXPECT_THAT(set, UnorderedElementsAre(a));
  set.insert(b);
  EXPECT_THAT(set, UnorderedElementsAre(a, b));
  set.erase(a);
  EXPECT_THAT(set, UnorderedElementsAre(b));
  set.rehash(0);  // Shrink to SOO.
  EXPECT_THAT(set, UnorderedElementsAre(b));
}

TEST(FlatHashSet, PoisonSooMoveConstructSooToSoo) {
  PoisonSoo a(0);
  flat_hash_set<PoisonSoo> set;
  set.insert(a);
  flat_hash_set<PoisonSoo> set2(std::move(set));
  EXPECT_THAT(set2, UnorderedElementsAre(a));
}

TEST(FlatHashSet, PoisonSooAllocMoveConstructSooToSoo) {
  PoisonSoo a(0);
  flat_hash_set<PoisonSoo> set;
  set.insert(a);
  flat_hash_set<PoisonSoo> set2(std::move(set), std::allocator<PoisonSoo>());
  EXPECT_THAT(set2, UnorderedElementsAre(a));
}

TEST(FlatHashSet, PoisonSooMoveAssignFullSooToEmptySoo) {
  PoisonSoo a(0);
  flat_hash_set<PoisonSoo> set, set2;
  set.insert(a);
  set2 = std::move(set);
  EXPECT_THAT(set2, UnorderedElementsAre(a));
}

TEST(FlatHashSet, PoisonSooMoveAssignFullSooToFullSoo) {
  PoisonSoo a(0), b(1);
  flat_hash_set<PoisonSoo> set, set2;
  set.insert(a);
  set2.insert(b);
  set2 = std::move(set);
  EXPECT_THAT(set2, UnorderedElementsAre(a));
}

TEST(FlatHashSet, FlatHashSetPolicyDestroyReturnsTrue) {
  EXPECT_TRUE((decltype(FlatHashSetPolicy<int>::destroy<std::allocator<int>>(
      nullptr, nullptr))()));
  EXPECT_FALSE(
      (decltype(FlatHashSetPolicy<int>::destroy<CountingAllocator<int>>(
          nullptr, nullptr))()));
  EXPECT_FALSE((decltype(FlatHashSetPolicy<std::unique_ptr<int>>::destroy<
                         std::allocator<int>>(nullptr, nullptr))()));
}

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
