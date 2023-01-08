
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/container/cache/item.h"
#include "testing/gtest_wrap.h"
#include <thread>

namespace testing {
    TEST(TestItem, TestItem) {
        auto duration = std::chrono::milliseconds(30);
        auto expires = std::chrono::steady_clock::now() + duration;
        flare::cache_item<int, int> item(10, 20, expires);
        EXPECT_EQ(10, item.key());
        EXPECT_EQ(20, item.value());
        EXPECT_FALSE(item.expired());

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        EXPECT_TRUE(item.expired());

        EXPECT_FALSE(item.is_delete());
        item.incr_promote_times();
        EXPECT_FALSE(item.should_promote(2));
        item.incr_promote_times();
        EXPECT_TRUE(item.should_promote(2));
        item.reset_status();
        EXPECT_FALSE(item.should_promote(2));
        item.set_deleted();
        EXPECT_TRUE(item.is_delete());
        item.incr_promote_times();
        item.incr_promote_times();
        EXPECT_FALSE(item.should_promote(2));
        item.reset_status();
        EXPECT_FALSE(item.is_delete());
    }
}  // namespace testing
