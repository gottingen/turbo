
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <vector>

#include "testing/gtest_wrap.h"

#include "turbo/container/flat_hash_map_dump.h"
#include "turbo/container/flat_hash_set.h"
#include "turbo/container/parallel_flat_hash_map.h"

namespace turbo {
    namespace priv {
        namespace {

            TEST(DumpLoad, FlatHashSet_uin32) {
                turbo::flat_hash_set<uint32_t> st1 = {1991, 1202};

                {
                    turbo::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(st1.turbo_map_dump(ar_out));
                }

                turbo::flat_hash_set<uint32_t> st2;
                {
                    turbo::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(st2.turbo_map_load(ar_in));
                }
                EXPECT_TRUE(st1 == st2);
            }

            TEST(DumpLoad, FlatHashMap_uint64_uint32) {
                turbo::flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {78731, 99},
                        {13141, 299},
                        {2651,  101}};

                {
                    turbo::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.turbo_map_dump(ar_out));
                }

                turbo::flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    turbo::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.turbo_map_load(ar_in));
                }

                EXPECT_TRUE(mp1 == mp2);
            }

            TEST(DumpLoad, ParallelFlatHashMap_uint64_uint32) {
                turbo::parallel_flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {99,  299},
                        {992, 2991},
                        {299, 1299}};

                {
                    turbo::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.turbo_map_dump(ar_out));
                }

                turbo::parallel_flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    turbo::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.turbo_map_load(ar_in));
                }
                EXPECT_TRUE(mp1 == mp2);
            }

        }
    }
}

