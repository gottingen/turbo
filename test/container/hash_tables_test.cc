
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/container/hash_tables.h"
#include "testing/gtest_wrap.h"

namespace {

    class HashPairTest : public testing::Test {
    };

#define INSERT_PAIR_TEST(Type, value1, value2) \
  { \
    Type pair(value1, value2); \
    flare::container::hash_map<Type, int> map; \
    map[pair] = 1; \
  }

// Verify that a hash_map can be constructed for pairs of integers of various
// sizes.
    TEST_F(HashPairTest, IntegerPairs) {
        typedef std::pair<int16_t, int16_t> Int16Int16Pair;
        typedef std::pair<int16_t, int32_t> Int16Int32Pair;
        typedef std::pair<int16_t, int64_t> Int16Int64Pair;

        INSERT_PAIR_TEST(Int16Int16Pair, 4, 6);
        INSERT_PAIR_TEST(Int16Int32Pair, 9, (1 << 29) + 378128932);
        INSERT_PAIR_TEST(Int16Int64Pair, 10,
                         (INT64_C(1) << 60) + INT64_C(78931732321));

        typedef std::pair<int32_t, int16_t> Int32Int16Pair;
        typedef std::pair<int32_t, int32_t> Int32Int32Pair;
        typedef std::pair<int32_t, int64_t> Int32Int64Pair;

        INSERT_PAIR_TEST(Int32Int16Pair, 4, 6);
        INSERT_PAIR_TEST(Int32Int32Pair, 9, (1 << 29) + 378128932);
        INSERT_PAIR_TEST(Int32Int64Pair, 10,
                         (INT64_C(1) << 60) + INT64_C(78931732321));

        typedef std::pair<int64_t, int16_t> Int64Int16Pair;
        typedef std::pair<int64_t, int32_t> Int64Int32Pair;
        typedef std::pair<int64_t, int64_t> Int64Int64Pair;

        INSERT_PAIR_TEST(Int64Int16Pair, 4, 6);
        INSERT_PAIR_TEST(Int64Int32Pair, 9, (1 << 29) + 378128932);
        INSERT_PAIR_TEST(Int64Int64Pair, 10,
                         (INT64_C(1) << 60) + INT64_C(78931732321));
    }

}  // namespace
