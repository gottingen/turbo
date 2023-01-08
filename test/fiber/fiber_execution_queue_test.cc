

#include "testing/gtest_wrap.h"

#include <flare/fiber/internal/execution_queue.h>
#include <flare/fiber/internal/sys_futex.h>
#include <flare/fiber/fiber_latch.h>
#include "flare/times/time.h"
#include "flare/base/fast_rand.h"
#include "flare/base/gperftools_profiler.h"
#include "flare/fiber/this_fiber.h"

namespace {
    bool stopped = false;

    class ExecutionQueueTest : public testing::Test {
    protected:

        void SetUp() { stopped = false; }

        void TearDown() {}
    };

    struct LongIntTask {
        long value;
        flare::fiber_latch *event;

        LongIntTask(long v)
                : value(v), event(nullptr) {}

        LongIntTask(long v, flare::fiber_latch *e)
                : value(v), event(e) {}

        LongIntTask() : value(0), event(nullptr) {}
    };

    int add(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        stopped = iter.is_queue_stopped();
        int64_t *result = (int64_t *) meta;
        for (; iter; ++iter) {
            *result += iter->value;
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, single_thread) {
        int64_t result = 0;
        int64_t expected_result = 0;
        stopped = false;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id;
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        for (int i = 0; i < 100; ++i) {
            expected_result += i;
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, i));
        }
        FLARE_LOG(INFO) << "stop";
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_NE(0, flare::fiber_internal::execution_queue_execute(queue_id, 0));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(expected_result, result);
        ASSERT_TRUE(stopped);
    }

    struct PushArg {
        flare::fiber_internal::ExecutionQueueId<LongIntTask> id;
        std::atomic<int64_t> total_num;
        std::atomic<int64_t> total_time;
        std::atomic<int64_t> expected_value;
        volatile bool stopped;
        bool wait_task_completed;

        PushArg() {
            memset(this, 0, sizeof(*this));
        }
    };

    void *push_thread(void *arg) {
        PushArg *pa = (PushArg *) arg;
        int64_t sum = 0;
        flare::stop_watcher timer;
        timer.start();
        int num = 0;
        flare::fiber_latch e;
        LongIntTask t(num, pa->wait_task_completed ? &e : nullptr);
        if (pa->wait_task_completed) {
            e.reset(1);
        }
        while (flare::fiber_internal::execution_queue_execute(pa->id, t) == 0) {
            sum += num;
            t.value = ++num;
            if (pa->wait_task_completed) {
                e.wait();
                e.reset(1);
            }
        }
        timer.stop();
        pa->expected_value.fetch_add(sum, std::memory_order_relaxed);
        pa->total_num.fetch_add(num);
        pa->total_time.fetch_add(timer.n_elapsed());
        return nullptr;
    }

    void *push_thread_which_addresses_execq(void *arg) {
        PushArg *pa = (PushArg *) arg;
        int64_t sum = 0;
        flare::stop_watcher timer;
        timer.start();
        int num = 0;
        flare::fiber_internal::ExecutionQueue<LongIntTask>::scoped_ptr_t ptr
                = flare::fiber_internal::execution_queue_address(pa->id);
        EXPECT_TRUE(ptr);
        while (ptr->execute(num) == 0) {
            sum += num;
            ++num;
        }
        EXPECT_TRUE(ptr->stopped());
        timer.stop();
        pa->expected_value.fetch_add(sum, std::memory_order_relaxed);
        pa->total_num.fetch_add(num);
        pa->total_time.fetch_add(timer.n_elapsed());
        return nullptr;
    }

    TEST_F(ExecutionQueueTest, performance) {
        pthread_t threads[8];
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        int64_t result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        PushArg pa;
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
        ProfilerStart("execq.prof");
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread_which_addresses_execq, &pa);
        }
        usleep(500 * 1000);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ProfilerStop();
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(pa.expected_value.load(), result);
        FLARE_LOG(INFO) << "With addressed execq, each execution_queue_execute takes "
                  << pa.total_time.load() / pa.total_num.load()
                  << " total_num=" << pa.total_num
                  << " ns with " << FLARE_ARRAY_SIZE(threads) << " threads";
#define BENCHMARK_BOTH
#ifdef BENCHMARK_BOTH
        result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
        ProfilerStart("execq_id.prof");
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread, &pa);
        }
        usleep(500 * 1000);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ProfilerStop();
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(pa.expected_value.load(), result);
        FLARE_LOG(INFO) << "With id explicitly, execution_queue_execute takes "
                  << pa.total_time.load() / pa.total_num.load()
                  << " total_num=" << pa.total_num
                  << " ns with " << FLARE_ARRAY_SIZE(threads) << " threads";
#endif  // BENCHMARK_BOTH
    }

    volatile bool g_suspending = false;
    volatile bool g_should_be_urgent = false;
    int urgent_times = 0;

    int add_with_suspend(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        int64_t *result = (int64_t *) meta;
        if (iter.is_queue_stopped()) {
            stopped = true;
            return 0;
        }
        if (g_should_be_urgent) {
            g_should_be_urgent = false;
            EXPECT_EQ(-1, iter->value) << urgent_times;
            if (iter->event) { iter->event->signal(); }
            ++iter;
            EXPECT_FALSE(iter) << urgent_times;
            ++urgent_times;
        } else {
            for (; iter; ++iter) {
                if (iter->value == -100) {
                    g_suspending = true;
                    while (g_suspending) {
                        flare::fiber_sleep_for(100);
                    }
                    g_should_be_urgent = true;
                    if (iter->event) { iter->event->signal(); }
                    EXPECT_FALSE(++iter);
                    return 0;
                } else {
                    *result += iter->value;
                    if (iter->event) { iter->event->signal(); }
                }
            }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, execute_urgent) {
        g_should_be_urgent = false;
        pthread_t threads[10];
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        int64_t result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend, &result));
        PushArg pa;
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
        pa.wait_task_completed = true;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread, &pa);
        }
        g_suspending = false;
        usleep(1000);

        for (int i = 0; i < 100; ++i) {
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, -100));
            while (!g_suspending) {
                usleep(100);
            }
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(
                    queue_id, -1, &flare::fiber_internal::TASK_OPTIONS_URGENT));
            g_suspending = false;
            usleep(100);
        }
        usleep(500 * 1000);
        pa.stopped = true;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        FLARE_LOG(INFO) << "result=" << result;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(pa.expected_value.load(), result);
    }

    TEST_F(ExecutionQueueTest, urgent_task_is_the_last_task) {
        g_should_be_urgent = false;
        g_suspending = false;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        int64_t result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend, &result));
        g_suspending = false;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, -100));
        while (!g_suspending) {
            usleep(10);
        }
        FLARE_LOG(INFO) << "Going to push";
        int64_t expected = 0;
        for (int i = 1; i < 100; ++i) {
            expected += i;
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, i));
        }
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, -1,
                                                                    &flare::fiber_internal::TASK_OPTIONS_URGENT));
        usleep(100);
        g_suspending = false;
        std::atomic_thread_fence(std::memory_order_acq_rel);
        usleep(10 * 1000);
        FLARE_LOG(INFO) << "going to quit";
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(expected, result);
    }

    long next_task[1024];
    std::atomic<int> num_threads(0);

    void *push_thread_with_id(void *arg) {
        flare::fiber_internal::ExecutionQueueId<LongIntTask> id = {(uint64_t) arg};
        int thread_id = num_threads.fetch_add(1, std::memory_order_relaxed);
        FLARE_LOG(INFO) << "Start thread" << thread_id;
        for (int i = 0; i < 100000; ++i) {
            flare::fiber_internal::execution_queue_execute(id, ((long) thread_id << 32) | i);
        }
        return nullptr;
    }

    int check_order(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        for (; iter; ++iter) {
            long value = iter->value;
            int thread_id = value >> 32;
            long task = value & 0xFFFFFFFFul;
            if (task != next_task[thread_id]++) {
                EXPECT_TRUE(false) << "task=" << task << " thread_id=" << thread_id;
                ++*(long *) meta;
            }
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, multi_threaded_order) {
        memset(next_task, 0, sizeof(next_task));
        long disorder_times = 0;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  check_order, &disorder_times));
        pthread_t threads[12];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread_with_id, (void *) queue_id.value);
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(0, disorder_times);
    }

    int check_running_thread(void *arg, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        for (; iter; ++iter) {}
        EXPECT_EQ(pthread_self(), (pthread_t) arg);
        return 0;
    }

    TEST_F(ExecutionQueueTest, in_place_task) {
        pthread_t thread_id = pthread_self();
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  check_running_thread,
                                                                  (void *) thread_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(
                queue_id, 0, &flare::fiber_internal::TASK_OPTIONS_INPLACE));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
    }

    struct InPlaceTask {
        bool first_task;
        pthread_t thread_id;
    };

    void *run_first_tasks(void *arg) {
        flare::fiber_internal::ExecutionQueueId<InPlaceTask> queue_id = {(uint64_t) arg};
        InPlaceTask task;
        task.first_task = true;
        task.thread_id = pthread_self();
        EXPECT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, task,
                                                                    &flare::fiber_internal::TASK_OPTIONS_INPLACE));
        return nullptr;
    }

    int stuck_and_check_running_thread(void *arg, flare::fiber_internal::TaskIterator<InPlaceTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        std::atomic<int> *futex = (std::atomic<int> *) arg;
        if (iter->first_task) {
            EXPECT_EQ(pthread_self(), iter->thread_id);
            futex->store(1);
            flare::fiber_internal::futex_wake_private(futex, 1);
            while (futex->load() != 2) {
                flare::fiber_internal::futex_wait_private(futex, 1, nullptr);
            }
            ++iter;
            EXPECT_FALSE(iter);
        } else {
            for (; iter; ++iter) {
                EXPECT_FALSE(iter->first_task);
                EXPECT_NE(pthread_self(), iter->thread_id);
            }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, should_start_new_thread_on_more_tasks) {
        flare::fiber_internal::ExecutionQueueId<InPlaceTask> queue_id = {0};
        flare::fiber_internal::ExecutionQueueOptions options;
        std::atomic<int> futex(0);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  stuck_and_check_running_thread,
                                                                  (void *) &futex));
        pthread_t thread;
        ASSERT_EQ(0, pthread_create(&thread, nullptr, run_first_tasks, (void *) queue_id.value));
        while (futex.load() != 1) {
            flare::fiber_internal::futex_wait_private(&futex, 0, nullptr);
        }
        for (size_t i = 0; i < 100; ++i) {
            InPlaceTask task;
            task.first_task = false;
            task.thread_id = pthread_self();
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, task,
                                                                        &flare::fiber_internal::TASK_OPTIONS_INPLACE));
        }
        futex.store(2);
        flare::fiber_internal::futex_wake_private(&futex, 1);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
    }

    void *inplace_push_thread(void *arg) {
        flare::fiber_internal::ExecutionQueueId<LongIntTask> id = {(uint64_t) arg};
        int thread_id = num_threads.fetch_add(1, std::memory_order_relaxed);
        FLARE_LOG(INFO) << "Start thread" << thread_id;
        for (int i = 0; i < 100000; ++i) {
            flare::fiber_internal::execution_queue_execute(id, ((long) thread_id << 32) | i,
                                                           &flare::fiber_internal::TASK_OPTIONS_INPLACE);
        }
        return nullptr;
    }

    TEST_F(ExecutionQueueTest, inplace_and_order) {
        memset(next_task, 0, sizeof(next_task));
        long disorder_times = 0;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  check_order, &disorder_times));
        pthread_t threads[12];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &inplace_push_thread, (void *) queue_id.value);
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(0, disorder_times);
    }

    TEST_F(ExecutionQueueTest, size_of_task_node) {
        FLARE_LOG(INFO) << "sizeof(TaskNode)=" << sizeof(flare::fiber_internal::TaskNode);
    }

    int add_with_suspend2(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        int64_t *result = (int64_t *) meta;
        if (iter.is_queue_stopped()) {
            stopped = true;
            return 0;
        }
        for (; iter; ++iter) {
            if (iter->value == -100) {
                g_suspending = true;
                while (g_suspending) {
                    usleep(10);
                }
                if (iter->event) { iter->event->signal(); }
            } else {
                *result += iter->value;
                if (iter->event) { iter->event->signal(); }
            }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, cancel) {
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        int64_t result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend2, &result));
        g_suspending = false;
        flare::fiber_internal::TaskHandle handle0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, -100, nullptr, &handle0));
        while (!g_suspending) {
            usleep(10);
        }
        ASSERT_EQ(1, flare::fiber_internal::execution_queue_cancel(handle0));
        ASSERT_EQ(1, flare::fiber_internal::execution_queue_cancel(handle0));
        flare::fiber_internal::TaskHandle handle1;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, 100, nullptr, &handle1));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_cancel(handle1));
        g_suspending = false;
        ASSERT_EQ(-1, flare::fiber_internal::execution_queue_cancel(handle1));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(0, result);
    }

    struct CancelSelf {
        std::atomic<flare::fiber_internal::TaskHandle *> handle;
    };

    int cancel_self(void * /*meta*/, flare::fiber_internal::TaskIterator<CancelSelf *> &iter) {

        for (; iter; ++iter) {
            while ((*iter)->handle == nullptr) {
                usleep(10);
            }
            EXPECT_EQ(1, flare::fiber_internal::execution_queue_cancel(*(*iter)->handle.load()));
            EXPECT_EQ(1, flare::fiber_internal::execution_queue_cancel(*(*iter)->handle.load()));
            EXPECT_EQ(1, flare::fiber_internal::execution_queue_cancel(*(*iter)->handle.load()));
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, cancel_self) {
        flare::fiber_internal::ExecutionQueueId<CancelSelf *> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  cancel_self, nullptr));
        CancelSelf task;
        task.handle = nullptr;
        flare::fiber_internal::TaskHandle handle;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, &task, nullptr, &handle));
        task.handle.store(&handle);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
    }

    struct AddTask {
        int value;
        bool cancel_task;
        int cancel_value;
        flare::fiber_internal::TaskHandle handle;
    };

    struct AddMeta {
        int64_t sum;
        std::atomic<int64_t> expected;
        std::atomic<int64_t> succ_times;
        std::atomic<int64_t> race_times;
        std::atomic<int64_t> fail_times;
    };

    int add_with_cancel(void *meta, flare::fiber_internal::TaskIterator<AddTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        AddMeta *m = (AddMeta *) meta;
        for (; iter; ++iter) {
            if (iter->cancel_task) {
                const int rc = flare::fiber_internal::execution_queue_cancel(iter->handle);
                if (rc == 0) {
                    m->expected.fetch_sub(iter->cancel_value);
                    m->succ_times.fetch_add(1);
                } else if (rc < 0) {
                    m->fail_times.fetch_add(1);
                } else {
                    m->race_times.fetch_add(1);
                }
            } else {
                m->sum += iter->value;
            }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, random_cancel) {
        flare::fiber_internal::ExecutionQueueId<AddTask> queue_id = {0};
        AddMeta m;
        m.sum = 0;
        m.expected.store(0);
        m.succ_times.store(0);
        m.fail_times.store(0);
        m.race_times.store(0);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, nullptr,
                                                                  add_with_cancel, &m));
        int64_t expected = 0;
        for (int i = 0; i < 100000; ++i) {
            flare::fiber_internal::TaskHandle h;
            AddTask t;
            t.value = i;
            t.cancel_task = false;
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, t, nullptr, &h));
            const int r = flare::base::fast_rand_less_than(4);
            expected += i;
            if (r == 0) {
                if (flare::fiber_internal::execution_queue_cancel(h) == 0) {
                    expected -= i;
                }
            } else if (r == 1) {
                t.cancel_task = true;
                t.cancel_value = i;
                t.handle = h;
                ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, t, nullptr));
            } else if (r == 2) {
                t.cancel_task = true;
                t.cancel_value = i;
                t.handle = h;
                ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, t,
                                                                            &flare::fiber_internal::TASK_OPTIONS_URGENT));
            } else {
                // do nothing;
            }
        }
        m.expected.fetch_add(expected);
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(m.sum, m.expected.load());
        FLARE_LOG(INFO) << "sum=" << m.sum << " race_times=" << m.race_times
                  << " succ_times=" << m.succ_times
                  << " fail_times=" << m.fail_times;

    }

    int add2(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        if (iter) {
            int64_t *result = (int64_t *) meta;
            *result += iter->value;
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, not_do_iterate_at_all) {
        int64_t result = 0;
        int64_t expected_result = 0;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id;
        flare::fiber_internal::ExecutionQueueOptions options;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add2, &result));
        for (int i = 0; i < 100; ++i) {
            expected_result += i;
            ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, i));
        }
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_NE(0, flare::fiber_internal::execution_queue_execute(queue_id, 0));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));
        ASSERT_EQ(expected_result, result);
    }

    int add_with_suspend3(void *meta, flare::fiber_internal::TaskIterator<LongIntTask> &iter) {
        int64_t *result = (int64_t *) meta;
        if (iter.is_queue_stopped()) {
            stopped = true;
            return 0;
        }
        for (; iter; ++iter) {
            if (iter->value == -100) {
                g_suspending = true;
                while (g_suspending) {
                    usleep(10);
                }
                if (iter->event) { iter->event->signal(); }
            } else {
                *result += iter->value;
                if (iter->event) { iter->event->signal(); }
            }
        }
        return 0;
    }

    TEST_F(ExecutionQueueTest, cancel_unexecuted_high_priority_task) {
        g_should_be_urgent = false;
        flare::fiber_internal::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        flare::fiber_internal::ExecutionQueueOptions options;
        int64_t result = 0;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend3, &result));
        // Push a normal task to make the executor suspend
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, -100));
        while (!g_suspending) {
            usleep(10);
        }
        // At this point, executor is suspended by the first task. Then we put
        // a high_priority task which is going to be cancelled immediately,
        // expecting that both operations are successful.
        flare::fiber_internal::TaskHandle h;
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(
                queue_id, -100, &flare::fiber_internal::TASK_OPTIONS_URGENT, &h));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_cancel(h));

        // Resume executor
        g_suspending = false;

        // Push a normal task
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_execute(queue_id, 12345));

        // The execq should stop normally
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_stop(queue_id));
        ASSERT_EQ(0, flare::fiber_internal::execution_queue_join(queue_id));

        ASSERT_EQ(12345, result);
    }
} // namespace
