
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <vector>

#include "testing/gtest_wrap.h"

#include "flare/container/flat_hash_map_dump.h"
#include "flare/container/flat_hash_set.h"
#include "flare/container/parallel_flat_hash_map.h"

namespace flare {
    namespace priv {
        namespace {

            TEST(DumpLoad, FlatHashSet_uin32) {
                flare::flat_hash_set<uint32_t> st1 = {1991, 1202};

                {
                    flare::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(st1.flare_map_dump(ar_out));
                }

                flare::flat_hash_set<uint32_t> st2;
                {
                    flare::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(st2.flare_map_load(ar_in));
                }
                EXPECT_TRUE(st1 == st2);
            }

            TEST(DumpLoad, FlatHashMap_uint64_uint32) {
                flare::flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {78731, 99},
                        {13141, 299},
                        {2651,  101}};

                {
                    flare::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.flare_map_dump(ar_out));
                }

                flare::flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    flare::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.flare_map_load(ar_in));
                }

                EXPECT_TRUE(mp1 == mp2);
            }

            TEST(DumpLoad, ParallelFlatHashMap_uint64_uint32) {
                flare::parallel_flat_hash_map<uint64_t, uint32_t> mp1 = {
                        {99,  299},
                        {992, 2991},
                        {299, 1299}};

                {
                    flare::BinaryOutputArchive ar_out("./dump.data");
                    EXPECT_TRUE(mp1.flare_map_dump(ar_out));
                }

                flare::parallel_flat_hash_map<uint64_t, uint32_t> mp2;
                {
                    flare::BinaryInputArchive ar_in("./dump.data");
                    EXPECT_TRUE(mp2.flare_map_load(ar_in));
                }
                EXPECT_TRUE(mp1 == mp2);
            }

        }
    }
}

