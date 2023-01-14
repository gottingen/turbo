
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/sstream_workaround.h"
#include "turbo/thread/thread.h"

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atomic>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "testing/gtest_wrap.h"
#include "turbo/thread/latch.h"

#ifdef TURBO_PLATFORM_OSX
TEST(thread, index) {
    EXPECT_EQ(turbo::thread::thread_index(), 1);
    turbo::thread th([]{
        EXPECT_EQ(turbo::thread::thread_index(), 2);
        EXPECT_EQ(turbo::thread::current_name(), "#2");
    });
    th.start();
    EXPECT_EQ(th.name(), "#2");
    th.join();

    turbo::thread th1([]{
        EXPECT_EQ(turbo::thread::thread_index(), 3);
        EXPECT_EQ(turbo::thread::current_name(), "th#3");
    });
    th1.set_prefix("th");
    th1.start();
    EXPECT_EQ(th1.name(), "th#3");
    th1.join();
}
#endif // TURBO_PLATFORM_OSX

#ifdef TURBO_PLATFORM_LINUX
TEST(thread, index) {
    EXPECT_EQ(turbo::thread::thread_index(), 0);
    turbo::thread th([]{
        EXPECT_EQ(turbo::thread::thread_index(), 1);
        EXPECT_EQ(turbo::thread::current_name(), "#1");
    });
    th.start();
    EXPECT_EQ(th.name(), "#1");
    th.join();

    turbo::thread th1([]{
        EXPECT_EQ(turbo::thread::thread_index(), 2);
        EXPECT_EQ(turbo::thread::current_name(), "th#2");
    });
    th1.set_prefix("th");
    th1.start();
    EXPECT_EQ(th1.name(), "th#2");
    th1.join();
}
#endif  // #ifdef TURBO_PLATFORM_LINUX

int c = 0;

void sum(int a, int b) {
     c = a+b;
}

TEST(thread, varidic_fun) {
    turbo::thread th(sum, 2, 3);
    th.start();
    th.join();
    EXPECT_EQ(c, 5);
}
