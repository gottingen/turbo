
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef THIS_HASH_SET
#define THIS_HASH_SET   node_hash_set
#define THIS_TEST_NAME  NodeHashSet
#endif

#include "flare/container/node_hash_set.h"
#include "flare/container/flat_hash_set.h"
#include "flare/container/parallel_node_hash_set.h"
#include "unordered_set_constructor_test.h"
#include "unordered_set_lookup_test.h"
#include "unordered_set_members_test.h"
#include "unordered_set_modifiers_test.h"

namespace flare {
    namespace priv {
        namespace {
            using ::flare::priv::hash_internal::Enum;
            using ::flare::priv::hash_internal::EnumClass;
            using ::testing::Pointee;
            using ::testing::UnorderedElementsAre;

            using SetTypes = ::testing::Types<
                    THIS_HASH_SET<int, StatefulTestingHash, StatefulTestingEqual, Alloc<int>>,
                    THIS_HASH_SET<std::string, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<std::string>>,
                    THIS_HASH_SET<Enum, StatefulTestingHash, StatefulTestingEqual, Alloc<Enum>>,
                    THIS_HASH_SET<EnumClass, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<EnumClass>>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ConstructorTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, LookupTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, MembersTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ModifiersTest, SetTypes);

            TEST(THIS_TEST_NAME, MoveableNotCopyableCompiles) {
                THIS_HASH_SET<std::unique_ptr<void *>> t;
                THIS_HASH_SET<std::unique_ptr<void *>> u;
                u = std::move(t);
            }

            TEST(THIS_TEST_NAME, MergeExtractInsert) {
                struct hash {
                    size_t operator()(const std::unique_ptr<int> &p) const { return *p; }
                };
                struct Eq {
                    bool operator()(const std::unique_ptr<int> &a,
                                    const std::unique_ptr<int> &b) const {
                        return *a == *b;
                    }
                };
                flare::THIS_HASH_SET<std::unique_ptr<int>, hash, Eq> set1, set2;
                set1.insert(flare::make_unique<int>(7));
                set1.insert(flare::make_unique<int>(17));

                set2.insert(flare::make_unique<int>(7));
                set2.insert(flare::make_unique<int>(19));

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(19)));

                set1.merge(set2);

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17), Pointee(19)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

                auto node = set1.extract(flare::make_unique<int>(7));
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

                node = set1.extract(flare::make_unique<int>(17));
                EXPECT_TRUE(node);
                EXPECT_THAT(node.value(), Pointee(17));
                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(19)));

                node.value() = flare::make_unique<int>(23);

                insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_TRUE(insert_result.inserted);
                EXPECT_FALSE(insert_result.node);
                EXPECT_EQ(**insert_result.position, 23);
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(23)));
            }

        }  // namespace
    }  // namespace priv
}  // namespace flare
