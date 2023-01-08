
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/thread/thread_local.h"

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
#include "flare/thread/latch.h"

namespace flare {

    struct Widget {
        static int total;
        int val_;

        ~Widget() { total += val_; }
    };

    int Widget::total = 0;

    TEST(thread_local_store, BasicDestructor) {
        Widget::total = 0;
        thread_local_store<Widget> w;
        std::thread([&w]() { w->val_ += 10; }).join();
        EXPECT_EQ(10, Widget::total);
    }

    TEST(thread_local_store, SimpleRepeatDestructor) {
        Widget::total = 0;
        {
            thread_local_store<Widget> w;
            w->val_ += 10;
        }
        {
            thread_local_store<Widget> w;
            w->val_ += 10;
        }
        EXPECT_EQ(20, Widget::total);
    }

    TEST(thread_local_store, InterleavedDestructors) {
        Widget::total = 0;
        std::unique_ptr<thread_local_store<Widget>> w;
        int version = 0;
        const int version_max = 2;
        int th_iter = 0;
        std::mutex lock;
        auto th = std::thread([&]() {
            int version_prev = 0;
            while (true) {
                while (true) {
                    std::lock_guard<std::mutex> g(lock);
                    if (version > version_max) {
                        return;
                    }
                    if (version > version_prev) {
                        // We have a new version of w, so it should be initialized to zero
                        EXPECT_EQ(0, (*w)->val_);
                        break;
                    }
                }
                std::lock_guard<std::mutex> g(lock);
                version_prev = version;
                (*w)->val_ += 10;
                ++th_iter;
            }
        });
        for (size_t i = 0; i < version_max; ++i) {
            int th_iter_prev = 0;
            {
                std::lock_guard<std::mutex> g(lock);
                th_iter_prev = th_iter;
                w = std::make_unique<thread_local_store<Widget>>();
                ++version;
            }
            while (true) {
                std::lock_guard<std::mutex> g(lock);
                if (th_iter > th_iter_prev) {
                    break;
                }
            }
        }
        {
            std::lock_guard<std::mutex> g(lock);
            version = version_max + 1;
        }
        th.join();
        EXPECT_EQ(version_max * 10, Widget::total);
    }

    class SimpleThreadCachedInt {
        thread_local_store<int> val_;

    public:
        void add(int val) { *val_ += val; }

        int read() {
            int ret = 0;
            val_.for_each([&](const int *p) { ret += *p; });
            return ret;
        }
    };

    TEST(thread_local_store, AccessAllThreadsCounter) {
        const int kNumThreads = 256;
        SimpleThreadCachedInt stci[kNumThreads + 1];
        std::atomic<bool> run(true);
        std::atomic<int> total_atomic{0};
        std::vector<std::thread> threads;
        // thread i will increment all the thread locals
        // in the range 0..i
        for (int i = 0; i < kNumThreads; ++i) {
            threads.push_back(std::thread([i, &stci, &run, &total_atomic]() {
                for (int j = 0; j <= i; ++j) {
                    stci[j].add(1);
                }

                total_atomic.fetch_add(1);
                while (run.load()) {
                    usleep(100);
                }
            }));
        }
        while (total_atomic.load() != kNumThreads) {
            usleep(100);
        }
        for (int i = 0; i <= kNumThreads; ++i) {
            EXPECT_EQ(kNumThreads - i, stci[i].read());
        }
        run.store(false);
        for (auto &t : threads) {
            t.join();
        }
    }

    TEST(thread_local_store, resetNull) {
        thread_local_store<int> tl;
        tl.reset(std::make_unique<int>(4));
        EXPECT_EQ(4, *tl.get());
        tl.reset();
        EXPECT_EQ(0, *tl.get());
        tl.reset(std::make_unique<int>(5));
        EXPECT_EQ(5, *tl.get());
    }

    struct Foo {
        thread_local_store<int> tl;
    };

    TEST(thread_local_store, Movable1) {
        Foo a;
        Foo b;
        EXPECT_TRUE(a.tl.get() != b.tl.get());
    }

    TEST(thread_local_store, Movable2) {
        std::map<int, Foo> map;

        map[42];
        map[10];
        map[23];
        map[100];

        std::set<void *> tls;
        for (auto &m : map) {
            tls.insert(m.second.tl.get());
        }

        // Make sure that we have 4 different instances of *tl
        EXPECT_EQ(4ul, tls.size());
    }

    using TLPInt = thread_local_store<std::atomic<int>>;

    template<typename Op, typename Check>
    void StressAccessTest(Op op, Check check, size_t num_threads,
                          size_t num_loops) {
        TLPInt ptr;
        ptr.reset(std::make_unique<std::atomic<int>>(0));
        std::atomic<bool> running{true};

        latch l(num_threads + 1);

        std::vector<std::thread> threads;

        for (size_t k = 0; k < num_threads; ++k) {
            threads.emplace_back([&] {
                ptr.reset(std::make_unique<std::atomic<int>>(1));

                l.count_down();
                l.wait();

                while (running.load()) {
                    op(ptr);
                }
            });
        }

        // wait for the threads to be up and running
        l.count_down();
        l.wait();

        for (size_t n = 0; n < num_loops; ++n) {
            int sum = 0;
            ptr.for_each([&](const std::atomic<int> *p) { sum += *p; });
            check(sum, num_threads);
        }

        running.store(false);
        for (auto &t : threads) {
            t.join();
        }
    }

    TEST(thread_local_store, StressAccessReset) {
        StressAccessTest(
                [](TLPInt &ptr) { ptr.reset(std::make_unique<std::atomic<int>>(1)); },
                [](size_t sum, size_t num_threads) { EXPECT_EQ(sum, num_threads); }, 16,
                10);
    }

    TEST(thread_local_store, StressAccessSet) {
        StressAccessTest(
                [](TLPInt &ptr) { *ptr = 1; },
                [](size_t sum, size_t num_threads) { EXPECT_EQ(sum, num_threads); }, 16,
                100);
    }

    TEST(thread_local_store, StressAccessRelease) {
        StressAccessTest(
                [](TLPInt &ptr) {
                    auto *p = ptr.leak();
                    delete p;
                    ptr.reset(std::make_unique<std::atomic<int>>(1));
                },
                [](size_t sum, size_t num_threads) { EXPECT_LE(sum, num_threads); }, 8,
                4);
    }

}  // namespace flare
