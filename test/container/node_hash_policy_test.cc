
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <memory>
#include "flare/container/flat_hash_map.h"


#include "testing/gtest_wrap.h"

namespace flare {
    namespace priv {
        namespace {

            using ::testing::Pointee;

            struct Policy : node_hash_policy<int &, Policy> {
                using key_type = int;
                using init_type = int;

                template<class Alloc>
                static int *new_element(Alloc *, int value) {
                    return new int(value);
                }

                template<class Alloc>
                static void delete_element(Alloc *, int *elem) {
                    delete elem;
                }
            };

            using NodePolicy = hash_policy_traits<Policy>;

            struct NodeTest : ::testing::Test {
                std::allocator<int> alloc;
                int n = 53;
                int *a = &n;
            };

            TEST_F(NodeTest, ConstructDestroy) {
                NodePolicy::construct(&alloc, &a, 42);
                EXPECT_THAT(a, Pointee(42));
                NodePolicy::destroy(&alloc, &a);
            }

            TEST_F(NodeTest, transfer) {
                int s = 42;
                int *b = &s;
                NodePolicy::transfer(&alloc, &a, &b);
                EXPECT_EQ(&s, a);
            }

        }  // namespace
    }  // namespace priv
}  // namespace flare
