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

#include <queue>
#include <utility>// heap functions
#include <vector>
#include "turbo/log/logging.h"
#include "turbo/memory/resource_pool.h"
#include "turbo/event/internal/timer_core.h"
#include "turbo/platform/port.h"
#include "turbo/hash/hash.h"
#include "turbo/system/sysinfo.h"
#include "turbo/system/threading.h"
#include "turbo/container/btree_set.h"


namespace turbo {

    std::atomic<size_t> g_sequence{1};

    inline bool task_greater(const OnceTask *a, const OnceTask *b) {
        return a->run_time > b->run_time;
    }

    struct once_task_less {
        bool operator()(const OnceTask *a, const OnceTask *b) const {
            return a->run_time < b->run_time;
        }
    };

    struct once_task_less_and_seq {
        bool operator()(const OnceTask *a, const OnceTask *b) const {
            return a->sequence < b->sequence ? true : a->run_time < b->run_time;
        }
    };

    typedef turbo::btree_multiset<OnceTask *, once_task_less_and_seq> TaskSeqSet;

    // TimerCore tasks are sharded into different Buckets to reduce contentions.
    class TURBO_CACHE_LINE_ALIGNED TimerCore::Bucket {
    public:
        Bucket() = default;

        ~Bucket() {}

        std::pair<TimerId, bool> schedule(timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime);

        // Pull all scheduled tasks.
        // This function is called in timer thread.
        void consume_tasks_to(TaskSeqSet &set, uint64_t nanos_delta);

        friend class TimerCore;

    private:
        typedef turbo::btree_multiset<OnceTask *, once_task_less> TaskSet;
        turbo::SpinLock _mutex;
        turbo::Time _nearest_run_time{turbo::Time::infinite_future()};
        TaskSet _task_set;
    };

    TimerCore::TimerCore() : _buckets(nullptr), _nearest_run_time(turbo::Time::infinite_future()) {
    }

    TimerCore::~TimerCore() {
        delete[] _buckets;
        _buckets = nullptr;
    }

    turbo::Status TimerCore::initialize(const TimerOptions *options_in) {
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

    void TimerCore::Bucket::consume_tasks_to(TaskSeqSet &tasks, uint64_t nanos_delta) {
        turbo::Time now = turbo::Time::time_now() + turbo::Duration::nanoseconds(nanos_delta);
        turbo::SpinLockHolder l(&_mutex);
        auto it = _task_set.begin();
        while (it != _task_set.end()) {
            if ((*it)->run_time > now) {
                break;
            }
            tasks.insert(*it);
            it = _task_set.erase(it);
        }
        if (it != _task_set.end()) {
            _nearest_run_time = (*it)->run_time;
        } else {
            _nearest_run_time = turbo::Time::infinite_future();
        }

    }

    std::pair<TimerId, bool>
    TimerCore::Bucket::schedule(timer_task_fn_t &&fn, void *arg,
                                const turbo::Time &abstime) {
        turbo::ResourceId<OnceTask> slot_id;
        auto *task = turbo::get_resource<OnceTask>(&slot_id);
        if (task == nullptr) {
            std::pair result = {INVALID_TIMER_ID, false};
            return result;
        }

        task->fn = std::move(fn);
        task->arg = arg;
        task->run_time = abstime;
        task->sequence = g_sequence.fetch_add(1, std::memory_order_relaxed);
        uint32_t version = task->version.load(std::memory_order_relaxed);
        if (version == 0) {  // skip 0.
            task->version.fetch_add(2, std::memory_order_relaxed);
            version = 2;
        }
        const TimerId id = turbo::make_resource_id<OnceTask>(version, slot_id);
        task->task_id = id;
        bool earlier = false;
        {
            turbo::SpinLockHolder l(&_mutex);
            auto it = _task_set.begin();
            if (it == _task_set.end() || (*it)->run_time > abstime) {
                earlier = true;
                _nearest_run_time = abstime;
            }
            _task_set.insert(task);
        }
        std::pair result = {id, earlier};
        return result;
    }

    std::pair<TimerId, bool> TimerCore::schedule(
            timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime) {
        const auto result = _buckets[turbo::hash_mixer8(turbo::thread_numeric_id()) % _options.num_buckets].schedule(
                std::move(fn), arg, abstime);
        bool earlier = false;
        if (result.second) {
            const auto run_time = abstime;
            {
                turbo::SpinLockHolder l(&_mutex);
                if (run_time < _nearest_run_time) {
                    _nearest_run_time = run_time;
                    earlier = true;
                }
            }
        }
        return {result.first, earlier};
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
    turbo::Status TimerCore::unschedule(TimerId task_id) {
        const turbo::ResourceId<OnceTask> slot_id = turbo::get_resource_id<OnceTask>(task_id);
        auto const task = turbo::address_resource(slot_id);
        if (task == nullptr) {
            TLOG_ERROR("Invalid task_id={}", task_id);
            return turbo::make_status(kEINVAL);
        }
        return task->cancel();
    }

    void TimerCore::stop() {
        _stop = true;
    }

    turbo::Time TimerCore::run_timer_tasks() {
        turbo::Time next_run_time = turbo::Time::infinite_future();
        do {
            run_timer_tasks_once();

            for (size_t i = 0; i < _options.num_buckets; ++i) {
                Bucket &bucket = _buckets[i];
                turbo::SpinLockHolder l(&bucket._mutex);
                if (next_run_time > bucket._nearest_run_time) {
                    next_run_time = bucket._nearest_run_time;
                }
            }
        } while (!_stop && next_run_time > _nearest_run_time);
        _nearest_run_time = next_run_time;
        return next_run_time
;    }

    void TimerCore::run_timer_tasks_once() {

        TaskSeqSet tasks;
        for (size_t i = 0; i < _options.num_buckets; ++i) {
            Bucket &bucket = _buckets[i];
            bucket.consume_tasks_to(tasks, _options.nano_delta);
        }
        for (auto &task: tasks) {
            if (!task->try_delete()) {
                task->run_and_delete();
            }
        }
    }

}  // end namespace turbo
