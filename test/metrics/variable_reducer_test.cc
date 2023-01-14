
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/sstream_workaround.h"
#include "testing/gtest_wrap.h"
#include <limits>                           //std::numeric_limits

#include "turbo/metrics/variable_reducer.h"
#include "turbo/metrics/gauge.h"
#include "turbo/times/time.h"
#include "turbo/strings/str_format.h"
#include "turbo/strings/string_splitter.h"
#include "turbo/container/hash_tables.h"

namespace {
    class ReducerTest : public testing::Test {
    protected:

        void SetUp() {}

        void TearDown() {}
    };

    TEST_F(ReducerTest, atomicity) {
        ASSERT_EQ(sizeof(int32_t), sizeof(turbo::metrics_detail::ElementContainer<int32_t>));
        ASSERT_EQ(sizeof(int64_t), sizeof(turbo::metrics_detail::ElementContainer<int64_t>));
        ASSERT_EQ(sizeof(float), sizeof(turbo::metrics_detail::ElementContainer<float>));
        ASSERT_EQ(sizeof(double), sizeof(turbo::metrics_detail::ElementContainer<double>));
    }

    TEST_F(ReducerTest, adder) {
        turbo::gauge<uint32_t> reducer1;
        ASSERT_TRUE(reducer1.valid());
        reducer1 << 2 << 4;
        ASSERT_EQ(6ul, reducer1.get_value());

        turbo::gauge<double> reducer2;
        ASSERT_TRUE(reducer2.valid());
        reducer2 << 2.0 << 4.0;
        ASSERT_DOUBLE_EQ(6.0, reducer2.get_value());

        turbo::gauge<int> reducer3;
        ASSERT_TRUE(reducer3.valid());
        reducer3 << -9 << 1 << 0 << 3;
        ASSERT_EQ(-5, reducer3.get_value());
    }

    const size_t OPS_PER_THREAD = 500000;

    static void *thread_counter(void *arg) {
        turbo::gauge<uint64_t> *reducer = (turbo::gauge<uint64_t> *) arg;
        turbo::stop_watcher timer;
        timer.start();
        for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
            (*reducer) << 2;
        }
        timer.stop();
        return (void *) (timer.n_elapsed());
    }

    void *add_atomic(void *arg) {
        std::atomic<uint64_t> *counter = (std::atomic<uint64_t> *) arg;
        turbo::stop_watcher timer;
        timer.start();
        for (size_t i = 0; i < OPS_PER_THREAD / 100; ++i) {
            counter->fetch_add(2, std::memory_order_relaxed);
        }
        timer.stop();
        return (void *) (timer.n_elapsed());
    }

    static long start_perf_test_with_atomic(size_t num_thread) {
        std::atomic<uint64_t> counter(0);
        pthread_t threads[num_thread];
        for (size_t i = 0; i < num_thread; ++i) {
            pthread_create(&threads[i], nullptr, &add_atomic, (void *) &counter);
        }
        long totol_time = 0;
        for (size_t i = 0; i < num_thread; ++i) {
            void *ret;
            pthread_join(threads[i], &ret);
            totol_time += (long) ret;
        }
        long avg_time = totol_time / (OPS_PER_THREAD / 100 * num_thread);
        EXPECT_EQ(2ul * num_thread * OPS_PER_THREAD / 100, counter.load());
        return avg_time;
    }

    static long start_perf_test_with_adder(size_t num_thread) {
        turbo::gauge<uint64_t> reducer;
        EXPECT_TRUE(reducer.valid());
        pthread_t threads[num_thread];
        for (size_t i = 0; i < num_thread; ++i) {
            pthread_create(&threads[i], nullptr, &thread_counter, (void *) &reducer);
        }
        long totol_time = 0;
        for (size_t i = 0; i < num_thread; ++i) {
            void *ret = nullptr;
            pthread_join(threads[i], &ret);
            totol_time += (long) ret;
        }
        long avg_time = totol_time / (OPS_PER_THREAD * num_thread);
        EXPECT_EQ(2ul * num_thread * OPS_PER_THREAD, reducer.get_value());
        return avg_time;
    }

    TEST_F(ReducerTest, perf) {
        std::ostringstream oss;
        for (size_t i = 1; i <= 24; ++i) {
            oss << i << '\t' << start_perf_test_with_adder(i) << '\n';
        }
        TURBO_LOG(INFO) << "Adder performance:\n" << oss.str();
        oss.str("");
        for (size_t i = 1; i <= 24; ++i) {
            oss << i << '\t' << start_perf_test_with_atomic(i) << '\n';
        }
        TURBO_LOG(INFO) << "Atomic performance:\n" << oss.str();
    }

    TEST_F(ReducerTest, Min) {
        turbo::min_gauge<uint64_t> reducer;
        ASSERT_EQ(std::numeric_limits<uint64_t>::max(), reducer.get_value());
        reducer << 10 << 20;
        ASSERT_EQ(10ul, reducer.get_value());
        reducer << 5;
        ASSERT_EQ(5ul, reducer.get_value());
        reducer << std::numeric_limits<uint64_t>::max();
        ASSERT_EQ(5ul, reducer.get_value());
        reducer << 0;
        ASSERT_EQ(0ul, reducer.get_value());

        turbo::min_gauge<int> reducer2;
        ASSERT_EQ(std::numeric_limits<int>::max(), reducer2.get_value());
        reducer2 << 10 << 20;
        ASSERT_EQ(10, reducer2.get_value());
        reducer2 << -5;
        ASSERT_EQ(-5, reducer2.get_value());
        reducer2 << std::numeric_limits<int>::max();
        ASSERT_EQ(-5, reducer2.get_value());
        reducer2 << 0;
        ASSERT_EQ(-5, reducer2.get_value());
        reducer2 << std::numeric_limits<int>::min();
        ASSERT_EQ(std::numeric_limits<int>::min(), reducer2.get_value());
    }

    TEST_F(ReducerTest, max) {
        turbo::max_gauge<uint64_t> reducer;
        ASSERT_EQ(std::numeric_limits<uint64_t>::min(), reducer.get_value());
        ASSERT_TRUE(reducer.valid());
        reducer << 20 << 10;
        ASSERT_EQ(20ul, reducer.get_value());
        reducer << 30;
        ASSERT_EQ(30ul, reducer.get_value());
        reducer << 0;
        ASSERT_EQ(30ul, reducer.get_value());

        turbo::max_gauge<int> reducer2;
        ASSERT_EQ(std::numeric_limits<int>::min(), reducer2.get_value());
        ASSERT_TRUE(reducer2.valid());
        reducer2 << 20 << 10;
        ASSERT_EQ(20, reducer2.get_value());
        reducer2 << 30;
        ASSERT_EQ(30, reducer2.get_value());
        reducer2 << 0;
        ASSERT_EQ(30, reducer2.get_value());
        reducer2 << std::numeric_limits<int>::max();
        ASSERT_EQ(std::numeric_limits<int>::max(), reducer2.get_value());
    }

    turbo::gauge<long> g_a;

    TEST_F(ReducerTest, global) {
        ASSERT_TRUE(g_a.valid());
        g_a.get_value();
    }

    void ReducerTest_window() {
        turbo::gauge<int> c1;
        turbo::max_gauge<int> c2;
        turbo::min_gauge<int> c3;
        turbo::window<turbo::gauge<int> > w1(&c1, 1);
        turbo::window<turbo::gauge<int> > w2(&c1, 2);
        turbo::window<turbo::gauge<int> > w3(&c1, 3);
        turbo::window<turbo::max_gauge<int> > w4(&c2, 1);
        turbo::window<turbo::max_gauge<int> > w5(&c2, 2);
        turbo::window<turbo::max_gauge<int> > w6(&c2, 3);
        turbo::window<turbo::min_gauge<int> > w7(&c3, 1);
        turbo::window<turbo::min_gauge<int> > w8(&c3, 2);
        turbo::window<turbo::min_gauge<int> > w9(&c3, 3);

        const int N = 6000;
        int count = 0;
        int total_count = 0;
        int64_t last_time = turbo::get_current_time_micros();
        for (int i = 1; i <= N; ++i) {
            c1 << 1;
            c2 << N - i;
            c3 << i;
            ++count;
            ++total_count;
            int64_t now = turbo::get_current_time_micros();
            if (now - last_time >= 1000000L) {
                last_time = now;
                ASSERT_EQ(total_count, c1.get_value());
                TURBO_LOG(INFO) << "c1=" << total_count
                                << " count=" << count
                                << " w1=" << w1
                                << " w2=" << w2
                                << " w3=" << w3
                                << " w4=" << w4
                                << " w5=" << w5
                                << " w6=" << w6
                                << " w7=" << w7
                                << " w8=" << w8
                                << " w9=" << w9;
                count = 0;
            } else {
                usleep(950);
            }
        }
    }


    struct Foo {
        int x;

        Foo() : x(0) {}

        explicit Foo(int x2) : x(x2) {}

        void operator+=(const Foo &rhs) {
            x += rhs.x;
        }
    };

    std::ostream &operator<<(std::ostream &os, const Foo &f) {
        return os << "Foo{" << f.x << "}";
    }

    TEST_F(ReducerTest, non_primitive) {
        turbo::gauge<Foo> adder;
        adder << Foo(2) << Foo(3) << Foo(4);
        ASSERT_EQ(9, adder.get_value().x);
    }

    bool g_stop = false;
    struct StringAppenderResult {
        int count;
    };

    static void *string_appender(void *arg) {
        turbo::gauge<std::string> *cater = (turbo::gauge<std::string> *) arg;
        int count = 0;
        std::string id = turbo::string_printf("%lld", (long long) pthread_self());
        std::string tmp = "a";
        for (count = 0; !count || !g_stop; ++count) {
            *cater << id << ":";
            for (char c = 'a'; c <= 'z'; ++c) {
                tmp[0] = c;
                *cater << tmp;
            }
            *cater << ".";
        }
        StringAppenderResult *res = new StringAppenderResult;
        res->count = count;
        TURBO_LOG(INFO) << "Appended " << count;
        return res;
    }

    TEST_F(ReducerTest, non_primitive_mt) {
        turbo::gauge<std::string> cater;
        pthread_t th[8];
        g_stop = false;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            pthread_create(&th[i], nullptr, string_appender, &cater);
        }
        usleep(50000);
        g_stop = true;
        turbo::container::hash_map<pthread_t, int> appended_count;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            StringAppenderResult *res = nullptr;
            pthread_join(th[i], (void **) &res);
            appended_count[th[i]] = res->count;
            delete res;
        }
        turbo::container::hash_map<pthread_t, int> got_count;
        std::string res = cater.get_value();
        for (turbo::StringSplitter sp(res.c_str(), '.'); sp; ++sp) {
            char *endptr = nullptr;
            ++got_count[(pthread_t) strtoll(sp.field(), &endptr, 10)];
            ASSERT_EQ(27LL, sp.field() + sp.length() - endptr)
                                        << std::string_view(sp.field(), sp.length());
            ASSERT_EQ(0, memcmp(":abcdefghijklmnopqrstuvwxyz", endptr, 27));
        }
        ASSERT_EQ(appended_count.size(), got_count.size());
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(appended_count[th[i]], got_count[th[i]]);
        }
    }

    TEST_F(ReducerTest, simple_window) {
        turbo::gauge<int64_t> a;
        turbo::window<turbo::gauge<int64_t> > w(&a, 10);
        a << 100;
        sleep(3);
        const int64_t v = w.get_value();
        ASSERT_EQ(100, v) << "v=" << v;
    }
} // namespace
