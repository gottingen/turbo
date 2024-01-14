// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 24-1-3.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "turbo/testing/test.h"
#include <turbo/fiber/execution_queue.h>
#include "turbo/times/time.h"
#include "turbo/times/stop_watcher.h"

#include "turbo/fiber/fiber_latch.h"
#include "turbo/random/uniform.h"
#include "turbo/concurrent/spinlock_wait.h"

namespace {
    bool stopped = false;

    class ExecutionQueueTest{
    public:
        ExecutionQueueTest() {
            stopped = false;
        }
    };

    struct LongIntTask {
        long value;
        turbo::FiberLatch *event;

        LongIntTask(long v)
                : value(v), event(nullptr) {}

        LongIntTask(long v, turbo::FiberLatch *e)
                : value(v), event(e) {}

        LongIntTask() : value(0), event(nullptr) {}
    };

    int add(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
        stopped = iter.is_queue_stopped();
        int64_t *result = (int64_t *) meta;
        for (; iter; ++iter) {
            *result += iter->value;
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "single_thread") {
        int64_t result = 0;
        int64_t expected_result = 0;
        stopped = false;
        turbo::ExecutionQueueId<LongIntTask> queue_id;
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        for (int i = 0; i < 100; ++i) {
            expected_result += i;
            REQUIRE_EQ(0, turbo::execution_queue_execute(queue_id, i));
        }
        TLOG_INFO("stop");
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE_NE(0,  turbo::execution_queue_execute(queue_id, 0));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(expected_result, result);
        REQUIRE(stopped);
    }

    struct PushArg {
        turbo::ExecutionQueueId<LongIntTask> id;
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
        turbo::StopWatcher timer;
        timer.reset();
        int num = 0;
        turbo::FiberLatch e;
        LongIntTask t(num, pa->wait_task_completed ? &e : nullptr);
        if (pa->wait_task_completed) {
            e.reset(1);
        }
        while ( turbo::execution_queue_execute(pa->id, t) == 0) {
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
        pa->total_time.fetch_add(timer.elapsed_nano());
        return nullptr;
    }

    void *push_thread_which_addresses_execq(void *arg) {
        PushArg *pa = (PushArg *) arg;
        int64_t sum = 0;
        turbo::StopWatcher timer;
        timer.reset();
        int num = 0;
        turbo::ExecutionQueue<LongIntTask>::unique_ptr_t ptr
                = turbo::execution_queue_address(pa->id);
        REQUIRE(ptr);
        while (ptr->execute(num) == 0) {
            sum += num;
            ++num;
        }
        REQUIRE(ptr->stopped());
        timer.stop();
        pa->expected_value.fetch_add(sum, std::memory_order_relaxed);
        pa->total_num.fetch_add(num);
        pa->total_time.fetch_add(timer.elapsed_nano());
        return nullptr;
    }



    TEST_CASE_FIXTURE(ExecutionQueueTest, "performance") {
        pthread_t threads[8];
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        int64_t result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        PushArg pa;
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
#ifdef ENABLE_PROFILE
        ProfilerStart("execq.prof");
#endif
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread_which_addresses_execq, &pa);
        }
        usleep(500 * 1000);
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
#ifdef ENABLE_PROFILE
        ProfilerStop();
#endif
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(pa.expected_value.load(), result);
        TLOG_INFO("With addressed execq, each execution_queue_execute takes {} total_num={} ns with {} threads",
                  pa.total_time.load() / pa.total_num.load(), pa.total_num.load(), TURBO_ARRAY_SIZE(threads));
#define BENCHMARK_BOTH
#ifdef BENCHMARK_BOTH
        result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add, &result));
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
#ifdef ENABLE_PROFILE
        ProfilerStart("execq_id.prof");
#endif
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread, &pa);
        }
        usleep(500 * 1000);
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
#ifdef ENABLE_PROFILE
        ProfilerStop();
#endif
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(pa.expected_value.load(), result);
        TLOG_INFO("With id explicitly, execution_queue_execute takes {} total_num={} ns with {} threads",
                  pa.total_time.load() / pa.total_num.load(), pa.total_num.load(), TURBO_ARRAY_SIZE(threads));
#endif  // BENCHMARK_BOTH
    }

    volatile bool g_suspending = false;
    volatile bool g_should_be_urgent = false;
    int urgent_times = 0;

    int add_with_suspend(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
        int64_t *result = (int64_t *) meta;
        if (iter.is_queue_stopped()) {
            stopped = true;
            return 0;
        }
        if (g_should_be_urgent) {
            g_should_be_urgent = false;
            REQUIRE_EQ(-1, iter->value);
            if (iter->event) { iter->event->signal(); }
            ++iter;
            REQUIRE_FALSE(iter);
            ++urgent_times;
        } else {
            for (; iter; ++iter) {
                if (iter->value == -100) {
                    g_suspending = true;
                    while (g_suspending) {
                        turbo::fiber_sleep_for(turbo::Duration::microseconds(100));
                    }
                    g_should_be_urgent = true;
                    if (iter->event) { iter->event->signal(); }
                    REQUIRE_FALSE(++iter);
                    return 0;
                } else {
                    *result += iter->value;
                    if (iter->event) { iter->event->signal(); }
                }
            }
        }
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "execute_urgent") {
        g_should_be_urgent = false;
        pthread_t threads[10];
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        int64_t result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options, add_with_suspend, &result));
        PushArg pa;
        pa.id = queue_id;
        pa.total_num = 0;
        pa.total_time = 0;
        pa.expected_value = 0;
        pa.stopped = false;
        pa.wait_task_completed = true;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread, &pa);
        }
        g_suspending = false;
        usleep(1000);

        for (int i = 0; i < 100; ++i) {
            REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, -100));
            while (!g_suspending) {
                usleep(100);
            }
            REQUIRE_EQ(0,  turbo::execution_queue_execute(
                    queue_id, -1, &turbo::TASK_OPTIONS_URGENT));
            g_suspending = false;
            usleep(100);
        }
        usleep(500 * 1000);
        pa.stopped = true;
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        TLOG_INFO("urgent_times={}", urgent_times);
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(pa.expected_value.load(), result);
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "urgent_task_is_the_last_task") {
        g_should_be_urgent = false;
        g_suspending = false;
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        int64_t result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend, &result));
        g_suspending = false;
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, -100));
        while (!g_suspending) {
            usleep(10);
        }
        TLOG_INFO("Going to push");
        int64_t expected = 0;
        for (int i = 1; i < 100; ++i) {
            expected += i;
            REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, i));
        }
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, -1,
                                                                    &turbo::TASK_OPTIONS_URGENT));
        usleep(100);
        g_suspending = false;
        std::atomic_thread_fence(std::memory_order_acq_rel);
        usleep(10 * 1000);
        TLOG_INFO("Going to quit");
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(expected, result);
    }

    long next_task[1024];
    std::atomic<int> num_threads(0);

    void *push_thread_with_id(void *arg) {
        turbo::ExecutionQueueId<LongIntTask> id = {(uint64_t) arg};
        int thread_id = num_threads.fetch_add(1, std::memory_order_relaxed);
        TLOG_INFO("Start thread {}", thread_id);
        for (int i = 0; i < 100000; ++i) {
             turbo::execution_queue_execute(id, ((long) thread_id << 32) | i);
        }
        return nullptr;
    }

    int check_order(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
        for (; iter; ++iter) {
            long value = iter->value;
            int thread_id = value >> 32;
            long task = value & 0xFFFFFFFFul;
            if (task != next_task[thread_id]++) {
                REQUIRE(false);
                ++*(long *) meta;
            }
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "multi_threaded_order") {
        memset(next_task, 0, sizeof(next_task));
        long disorder_times = 0;
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  check_order, &disorder_times));
        pthread_t threads[12];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &push_thread_with_id, (void *) queue_id.value);
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(0, disorder_times);
    }

    int check_running_thread(void *arg, turbo::TaskIterator<LongIntTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        for (; iter; ++iter) {}
        REQUIRE_EQ(pthread_self(), (pthread_t) arg);
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "in_place_task") {
        pthread_t thread_id = pthread_self();
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  check_running_thread,
                                                                  (void *) thread_id));
        REQUIRE_EQ(0,  turbo::execution_queue_execute(
                queue_id, 0, &turbo::TASK_OPTIONS_INPLACE));
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
    }

    struct InPlaceTask {
        bool first_task;
        pthread_t thread_id;
    };

    void *run_first_tasks(void *arg) {
        turbo::ExecutionQueueId<InPlaceTask> queue_id = {(uint64_t) arg};
        InPlaceTask task;
        task.first_task = true;
        task.thread_id = pthread_self();
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, task,
                                                                    &turbo::TASK_OPTIONS_INPLACE));
        return nullptr;
    }

    int stuck_and_check_running_thread(void *arg, turbo::TaskIterator<InPlaceTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        auto *futex = (turbo::SpinWaiter *) arg;
        if (iter->first_task) {
            REQUIRE_EQ(pthread_self(), iter->thread_id);
            futex->store(1);
            futex->wake_one();
            while (futex->load() != 2) {
                futex->wait(1);
            }
            ++iter;
            REQUIRE_FALSE(iter);
        } else {
            for (; iter; ++iter) {
                REQUIRE_FALSE(iter->first_task);
                REQUIRE_NE(pthread_self(), iter->thread_id);
            }
        }
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "should_start_new_thread_on_more_tasks") {
        turbo::ExecutionQueueId<InPlaceTask> queue_id = {0};
        turbo::ExecutionQueueOptions options;
        turbo::SpinWaiter futex(0);

        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  stuck_and_check_running_thread,
                                                                  (void *) &futex));
        pthread_t thread;
        REQUIRE_EQ(0, pthread_create(&thread, nullptr, run_first_tasks, (void *) queue_id.value));
        while (futex.load() != 1) {
            futex.wake(0);
        }
        for (size_t i = 0; i < 100; ++i) {
            InPlaceTask task;
            task.first_task = false;
            task.thread_id = pthread_self();
            REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, task,
                                                                        &turbo::TASK_OPTIONS_INPLACE));
        }
        futex.store(2);
        futex.wake_one();
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
    }

    void *inplace_push_thread(void *arg) {
        turbo::ExecutionQueueId<LongIntTask> id = {(uint64_t) arg};
        int thread_id = num_threads.fetch_add(1, std::memory_order_relaxed);
        TLOG_INFO("Start thread {}", thread_id);
        for (int i = 0; i < 100000; ++i) {
             turbo::execution_queue_execute(id, ((long) thread_id << 32) | i,
                                                           &turbo::TASK_OPTIONS_INPLACE);
        }
        return nullptr;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "inplace_and_order") {
        memset(next_task, 0, sizeof(next_task));
        long disorder_times = 0;
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  check_order, &disorder_times));
        pthread_t threads[12];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], nullptr, &inplace_push_thread, (void *) queue_id.value);
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(threads); ++i) {
            pthread_join(threads[i], nullptr);
        }
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(0, disorder_times);
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "size_of_task_node") {
        TLOG_INFO("sizeof(TaskNode)={}", sizeof(turbo::TaskNode));
    }

    int add_with_suspend2(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
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

    TEST_CASE_FIXTURE(ExecutionQueueTest, "cancel") {
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        int64_t result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend2, &result));
        g_suspending = false;
        turbo::TaskHandle handle0;
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, -100, nullptr, &handle0));
        while (!g_suspending) {
            usleep(10);
        }
        REQUIRE_EQ(1, turbo::execution_queue_cancel(handle0));
        REQUIRE_EQ(1, turbo::execution_queue_cancel(handle0));
        turbo::TaskHandle handle1;
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, 100, nullptr, &handle1));
        REQUIRE_EQ(0, turbo::execution_queue_cancel(handle1));
        g_suspending = false;
        REQUIRE_EQ(-1, turbo::execution_queue_cancel(handle1));
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(0, result);
    }

    struct CancelSelf {
        std::atomic<turbo::TaskHandle *> handle;
    };

    int cancel_self(void * /*meta*/, turbo::TaskIterator<CancelSelf *> &iter) {

        for (; iter; ++iter) {
            while ((*iter)->handle == nullptr) {
                usleep(10);
            }
            REQUIRE_EQ(1, turbo::execution_queue_cancel(*(*iter)->handle.load()));
            REQUIRE_EQ(1, turbo::execution_queue_cancel(*(*iter)->handle.load()));
            REQUIRE_EQ(1, turbo::execution_queue_cancel(*(*iter)->handle.load()));
        }
        return 0;
    }

    TEST_CASE_FIXTURE(ExecutionQueueTest, "cancel_self") {
        turbo::ExecutionQueueId<CancelSelf *> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  cancel_self, nullptr));
        CancelSelf task;
        task.handle = nullptr;
        turbo::TaskHandle handle;
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, &task, nullptr, &handle));
        task.handle.store(&handle);
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
    }

    struct AddTask {
        int value;
        bool cancel_task;
        int cancel_value;
        turbo::TaskHandle handle;
    };

    struct AddMeta {
        int64_t sum;
        std::atomic<int64_t> expected;
        std::atomic<int64_t> succ_times;
        std::atomic<int64_t> race_times;
        std::atomic<int64_t> fail_times;
    };

    int add_with_cancel(void *meta, turbo::TaskIterator<AddTask> &iter) {
        if (iter.is_queue_stopped()) {
            return 0;
        }
        AddMeta *m = (AddMeta *) meta;
        for (; iter; ++iter) {
            if (iter->cancel_task) {
                const int rc = turbo::execution_queue_cancel(iter->handle);
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

    TEST_CASE_FIXTURE(ExecutionQueueTest, "random_cancel") {
        turbo::ExecutionQueueId<AddTask> queue_id = {0};
        AddMeta m;
        m.sum = 0;
        m.expected.store(0);
        m.succ_times.store(0);
        m.fail_times.store(0);
        m.race_times.store(0);
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, nullptr,
                                                                  add_with_cancel, &m));
        int64_t expected = 0;
        for (int i = 0; i < 100000; ++i) {
            turbo::TaskHandle h;
            AddTask t;
            t.value = i;
            t.cancel_task = false;
            REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, t, nullptr, &h));
            const int r = turbo::fast_uniform(0u, 4u);
            expected += i;
            if (r == 0) {
                if (turbo::execution_queue_cancel(h) == 0) {
                    expected -= i;
                }
            } else if (r == 1) {
                t.cancel_task = true;
                t.cancel_value = i;
                t.handle = h;
                REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, t, nullptr));
            } else if (r == 2) {
                t.cancel_task = true;
                t.cancel_value = i;
                t.handle = h;
                REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, t,
                                                                            &turbo::TASK_OPTIONS_URGENT));
            } else {
                // do nothing;
            }
        }
        m.expected.fetch_add(expected);
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(m.sum, m.expected.load());
        TLOG_INFO("sum={} race_times={} succ_times={} fail_times={}", m.sum, m.race_times.load(),  m.succ_times.load(), m.fail_times.load());

    }

    int add2(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
        if (iter) {
            int64_t *result = (int64_t *) meta;
            *result += iter->value;
            if (iter->event) { iter->event->signal(); }
        }
        return 0;
    }

TEST_CASE_FIXTURE(ExecutionQueueTest, "not_do_iterate_at_all") {
        int64_t result = 0;
        int64_t expected_result = 0;
        turbo::ExecutionQueueId<LongIntTask> queue_id;
        turbo::ExecutionQueueOptions options;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add2, &result));
        for (int i = 0; i < 100; ++i) {
            expected_result += i;
            REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, i));
        }
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE_NE(0,  turbo::execution_queue_execute(queue_id, 0));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());
        REQUIRE_EQ(expected_result, result);
    }

    int add_with_suspend3(void *meta, turbo::TaskIterator<LongIntTask> &iter) {
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

TEST_CASE_FIXTURE(ExecutionQueueTest, "cancel_unexecuted_high_priority_task") {
        g_should_be_urgent = false;
        turbo::ExecutionQueueId<LongIntTask> queue_id = {0}; // to suppress warnings
        turbo::ExecutionQueueOptions options;
        int64_t result = 0;
        REQUIRE_EQ(0, turbo::execution_queue_start(&queue_id, &options,
                                                                  add_with_suspend3, &result));
        // Push a normal task to make the executor suspend
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, -100));
        while (!g_suspending) {
            usleep(10);
        }
        // At this point, executor is suspended by the first task. Then we put
        // a high_priority task which is going to be cancelled immediately,
        // expecting that both operations are successful.
        turbo::TaskHandle h;
        REQUIRE_EQ(0,  turbo::execution_queue_execute(
                queue_id, -100, &turbo::TASK_OPTIONS_URGENT, &h));
        REQUIRE_EQ(0, turbo::execution_queue_cancel(h));

        // Resume executor
        g_suspending = false;

        // Push a normal task
        REQUIRE_EQ(0,  turbo::execution_queue_execute(queue_id, 12345));

        // The execq should stop normally
        REQUIRE_EQ(0, turbo::execution_queue_stop(queue_id));
        REQUIRE(turbo::execution_queue_join(queue_id).ok());

        REQUIRE_EQ(12345, result);
    }
} // namespace
