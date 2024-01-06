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

#ifndef TURBO_TIMES_TIMER_THREAD_H_
#define TURBO_TIMES_TIMER_THREAD_H_

#include <vector>
#include <thread>
#include <atomic>
#include "turbo/times/clock.h"
#include "turbo/concurrent/spinlock.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/base/status.h"

namespace turbo {

    struct TimerThreadOptions {
        size_t num_buckets;
        std::string bvar_prefix;

        TimerThreadOptions();
    };

    class TimerThread {
    public:
        struct Task;

        class Bucket;

        using TaskId = uint64_t;
        const static TaskId INVALID_TASK_ID;

        TimerThread();

        ~TimerThread();

        // Start the timer thread.
        // This method should only be called once.
        // return 0 if success, errno otherwise.
        turbo::Status start(const TimerThreadOptions *options);

        // Stop the timer thread. Later schedule() will return INVALID_TASK_ID.
        void stop_and_join();

        // Schedule |fn(arg)| to run at realtime |abstime| approximately.
        // Returns: identifier of the scheduled task, INVALID_TASK_ID on error.
        [[nodiscard]] TaskId schedule(void (*fn)(void *), void *arg, const turbo::Time &abstime);

        // Prevent the task denoted by `task_id' from running. `task_id' must be
        // returned by schedule() ever.
        // Returns:
        //   ok()   -  Removed the task which does not run yet
        //  not_found_error()   -  The task does not exist.
        //   resource_busy_error   -  The task is just running.
        [[maybe_unused]] turbo::Status unschedule(TaskId task_id);

        // Get identifier of internal pthread.
        // Returns (pthread_t)0 if start() is not called yet.
        pthread_t thread_id() const { return _thread; }

    private:
        // the timer thread will run this method.
        void run();

        static void *run_this(void *arg);

        bool _started;            // whether the timer thread was started successfully.
        std::atomic<bool> _stop;

        TimerThreadOptions _options;
        Bucket *_buckets;                   // list of tasks to be run
        SpinLock _mutex;              // protect _nearest_run_time
        turbo::Time _nearest_run_time;
        // the futex for wake up timer thread. can't use _nearest_run_time because
        // it's 64-bit.
        turbo::SpinWaiter _nsignals;
        pthread_t _thread;       // all scheduled task will be run on this thread
    };

    // Get the global TimerThread which never quits.
    TimerThread *get_or_create_global_timer_thread();

    TimerThread *get_global_timer_thread();

}   // namespace turbo

#endif  // TURBO_TIMES_TIMER_THREAD_H_
