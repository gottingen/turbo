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

#ifndef TURBO_EVENT_INTERNAL_TIMER_H_
#define TURBO_EVENT_INTERNAL_TIMER_H_

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include "turbo/times/clock.h"
#include "turbo/concurrent/spinlock.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/status/status.h"
#include "turbo/event/timer.h"

namespace turbo {

    class TimerCore {
    public:

        class Bucket;

        TimerCore();

        ~TimerCore();

        // Start the timer thread.
        // This method should only be called once.
        // return 0 if success, errno otherwise.
        turbo::Status initialize(const TimerOptions *options);

        // Schedule |fn(arg)| to run at realtime |abstime| approximately.
        // Returns: identifier of the scheduled task, INVALID_TASK_ID on error.
        [[nodiscard]] std::pair<TimerId ,bool> schedule(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime);

        // Prevent the task denoted by `task_id' from running. `task_id' must be
        // returned by schedule() ever.
        // Returns:
        //   ok()   -  Removed the task which does not run yet
        //   ESTOP   -  The task does not exist.
        //   EBUSY   -  The task is just running.
        [[maybe_unused]] turbo::Status unschedule(TimerId task_id);

        turbo::Time run_timer_tasks();

        turbo::Time next_run_time() const { return _nearest_run_time; }

        void stop();

    private:

        void run_timer_tasks_once();
        TimerOptions _options;
        Bucket *_buckets;                   // list of tasks to be run
        SpinLock _mutex;              // protect _nearest_run_time
        turbo::Time _nearest_run_time;
        bool _stop{false};
    };
}   // namespace turbo

#endif  // TURBO_EVENT_INTERNAL_TIMER_H_
