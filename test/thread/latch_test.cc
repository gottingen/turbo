
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include "flare/thread/latch.h"
#include "testing/gtest_wrap.h"
#include "flare/times/time.h"

namespace flare {

    std::atomic<bool> exiting{false};

    void RunTest() {
        std::size_t local_count = 0, remote_count = 0;
        while (!exiting) {
            auto called = std::make_shared<std::atomic<bool>>(false);
            std::this_thread::yield();  // Wait for thread pool to start.
            latch l(1);
            auto t = std::thread([&] {
                if (!called->exchange(true)) {
                    std::this_thread::yield();  // Something costly.
                    l.count_down();
                    ++remote_count;
                }
            });
            std::this_thread::yield();  // Something costly.
            if (!called->exchange(true)) {
                l.count_down();
                ++local_count;
            }
            l.wait();
            t.join();
        }
        std::cout << local_count << " " << remote_count << std::endl;
    }

    TEST(Latch, Torture) {
        std::thread ts[10];
        for (auto &&t : ts) {
            t = std::thread(RunTest);
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
        exiting = true;
        for (auto &&t : ts) {
            t.join();
        }
    }

    TEST(Latch, CountDownTwo) {
        latch l(2);
        l.arrive_and_wait(2);
        ASSERT_TRUE(1);
    }

    TEST(Latch, WaitFor) {
        latch l(1);
        ASSERT_FALSE(l.wait_for(flare::duration::milliseconds(100)));
        l.count_down();
        ASSERT_TRUE(l.wait_for(flare::zero_duration()));
    }

    TEST(Latch, WaitUntil) {
        latch l(1);
        ASSERT_FALSE(l.wait_until(flare::time_point::future_unix_millis(100)));
        l.count_down();
        ASSERT_TRUE(l.wait_until(flare::time_now()));
    }
}