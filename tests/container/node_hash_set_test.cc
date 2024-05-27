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

#include <turbo/container/node_hash_set.h>

#include <tests/container/unordered_set_constructor_test.h>
#include <tests/container/unordered_set_lookup_test.h>
#include <tests/container/unordered_set_members_test.h>
#include <tests/container/unordered_set_modifiers_test.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {
using ::turbo::container_internal::hash_internal::Enum;
using ::turbo::container_internal::hash_internal::EnumClass;
using ::testing::IsEmpty;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

using SetTypes = ::testing::Types<
    node_hash_set<int, StatefulTestingHash, StatefulTestingEqual, Alloc<int>>,
    node_hash_set<std::string, StatefulTestingHash, StatefulTestingEqual,
                  Alloc<std::string>>,
    node_hash_set<Enum, StatefulTestingHash, StatefulTestingEqual, Alloc<Enum>>,
    node_hash_set<EnumClass, StatefulTestingHash, StatefulTestingEqual,
                  Alloc<EnumClass>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(NodeHashSet, ConstructorTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(NodeHashSet, LookupTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(NodeHashSet, MembersTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(NodeHashSet, ModifiersTest, SetTypes);

TEST(NodeHashSet, MoveableNotCopyableCompiles) {
  node_hash_set<std::unique_ptr<void*>> t;
  node_hash_set<std::unique_ptr<void*>> u;
  u = std::move(t);
}

TEST(NodeHashSet, MergeExtractInsert) {
  struct Hash {
    size_t operator()(const std::unique_ptr<int>& p) const { return *p; }
  };
  struct Eq {
    bool operator()(const std::unique_ptr<int>& a,
                    const std::unique_ptr<int>& b) const {
      return *a == *b;
    }
  };
  turbo::node_hash_set<std::unique_ptr<int>, Hash, Eq> set1, set2;
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

TEST(NodeHashSet, EraseIf) {
  // Erase all elements.
  {
    node_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int) { return true; }), 5);
    EXPECT_THAT(s, IsEmpty());
  }
  // Erase no elements.
  {
    node_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int) { return false; }), 0);
    EXPECT_THAT(s, UnorderedElementsAre(1, 2, 3, 4, 5));
  }
  // Erase specific elements.
  {
    node_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, [](int k) { return k % 2 == 1; }), 3);
    EXPECT_THAT(s, UnorderedElementsAre(2, 4));
  }
  // Predicate is function reference.
  {
    node_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, IsEven), 2);
    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
  }
  // Predicate is function pointer.
  {
    node_hash_set<int> s = {1, 2, 3, 4, 5};
    EXPECT_EQ(erase_if(s, &IsEven), 2);
    EXPECT_THAT(s, UnorderedElementsAre(1, 3, 5));
  }
}

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
