
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/thread/thread_cache.h"

#include <string>
#include <thread>
#include <vector>

#include "testing/gtest_wrap.h"
#include "flare/strings/numbers.h"
#include "flare/times/time.h"
#include "flare/thread/latch.h"
#include "flare/base/fast_rand.h"

using namespace std::literals;

namespace flare {

    TEST(thread_cache, Basic) {
        thread_cache<std::string> tc_str("123");
        for (int i = 0; i != 1000; ++i) {
            latch latch1(1), latch2(1);
            auto t = std::thread([&] {
                ASSERT_EQ("123", tc_str.non_idempotent_get());
                latch1.count_down();
                latch2.wait();
                ASSERT_EQ("456", tc_str.non_idempotent_get());
            });
            latch1.wait();
            tc_str.emplace("456");
            latch2.count_down();
            t.join();
            tc_str.emplace("123");
        }

        // Were `thread_local` keyword used internally, the assertion below would
        // fail.
        thread_cache<std::string> tc_str2("777");
        std::thread([&] { ASSERT_EQ("777", tc_str2.non_idempotent_get()); }).join();
    }

    TEST(thread_cache, Torture) {
        thread_cache<std::string> str;
        std::vector<std::thread> ts;

        for (int i = 0; i != 100; ++i) {
            ts.push_back(std::thread([&, s = flare::time_now()] {
                while (flare::time_now() + flare::duration::seconds(10) < s) {
                    if (flare::base::fast_rand() % 1000 == 0) {
                        str.emplace(std::to_string(flare::base::fast_rand() % 33333));
                    } else {
                        int64_t opt;
                        auto r = flare::simple_atoi(str.non_idempotent_get(), &opt);
                        ASSERT_TRUE(r);
                        ASSERT_LT(opt, 33333);
                    }
                }
            }));
        }
        for (auto &&e : ts) {
            e.join();
        }
    }

}  // namespace flare
