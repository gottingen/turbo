
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "testing/sstream_workaround.h"
#include "turbo/thread/spinlock.h"
#include <thread>
#include "turbo/thread/latch.h"
#include "testing/gtest_wrap.h"

namespace turbo {


    std::uint64_t counter{};

    TEST(Spinlock, All) {
        constexpr auto T = 100;
        constexpr auto N = 100000;
        std::thread ts[100];
        latch latch(1);
        spinlock splk;

        for (auto &&t : ts) {
            t = std::thread([&] {
                latch.wait();
                for (int i = 0; i != N; ++i) {
                    std::scoped_lock lk(splk);
                    ++counter;
                }
            });
        }
        latch.count_down();
        for (auto &&t : ts) {
            t.join();
        }
        ASSERT_EQ(T * N, counter);
    }
}  // namespace turbo