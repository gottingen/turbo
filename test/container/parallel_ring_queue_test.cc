
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "turbo/container/parallel_ring_queue.h"
#include "testing/gtest_wrap.h"
#include <thread>
#include <atomic>

namespace testing {
    TEST(TestParallelRingQueue, TestQueue) {
        turbo::parallel_ring_queue<int> queue;
        for (uint32_t i = 1; i <= 2000; ++i) {
            auto stat = queue.push_back(i);
            if (i < queue.capacity()) {
                EXPECT_EQ(true, stat);
                EXPECT_EQ(i, queue.size());
            } else {
                EXPECT_EQ(false, stat);
                EXPECT_EQ((queue.capacity() - 1), queue.size());
            }
            auto[stat1, item1] = queue.front();
            EXPECT_EQ(true, stat1);
            EXPECT_EQ(1, item1);
        }
        for (uint32_t i = 1; i <= 2000; ++i) {
            auto[stat2, item2] = queue.pop_front();
            if (i < (queue.capacity() - 1)) {
                EXPECT_EQ(true, stat2);
                EXPECT_EQ(queue.capacity() - 1 - i, queue.size());
                EXPECT_EQ(false, queue.is_empty());
            } else if (i == (queue.capacity() - 1)) {
                EXPECT_EQ(true, stat2);
                EXPECT_EQ(0, queue.size());
                EXPECT_EQ(true, queue.is_empty());
            } else {
                EXPECT_EQ(false, stat2);
                EXPECT_EQ(0, queue.size());
                EXPECT_EQ(true, queue.is_empty());
            }
        }
    }

    TEST(TestParallelRingQueue, TestCapacity) {
        turbo::parallel_ring_queue<int> queue;
        EXPECT_EQ(queue.capacity(), 1024);
        queue.reserve(100);
        EXPECT_EQ(queue.capacity(), 128);
        queue.reserve(0);
        EXPECT_EQ(queue.capacity(), 1024);
        queue.reserve(1);
        EXPECT_EQ(queue.capacity(), 2);
        uint32_t i1 = static_cast<uint32_t>(1 << 31) + 2;
        queue.reserve(i1);
        EXPECT_EQ(queue.capacity(), (1 << 31));
        uint32_t i2 = static_cast<uint32_t>(1 << 31) - 2;
        queue.reserve(i2);
        EXPECT_EQ(queue.capacity(), (1 << 31));
        queue.reserve(2);
        EXPECT_EQ(queue.capacity(), 2);
        EXPECT_TRUE(queue.push_back(1));
        EXPECT_TRUE(queue.is_full());
        EXPECT_FALSE(queue.push_back(2));
        EXPECT_TRUE(queue.is_full());
        auto[stats, pop] = queue.pop_front();
        EXPECT_TRUE(stats);
        EXPECT_TRUE(pop == 1);
        EXPECT_FALSE(queue.is_full());
        EXPECT_TRUE(queue.push_back(2));
        EXPECT_TRUE(queue.is_full());
        queue.pop_front();
        EXPECT_FALSE(queue.is_full());
    }

    TEST(TestQueue, TestConcurrent) {
        turbo::parallel_ring_queue<int> q(1000);
        std::thread threads[10];
        for (int i = 0; i < 10; i++) {
            threads[i] = std::thread(
                    [&q](int i) {
                        for (int i = 0; i < 100; i++) {
                            q.push_back(i);
                        }
                    },
                    i);
        }

        for (size_t i = 0; i < 10; i++) {
            threads[i].join();
        }
        EXPECT_EQ(q.size(), 1000);

        for (int i = 0; i < 10; i++) {
            threads[i] = std::thread(
                    [&q](int i) {
                        for (int i = 0; i < 100; i++) {
                            auto[stat, item] = q.pop_front();
                            EXPECT_TRUE(stat);
                        }
                    },
                    i);
        }

        for (size_t i = 0; i < 10; i++) {
            threads[i].join();
        }
        EXPECT_EQ(q.size(), 0);
    }

    TEST(TestQueue, TestConcurrentPushAndPop) {
        turbo::parallel_ring_queue<int> q(1000);
        std::thread threads_a[10];
        for (int i = 0; i < 10; i++) {
            threads_a[i] = std::thread(
                    [&q](int i) {
                        for (int i = 0; i < 100; i++) {
                            q.push_back(i);
                        }
                    },
                    i);
        }

        std::thread threads_b[10];
        std::atomic<size_t> pop_count{0};
        for (int i = 0; i < 10; i++) {
            threads_b[i] = std::thread(
                    [&q, &pop_count](int i) {
                        while (pop_count.load() < 1000) {
                            auto[stat, item] = q.pop_front();
                            if(stat) {
                                pop_count.fetch_add(1);
                            }
                        }
                    },
                    i);
        }

        for (size_t i = 0; i < 10; i++) {
            threads_a[i].join();
            threads_b[i].join();
        }
        EXPECT_EQ(q.size(), 0);
    }

    TEST(TestQueue, TestNoConcurrent) {
        turbo::parallel_ring_queue<int> q(64);
        for (size_t i = 0; i < 30; i++) {
            q.push_back(i);
        }

        for (size_t i = 0; i < 10; i++) {
            q.pop_front();
        }

        for (size_t i = 30; i < 60; i++) {
            q.push_back(i);
        }

        EXPECT_EQ(q.size(), 50);

        for (size_t i = 0; i < 55; i++) {
            auto[stat, item] = q.pop_front();
            if (i < 50) {
                EXPECT_TRUE(stat);
                EXPECT_EQ(item, i + 10);
            } else {
                EXPECT_FALSE(stat);
            }
        }

        EXPECT_TRUE(q.is_empty());
    }

    TEST(TestQueue, TestFront) {
        turbo::parallel_ring_queue<int> q(10);
        for (int i = 0; i < 15; i++) {
            q.push_back(i);
        }
        EXPECT_TRUE(q.is_full());
        for (int i = 0; i < 20; i++) {
            if (i < 15) {
                auto[front_stat, front] = q.front();
                auto[stat, item] = q.pop_front();
                EXPECT_TRUE(front_stat);
                EXPECT_TRUE(stat);
                EXPECT_EQ(item, i);
                EXPECT_EQ(item, front);
            } else {
                auto[front_stat, front] = q.front();
                auto[stat, item] = q.pop_front();
                EXPECT_FALSE(front_stat);
                EXPECT_FALSE(stat);
                EXPECT_EQ(item, 0);
                EXPECT_EQ(0, front);
            }
        }
        EXPECT_TRUE(q.is_empty());
    }

    TEST(TestQueue, TestFixCapacity) {
        {
            turbo::parallel_ring_queue<int> q(10);
            EXPECT_EQ(q.fix_capacity(10), 16);
        }
        {
            turbo::parallel_ring_queue<int> q(200);
            EXPECT_EQ(q.fix_capacity(200), 256);
        }
        {
            turbo::parallel_ring_queue<int> q(0);
            EXPECT_EQ(q.fix_capacity(0), 1024);
        }
        {
            turbo::parallel_ring_queue<int> q(static_cast<uint32_t>(1 << 31) + 2);
            EXPECT_EQ(q.fix_capacity(static_cast<uint32_t>(1 << 31) + 2), static_cast<uint32_t>(1 << 31));
        }
        {
            turbo::parallel_ring_queue<int> q(0);
            EXPECT_EQ(q.fix_capacity(0), 1024);
        }
    }

    TEST(TestQueue, TestPowerOfTwoForSize) {
        {
            turbo::parallel_ring_queue<int> q(10);
            EXPECT_EQ(q.power_of_two_for_size(10), 16);
        }
        {
            turbo::parallel_ring_queue<int> q(0);
            EXPECT_EQ(q.power_of_two_for_size(0), 0);
        }
        {
            turbo::parallel_ring_queue<int> q(1);
            EXPECT_EQ(q.power_of_two_for_size(1), 1);
        }
    }

    TEST(TestQueue, TestHighestOneBit) {
        {
            turbo::parallel_ring_queue<int> q(10);
            EXPECT_EQ(q.highest_one_bit(10), 8);
        }
        {
            turbo::parallel_ring_queue<int> q(0);
            EXPECT_EQ(q.highest_one_bit(0), 0);
        }
        {
            turbo::parallel_ring_queue<int> q(17);
            EXPECT_EQ(q.highest_one_bit(17), 16);
        }
    }
}  // namespace testing
