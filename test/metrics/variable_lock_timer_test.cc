
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include <iostream>
#include <condition_variable>

#include "flare/base/gperftools_profiler.h"
#include "flare/metrics/utils/lock_timer.h"

namespace {
    struct DummyMutex {
    };
}

namespace std {
    template<>
    class lock_guard<DummyMutex> {
    public:
        lock_guard(DummyMutex &) {}
    };

    template<>
    class unique_lock<DummyMutex> {
    public:
        unique_lock() {}

        unique_lock(DummyMutex &) {}

        template<typename T>
        unique_lock(DummyMutex &, T) {}

        bool try_lock() { return true; }

        void lock() {}

        void unlock() {}
    };
} // namespace std

namespace {
    using flare::IntRecorder;
    using flare::LatencyRecorder;
    using flare::utils::MutexWithRecorder;
    using flare::utils::MutexWithLatencyRecorder;

    class LockTimerTest : public testing::Test {
    };

    TEST_F(LockTimerTest, MutexWithRecorder) {
        IntRecorder recorder;
        MutexWithRecorder<std::mutex> mutex(recorder);
        {
            FLARE_SCOPED_LOCK(mutex);
        }
        ASSERT_EQ(1u, recorder.get_value().num);
        FLARE_LOG(INFO) << recorder;
        {
            std::unique_lock<decltype(mutex)> lck(mutex);
            lck.unlock();
            lck.lock();
            ASSERT_EQ(2u, recorder.get_value().num);
            FLARE_LOG(INFO) << recorder;
            std::condition_variable cond;
            cond.wait_for(lck, std::chrono::milliseconds(10));
        }
        ASSERT_EQ(3u, recorder.get_value().num);
    }

    TEST_F(LockTimerTest, MutexWithLatencyRecorder) {
        LatencyRecorder recorder(10);
        MutexWithLatencyRecorder<std::mutex> mutex(recorder);
        {
            FLARE_SCOPED_LOCK(mutex);
        }
        ASSERT_EQ(1u, recorder.count());
        {
            std::unique_lock<decltype(mutex)> lck(mutex);
            lck.unlock();
            lck.lock();
            ASSERT_EQ(2u, recorder.count());
            FLARE_LOG(INFO) << recorder;
            std::condition_variable cond;
            cond.wait_for(lck, std::chrono::milliseconds(10));
        }
        ASSERT_EQ(3u, recorder.count());
    }

    TEST_F(LockTimerTest, pthread_mutex_and_cond) {
        LatencyRecorder recorder(10);
        MutexWithLatencyRecorder<pthread_mutex_t> mutex(recorder);
        {
            FLARE_SCOPED_LOCK(mutex);
        }
        ASSERT_EQ(1u, recorder.count());
        {
            std::unique_lock<MutexWithLatencyRecorder<pthread_mutex_t> > lck(mutex);
            ASSERT_EQ(1u, recorder.count());
            timespec due_time = flare::time_point::future_unix_millis(10).to_timespec();
            pthread_cond_t cond;
            ASSERT_EQ(0, pthread_cond_init(&cond, nullptr));
            pthread_cond_timedwait(&cond, &(pthread_mutex_t &) mutex, &due_time);
            pthread_cond_timedwait(&cond, &mutex.mutex(), &due_time);
            ASSERT_EQ(0, pthread_cond_destroy(&cond));
        }
        ASSERT_EQ(2u, recorder.count());
    }

    const static size_t OPS_PER_THREAD = 1000;

    template<typename M>
    void *signal_lock_thread(void *arg) {
        M *m = (M *) arg;
        for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
            {
                std::unique_lock<M> lck(*m);
                usleep(10);
            }
        }
        return nullptr;
    }

    TEST_F(LockTimerTest, signal_lock_time) {
        IntRecorder r0;
        MutexWithRecorder<pthread_mutex_t> m0(r0);
        pthread_t threads[4];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            ASSERT_EQ(0, pthread_create(&threads[i], nullptr,
                                        signal_lock_thread<MutexWithRecorder<pthread_mutex_t> >, &m0));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        FLARE_LOG(INFO) << r0;
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r0.get_value().num);
        LatencyRecorder r1;
        MutexWithLatencyRecorder<pthread_mutex_t> m1(r1);
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            ASSERT_EQ(0, pthread_create(&threads[i], nullptr,
                                        signal_lock_thread<MutexWithLatencyRecorder<pthread_mutex_t> >, &m1));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        FLARE_LOG(INFO) << r1._latency;
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r1.count());
    }

    template<typename M0, typename M1>
    struct DoubleLockArg {
        M0 m0;
        M1 m1;
    };

    template<typename M0, typename M1>
    void *double_lock_thread(void *arg) {
        DoubleLockArg<M0, M1> *dla = (DoubleLockArg<M0, M1> *) arg;
        for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
            std::unique_lock<M0> lck0(dla->m0, std::defer_lock);
            std::unique_lock<M1> lck1(dla->m1, std::defer_lock);
            flare::base::double_lock(lck0, lck1);
            usleep(10);
        }
        return nullptr;
    }

    TEST_F(LockTimerTest, double_lock_time) {
        typedef MutexWithRecorder<pthread_mutex_t> M0;
        typedef MutexWithLatencyRecorder<pthread_mutex_t> M1;
        DoubleLockArg<M0, M1> arg;
        IntRecorder r0;
        LatencyRecorder r1;
        arg.m0.set_recorder(r0);
        arg.m1.set_recorder(r1);
        pthread_t threads[4];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            ASSERT_EQ(0, pthread_create(&threads[i], nullptr,
                                        double_lock_thread<M0, M1>, &arg));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r0.get_value().num);
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r1.count());
        FLARE_LOG(INFO) << r0;
        FLARE_LOG(INFO) << r1._latency;
        r0.reset();
        r1._latency.reset();
        DoubleLockArg<M1, M0> arg1;
        arg1.m0.set_recorder(r1);
        arg1.m1.set_recorder(r0);
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            ASSERT_EQ(0, pthread_create(&threads[i], nullptr,
                                        double_lock_thread<M1, M0>, &arg1));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r0.get_value().num);
        ASSERT_EQ(OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads), (size_t) r1.count());
        FLARE_LOG(INFO) << r0;
        FLARE_LOG(INFO) << r1._latency;
    }

    TEST_F(LockTimerTest, overhead) {
        LatencyRecorder r0;
        MutexWithLatencyRecorder<DummyMutex> m0(r0);
        flare::stop_watcher timer;
        const size_t N = 1000 * 1000 * 10;

        ProfilerStart("mutex_with_latency_recorder.prof");
        timer.start();
        for (size_t i = 0; i < N; ++i) {
            FLARE_SCOPED_LOCK(m0);
        }
        timer.stop();
        ProfilerStop();
        FLARE_LOG(INFO) << "The overhead of MutexWithLatencyRecorder is "
                        << timer.n_elapsed() / N << "ns";

        IntRecorder r1;
        MutexWithRecorder<DummyMutex> m1(r1);
        ProfilerStart("mutex_with_recorder.prof");
        timer.start();
        for (size_t i = 0; i < N; ++i) {
            FLARE_SCOPED_LOCK(m1);
        }
        timer.stop();
        ProfilerStop();
        FLARE_LOG(INFO) << "The overhead of MutexWithRecorder is "
                        << timer.n_elapsed() / N << "ns";
        MutexWithRecorder<DummyMutex> m2;
        ProfilerStart("mutex_with_timer.prof");
        timer.start();
        for (size_t i = 0; i < N; ++i) {
            FLARE_SCOPED_LOCK(m2);
        }
        timer.stop();
        ProfilerStop();
        FLARE_LOG(INFO) << "The overhead of timer is "
                        << timer.n_elapsed() / N << "ns";
    }
} // namespace
