

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/container/lru_cache.h"
#include "testing/gtest_wrap.h"

#include <iostream>

namespace testing {
    TEST(TestCache, TestSet) {
        flare::cache_config config;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        {
            auto item = cache.set(10, 20);
            EXPECT_EQ(10, item->key());
            EXPECT_EQ(20, item->value());
            EXPECT_EQ(20, cache.get(10)->value());

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            EXPECT_EQ(1, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            cache.set(10, 30);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            EXPECT_EQ(1, cache.size());
            EXPECT_EQ(30, cache.get(10)->value());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            cache.set(20, 40);
            EXPECT_EQ(40, cache.get(20)->value());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            EXPECT_EQ(2, cache.size());
            EXPECT_EQ(40, cache.get(20)->value());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            auto item_ptr1 = cache.get_or_set(
                    20, 10, [](int i) -> int { return i; }, 10);
            EXPECT_EQ(40, item_ptr1->value());
            auto item_ptr2 = cache.get_or_set(
                    20000, 10, [](int i) -> int { return i; }, 10);
            EXPECT_EQ(10, item_ptr2->value());
        }
        {
            auto item_ptr1 = cache.set(111, 111, 0.01);
            EXPECT_TRUE(item_ptr1 == nullptr);
            auto item_ptr2 = cache.set(111, 111, 1.0);
            EXPECT_EQ(111, item_ptr2->value());
        }
    }

    TEST(TestCache, TestTwiceSet) {
        flare::cache_config config;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        cache.set(10, 20);
        cache.set(10, 30);
        EXPECT_EQ(30, cache.get(10)->value());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_EQ(1, cache.size());
        EXPECT_EQ(cache.size(), cache.item_num_in_bucket());
    }

    TEST(TestCache, TestDel) {
        flare::cache_config config;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        {
            EXPECT_FALSE(cache.del(10));
        }
        {
            cache.set(10, 20);
            cache.set(11, 21);
            cache.set(12, 22);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(3, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            EXPECT_FALSE(cache.del(20));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(3, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            EXPECT_TRUE(cache.del(10));
            EXPECT_EQ(nullptr, cache.get(10));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(2, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            EXPECT_TRUE(cache.del(11));
            EXPECT_EQ(nullptr, cache.get(11));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(1, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            EXPECT_TRUE(cache.del(12));
            EXPECT_EQ(nullptr, cache.get(12));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(0, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
        {
            EXPECT_FALSE(cache.del(10));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            EXPECT_EQ(0, cache.size());
            EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        }
    }

    TEST(TestCache, TestGC) {
        flare::cache_config config;
        config.max_item_num_ = 10;
        config.prune_batch_size_ = 3;
        config.promote_per_times_ = 3;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        for (int i = 0; i < 10; i++) {
            cache.set(i, i + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(10, cache.size());
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        cache.set(10, 11);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(8, cache.size());
        EXPECT_EQ(nullptr, cache.get(0));
        EXPECT_EQ(nullptr, cache.get(1));
        EXPECT_EQ(nullptr, cache.get(2));
        for (int i = 3; i <= 10; i++) {
            auto item_ptr = cache.get(i);
            ASSERT_TRUE(item_ptr != nullptr);
            EXPECT_EQ(i + 1, item_ptr->value());
        }
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
    }

    TEST(TestCache, TestPromoteGC1) {
        flare::cache_config config;
        config.max_item_num_ = 10;
        config.prune_batch_size_ = 3;
        config.promote_per_times_ = 3;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        for (int i = 0; i < 10; i++) {
            cache.set(i, i + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(10, cache.size());
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());

        cache.get(0);

        cache.set(10, 11);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(8, cache.size());
        EXPECT_EQ(nullptr, cache.get(0));
        EXPECT_EQ(nullptr, cache.get(1));
        EXPECT_EQ(nullptr, cache.get(2));
        for (int i = 3; i <= 10; i++) {
            auto item_ptr = cache.get(i);
            ASSERT_TRUE(item_ptr != nullptr);
            EXPECT_EQ(i + 1, item_ptr->value());
        }
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
    }

    TEST(TestCache, TestPromoteGC2) {
        flare::cache_config config;
        config.max_item_num_ = 10;
        config.prune_batch_size_ = 3;
        config.promote_per_times_ = 3;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        for (int i = 0; i < 10; i++) {
            cache.set(i, i + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(10, cache.size());
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());

        cache.get(0);
        cache.get(0);
        cache.get(0);

        cache.set(10, 11);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(8, cache.size());
        EXPECT_EQ(1, cache.get(0)->value());
        EXPECT_EQ(nullptr, cache.get(1));
        EXPECT_EQ(nullptr, cache.get(2));
        EXPECT_EQ(nullptr, cache.get(3));
        for (int i = 4; i <= 10; i++) {
            auto item_ptr = cache.get(i);
            ASSERT_TRUE(item_ptr != nullptr);
            EXPECT_EQ(i + 1, item_ptr->value());
        }
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
    }

    TEST(TestCache, TestPromoteTableFull) {
        flare::cache_config config;
        config.max_item_num_ = 5000;
        config.prune_batch_size_ = 100;
        config.promote_per_times_ = 3;
        config.item_expire_sec_ = flare::cache_config::kDefaultCacheItemExpireSec;
        config.item_gen_time_threshold_ms_ = flare::cache_config::kDefaultGenItemTimeThresholdMs;
        config.delete_buffer_len_ = 10;
        config.promote_buffer_len_ = 1;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        for (int i = 0; i < 1000; i++) {
            cache.set(i, i + 1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_TRUE(cache.size() == cache.item_num_in_bucket());
        EXPECT_TRUE(cache.size() < 1000);
    }

    TEST(TestCache, TestTwoThreadSetDel) {
        flare::cache_config config;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        auto threadFunc = [&cache]() {
            for (int i = 0; i < 1000; i++) {
                for (int j = 0; j < 5; j++) {
                    cache.set(j, j + 1);
                }
                for (int j = 4; j >= 0; j--) {
                    cache.del(j);
                }
            }
        };
        auto thread1 = std::thread(threadFunc);
        auto thread2 = std::thread(threadFunc);
        thread1.join();
        thread2.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(0, cache.item_num_in_bucket());
        for (int j = 0; j < 5; j++) {
            EXPECT_EQ(nullptr, cache.get(j));
        }
        for (int i = 10; i < 111; i++) {
            cache.set(i, i + 1);
        }
        for (int i = 50; i < 111; i++) {
            auto item_ptr = cache.get(i);
            ASSERT_TRUE(item_ptr != nullptr);
            EXPECT_EQ(i + 1, item_ptr->value());
        }
    }

    TEST(TestCache, TestTwoThreadDelSet) {
        flare::cache_config config;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<int, int> cache(config);
        cache.start();
        auto threadFunc = [&cache]() {
            for (int j = 4; j >= 0; j--) {
                cache.del(j);
            }
            for (int j = 0; j < 5; j++) {
                cache.set(j, j + 1);
            }
        };
        auto thread1 = std::thread(threadFunc);
        auto thread2 = std::thread(threadFunc);
        thread1.join();
        thread2.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(5, cache.item_num_in_bucket());
        for (int j = 0; j < 5; j++) {
            auto item_ptr = cache.get(j);
            ASSERT_TRUE(item_ptr != nullptr);
            EXPECT_EQ(j + 1, item_ptr->value());
        }
    }

    TEST(TestCache, TestDump) {
        flare::lru_cache<int, int> cache;
        cache.start();
        for (int i = 0; i < 10; i++) {
            cache.set(i, i);
        }
        cache.get(2);
        cache.get(1);
        cache.get(0);
        cache.get(110);
        cache.get(1120);

        std::string want_dump =
                "{\"cache\":{\"policy\":{\"empty_cache_policy\":{}},\"statistic\":{\"cache_stats\":{\"cache_hit_count\":3,"
                "\"cache_miss_count\":2}}}}";
        EXPECT_EQ(cache.dump(), want_dump);
    }

    TEST(TestCache, TestWithRamPolicyDump) {
        flare::lru_cache<uint32_t, uint32_t> cache;
        cache.use_ram_policy();
        cache.start();
        for (uint32_t i = 0; i < 10; i++) {
            cache.set(i, i);
        }
        cache.get(2);
        cache.get(1);
        cache.get(0);
        cache.get(110);
        cache.get(1120);
        std::string want_dump =
                "{\"cache\":{\"policy\":{\"ram_cache_policy\":{\"max_ram_bytes_used\":33554432,\"ram_bytes_used\":320,\"%usage\":"
                "9.53674e-06}},\"statistic\":{\"cache_stats\":{\"cache_hit_count\":3,\"cache_miss_count\":2}}}}";
        EXPECT_EQ(cache.dump(), want_dump);
    }

    TEST(TestCache, TestWithRamPolicyGc) {
        flare::cache_config config;
        config.max_item_num_ = 10;
        config.prune_batch_size_ = 3;
        config.promote_per_times_ = 3;
        config.worker_sleep_ms_ = 0;
        flare::lru_cache<uint32_t, uint32_t> cache(config);
        cache.use_ram_policy(256);
        cache.start();
        for (uint32_t i = 0; i < 7; i++) {
            cache.set(i, i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        for (uint32_t i = 7; i < 10; i++) {
            cache.set(i, i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_TRUE(cache.item_num_in_bucket() < 10);
    }
}  // namespace testing
