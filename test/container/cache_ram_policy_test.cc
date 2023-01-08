
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/container/cache/ram_policy.h"

#include "testing/gtest_wrap.h"

#include <thread>
#include <vector>

namespace testing {
    TEST(TestRamPolicy, TestOnCacheNormalSetAndDel) {
        uint32_t i = 0;
        std::function<void(void)> callback = [&]() { i++; };
        flare::ram_cache_policy<uint32_t, uint32_t> p(64, callback);
        p.on_cache_set(10, 233);
        p.on_cache_set(10, 233);
        EXPECT_EQ(i, 1);
        p.on_cache_set(10, 233);
        EXPECT_EQ(i, 2);

        std::string want_dump1 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":64,\"ram_bytes_used\":96,\"%usage\":1.5}}";
        EXPECT_EQ(p.to_string(), want_dump1);

        p.on_cache_del(10, 233);
        std::string want_dump2 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":64,\"ram_bytes_used\":64,\"%usage\":1}}";
        EXPECT_EQ(p.to_string(), want_dump2);
        p.on_cache_del(10, 233);
        p.on_cache_del(10, 233);
        std::string want_dump3 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":64,\"ram_bytes_used\":0,\"%usage\":0}}";
        EXPECT_EQ(p.to_string(), want_dump3);
    }

    TEST(TestRamPolicy, TestInitWithInvalidPram) {
        uint32_t i = 0;
        std::function<void(void)> callback = [&]() { i++; };
        flare::ram_cache_policy<uint32_t, uint32_t> p(0, callback);
        std::string want_dump1 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":33554432,\"ram_bytes_used\":0,\"%usage\":0}}";
        EXPECT_EQ(p.to_string(), want_dump1);
    }

    TEST(TestRamPolicy, TestConcurrentSetAndDel) {
        std::atomic<uint32_t> set_count = 0;
        std::function<void(void)> callback = [&set_count]() { set_count++; };
        flare::ram_cache_policy<uint32_t, uint32_t> p(256, callback);
        std::vector<std::thread> vct;
        for (uint32_t i = 0; i < 10; i++) {
            vct.push_back(std::thread([&]() { p.on_cache_set(i, i); }));
        }
        for (auto &&v : vct) {
            v.join();
        }
        std::string want_dump1 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":256,\"ram_bytes_used\":320,\"%usage\":1.25}}";
        EXPECT_EQ(p.to_string(), want_dump1);
        EXPECT_TRUE(set_count >= 3);

        std::vector<std::thread> del_vct;
        for (uint32_t i = 0; i < 10; i++) {
            del_vct.push_back(std::thread([&]() { p.on_cache_del(i, i); }));
        }
        for (auto &&v : del_vct) {
            v.join();
        }
        std::string want_dump2 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":256,\"ram_bytes_used\":0,\"%usage\":0}}";
        EXPECT_EQ(p.to_string(), want_dump2);
    }

    TEST(TestRamPolicy, TestMixConcurrentSetAndDel) {
        uint32_t i = 0;
        std::function<void(void)> callback = [&]() { i++; };
        flare::ram_cache_policy<uint32_t, uint32_t> p(256, callback);
        std::vector<std::thread> vct;
        std::vector<std::thread> del_vct;
        for (uint32_t i = 0; i < 10; i++) {
            if ((i & 1) == 0) {
                vct.push_back(std::thread([&]() { p.on_cache_set(i, i); }));
            } else {
                del_vct.push_back(std::thread([&]() { p.on_cache_del(i, i); }));
            }
        }
        for (auto &&v : vct) {
            v.join();
        }
        for (auto &&v : del_vct) {
            v.join();
        }
        std::string want_dump1 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":256,\"ram_bytes_used\":0,\"%usage\":0}}";
        EXPECT_EQ(p.to_string(), want_dump1);
        EXPECT_EQ(i, 0);

        p.on_cache_del(10, 10);
        p.on_cache_del(10, 10);
        p.on_cache_del(10, 10);
        std::string want_dump2 = "{\"ram_cache_policy\":{\"max_ram_bytes_used\":256,\"ram_bytes_used\":0,\"%usage\":0}}";
        EXPECT_EQ(p.to_string(), want_dump2);
    }

    TEST(TestRamPolicy, TestOnCacheDiyEstimator) {
        struct VectorRamUsage {
            uint64_t operator()(const std::vector<uint32_t> &t) const {
                return sizeof(t) + sizeof(uint32_t) * t.size();
            }
        };
        uint32_t i = 0;
        std::function<void(void)> callback = [&]() { i++; };
        flare::ram_cache_policy<uint32_t, std::vector<uint32_t>, flare::ram_usage<uint32_t>, VectorRamUsage> p(
                1 << 25, callback);
        p.on_cache_set(10, {233, 21});
        p.on_cache_set(10, {233, 21, 1});
        p.on_cache_set(10, {233, 21, 0, 12});
        EXPECT_EQ(i, 0);
        std::string want_dump1 =
                "{\"ram_cache_policy\":{\"max_ram_bytes_used\":33554432,\"ram_bytes_used\":204,\"%usage\":6.07967e-06}}";
        EXPECT_EQ(p.to_string(), want_dump1);
    }

}  // namespace testing