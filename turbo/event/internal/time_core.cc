// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// bthread - An M:N threading library to make applications more concurrent.


#include <queue>                           // heap functions
#include "turbo/log/logging.h"
#include "turbo/memory/resource_pool.h"
#include "turbo/event/internal/timer_core.h"
#include "turbo/platform/port.h"
#include "turbo/hash/hash.h"
#include "turbo/system/sysinfo.h"
#include "turbo/system/threading.h"
#include <utility>

namespace turbo {

    // A task contains the necessary information for running fn(arg).
    // Tasks are created in Bucket::schedule and destroyed in TimerCore::run
    struct TURBO_CACHE_LINE_ALIGNED TimerCore::Task {
        Task *next;                 // For linking tasks in a Bucket.
        turbo::Time run_time;           // run the task at this realtime
        timer_task_fn_t fn;          // the fn(arg) to run
        void *arg;
        // Current TaskId, checked against version in TimerCore::run to test
        // if this task is unscheduled.
        TaskId task_id;
        // initial_version:     not run yet
        // initial_version + 1: running
        // initial_version + 2: removed (also the version of next Task reused
        //                      this struct)
        std::atomic<uint32_t> version;

        Task() : version(2/*skip 0*/) {}

        // Run this task and delete this struct.
        // Returns true if fn(arg) did run.
        bool run_and_delete();

        // Delete this struct if this task was unscheduled.
        // Returns true on deletion.
        bool try_delete();
    };

    // TimerCore tasks are sharded into different Buckets to reduce contentions.
    class TURBO_CACHE_LINE_ALIGNED TimerCore::Bucket {
    public:
        Bucket()
                : _nearest_run_time(turbo::Time::infinite_future()), _task_head(nullptr) {
        }

        ~Bucket() {}

        struct ScheduleResult {
            TimerCore::TaskId task_id;
            bool earlier;
        };

        // Schedule a task into this bucket.
        // Returns the TaskId and if it has the nearest run time.
        ScheduleResult schedule(timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime);

        // Pull all scheduled tasks.
        // This function is called in timer thread.
        Task *consume_tasks();

    private:
        turbo::SpinLock _mutex;
        turbo::Time _nearest_run_time;
        Task *_task_head;
    };

    inline TimerCore::TaskId make_task_id(
            turbo::ResourceId<TimerCore::Task> slot, uint32_t version) {
        return TimerCore::TaskId((((uint64_t) version) << 32) | slot.value);
    }

    inline
    turbo::ResourceId<TimerCore::Task> slot_of_task_id(TimerCore::TaskId id) {
        turbo::ResourceId<TimerCore::Task> slot = {(id & 0xFFFFFFFFul)};
        return slot;
    }

    inline uint32_t version_of_task_id(TimerCore::TaskId id) {
        return (uint32_t) (id >> 32);
    }

    inline bool task_greater(const TimerCore::Task *a, const TimerCore::Task *b) {
        return a->run_time > b->run_time;
    }

    TimerCore::TimerCore() :_buckets(nullptr), _nearest_run_time(turbo::Time::infinite_future()){
    }

    TimerCore::~TimerCore() {
        delete[] _buckets;
        _buckets = nullptr;
    }

    turbo::Status TimerCore::initialize(const TimerThreadOptions *options_in) {
        if (options_in) {
            _options = *options_in;
        }
        if (_options.num_buckets == 0) {
            TLOG_ERROR("num_buckets can't be 0");
            return turbo::make_status(kEINVAL);
        }
        if (_options.num_buckets > 1024) {
            TLOG_ERROR("num_buckets={} is too big", _options.num_buckets);
            return turbo::make_status(kEINVAL);
        }
        _buckets = new(std::nothrow) Bucket[_options.num_buckets];
        if (nullptr == _buckets) {
            TLOG_ERROR("Fail to new _buckets");
            return make_status(kENOMEM);
        }
        return turbo::ok_status();
    }

    TimerCore::Task *TimerCore::Bucket::consume_tasks() {
        Task *head = nullptr;
        if (_task_head) { // NOTE: schedule() and consume_tasks() are sequenced
            // by TimerCore._nearest_run_time and fenced by TimerCore._mutex.
            // We can avoid touching the mutex and related cacheline when the
            // bucket is actually empty.
            turbo::SpinLockHolder l(&_mutex);
            if (_task_head) {
                head = _task_head;
                _task_head = nullptr;
                _nearest_run_time = turbo::Time::infinite_future();
            }
        }
        return head;
    }

    TimerCore::Bucket::ScheduleResult
    TimerCore::Bucket::schedule(timer_task_fn_t &&fn, void *arg,
                                  const turbo::Time &abstime) {
        turbo::ResourceId<Task> slot_id;
        Task *task = turbo::get_resource<Task>(&slot_id);
        if (task == nullptr) {
            ScheduleResult result = {INVALID_TASK_ID, false};
            return result;
        }
        task->next = nullptr;
        task->fn = std::move(fn);
        task->arg = arg;
        task->run_time = abstime;
        uint32_t version = task->version.load(std::memory_order_relaxed);
        if (version == 0) {  // skip 0.
            task->version.fetch_add(2, std::memory_order_relaxed);
            version = 2;
        }
        const TaskId id = make_task_id(slot_id, version);
        task->task_id = id;
        bool earlier = false;
        {
            turbo::SpinLockHolder l(&_mutex);
            task->next = _task_head;
            _task_head = task;
            if (task->run_time < _nearest_run_time) {
                _nearest_run_time = task->run_time;
                earlier = true;
            }
        }
        ScheduleResult result = {id, earlier};
        return result;
    }

    std::pair<TimerCore::TaskId,bool> TimerCore::schedule(
            timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime) {
        // Hashing by pthread id is better for cache locality.
        const Bucket::ScheduleResult result =
                _buckets[turbo::hash_mixer8(turbo::thread_numeric_id()) % _options.num_buckets].schedule(std::move(fn), arg,
                                                                                                         abstime);
        bool earlier = false;
        if (result.earlier) {
            const auto run_time = abstime;
            {
                turbo::SpinLockHolder l(&_mutex);
                if (run_time < _nearest_run_time) {
                    _nearest_run_time = run_time;
                    earlier = true;
                }
            }
        }
        return {result.task_id, earlier};
    }

    // Notice that we don't recycle the Task in this function, let TimerCore::run
    // do it. The side effect is that we may allocate many unscheduled tasks before
    // TimerCore wakes up. The number is approximately qps * timeout_s. Under the
    // precondition that ResourcePool<Task> caches 128K for each thread, with some
    // further calculations, we can conclude that in a RPC scenario:
    //   when timeout / latency < 2730 (128K / sizeof(Task))
    // unscheduled tasks do not occupy additional memory. 2730 is a large ratio
    // between timeout and latency in most RPC scenarios, this is why we don't
    // try to reuse tasks right now inside unschedule() with more complicated code.
    turbo::Status TimerCore::unschedule(TaskId task_id) {
        const turbo::ResourceId<Task> slot_id = slot_of_task_id(task_id);
        Task *const task = turbo::address_resource(slot_id);
        if (task == nullptr) {
            TLOG_ERROR("Invalid task_id={}", task_id);
            return turbo::make_status(kEINVAL);
        }
        const uint32_t id_version = version_of_task_id(task_id);
        uint32_t expected_version = id_version;
        // This CAS is rarely contended, should be fast.
        // The acquire fence is paired with release fence in Task::run_and_delete
        // to make sure that we see all changes brought by fn(arg).
        if (task->version.compare_exchange_strong(expected_version, id_version + 2,std::memory_order_acquire)) {
            return turbo::ok_status();
        }
        return (expected_version == id_version + 1) ? turbo::make_status(kEBUSY) : turbo::make_status(kESTOP);
    }

    bool TimerCore::Task::run_and_delete() {
        const uint32_t id_version = version_of_task_id(task_id);
        uint32_t expected_version = id_version;
        // This CAS is rarely contended, should be fast.
        if (version.compare_exchange_strong(
                expected_version, id_version + 1, std::memory_order_relaxed)) {
            fn(arg);
            // The release fence is paired with acquire fence in
            // TimerCore::unschedule to make changes of fn(arg) visible.
            version.store(id_version + 2, std::memory_order_release);
            turbo::return_resource(slot_of_task_id(task_id));
            return true;
        } else if (expected_version == id_version + 2) {
            // already unscheduled.
            turbo::return_resource(slot_of_task_id(task_id));
            return false;
        } else {
            // Impossible.
            TLOG_ERROR("Invalid version={}, expecting {}", expected_version, id_version + 2);
            return false;
        }
    }

    bool TimerCore::Task::try_delete() {
        const uint32_t id_version = version_of_task_id(task_id);
        if (version.load(std::memory_order_relaxed) != id_version) {
            TLOG_CHECK_EQ(version.load(std::memory_order_relaxed), id_version + 2);
            turbo::return_resource(slot_of_task_id(task_id));
            return true;
        }
        return false;
    }

    template<typename T>
    static T deref_value(void *arg) {
        return *(T *) arg;
    }


    void TimerCore::run_timer_tasks() {
        TLOG_INFO("Started TimerCore={}", pthread_self());

        // min heap of tasks (ordered by run_time)
        std::vector<Task *> tasks;
        tasks.reserve(4096);

        while (true) {
            // Clear _nearest_run_time before consuming tasks from buckets.
            // This helps us to be aware of earliest task of the new tasks before we
            // would run the consumed tasks.
            {
                turbo::SpinLockHolder l(&_mutex);
                _nearest_run_time = turbo::Time::infinite_future();
            }

            // Pull tasks from buckets.
            for (size_t i = 0; i < _options.num_buckets; ++i) {
                Bucket &bucket = _buckets[i];
                for (Task *p = bucket.consume_tasks(); p != nullptr;) {
                    // p->next should be kept first
                    // in case of the deletion of Task p which is unscheduled
                    Task *next_task = p->next;

                    if (!p->try_delete()) { // remove the task if it's unscheduled
                        tasks.push_back(p);
                        std::push_heap(tasks.begin(), tasks.end(), task_greater);
                    }
                    p = next_task;
                }
            }

            bool pull_again = false;
            while (!tasks.empty()) {
                Task *task1 = tasks[0];  // the about-to-run task
                if (turbo::time_now() < task1->run_time) {  // not ready yet.
                    break;
                }
                // Each time before we run the earliest task (that we think),
                // check the globally shared _nearest_run_time. If a task earlier
                // than task1 was scheduled during pulling from buckets, we'll
                // know. In RPC scenarios, _nearest_run_time is not often changed by
                // threads because the task needs to be the earliest in its bucket,
                // since run_time of scheduled tasks are often in ascending order,
                // most tasks are unlikely to be "earliest". (If run_time of tasks
                // are in descending orders, all tasks are "earliest" after every
                // insertion, and they'll grab _mutex and change _nearest_run_time
                // frequently, fortunately this is not true at most of time).
                {
                    turbo::SpinLockHolder l(&_mutex);
                    if (task1->run_time > _nearest_run_time) {
                        // a task is earlier than task1. We need to check buckets.
                        pull_again = true;
                        break;
                    }
                }
                std::pop_heap(tasks.begin(), tasks.end(), task_greater);
                tasks.pop_back();
                task1->run_and_delete();
            }
            if (pull_again) {
                //TLOG_INFO("pull again, tasks={}", tasks.size());
                continue;
            }
            return;
        }
    }

}  // end namespace turbo
