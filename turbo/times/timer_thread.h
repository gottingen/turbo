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
#include <functional>
#include "turbo/times/clock.h"
#include "turbo/concurrent/spinlock.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/status/status.h"

namespace turbo {

    struct TimerThreadOptions {
        size_t num_buckets{13};
        TimerThreadOptions() = default;
    };

    typedef std::function<void(void *)> timer_task_fn_t;

    class TimerThread {
    public:
        struct Task;

        class Bucket;

        using TaskId = uint64_t;
        constexpr static TaskId INVALID_TASK_ID = 0;

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
        [[nodiscard]] TaskId schedule(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime);

        // Prevent the task denoted by `task_id' from running. `task_id' must be
        // returned by schedule() ever.
        // Returns:
        //   ok()   -  Removed the task which does not run yet
        //   ESTOP   -  The task does not exist.
        //   EBUSY   -  The task is just running.
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

    template<typename Tag>
    struct TimerThreadInstance {

        ~TimerThreadInstance() {
            /*
            if (timer_thread) {
                delete timer_thread;
                timer_thread = nullptr;
            }*/
        }

        static TimerThreadInstance* get_instance() {
            static TimerThreadInstance instance;
            return &instance;
        }
        static turbo::Status init_timer_thread(const TimerThreadOptions *options = nullptr) {
            auto instance = get_instance();
            if(instance->timer_thread != nullptr) {
                return turbo::ok_status();
            }
            std::lock_guard<std::mutex> lock(instance->mutex);
            if (instance->timer_thread == nullptr) {
                instance->timer_thread = new(std::nothrow) TimerThread;
                if (instance->timer_thread == nullptr) {
                    //TLOG_CRITICAL("Fail to new timer_thread");
                    return turbo::make_status(kENOMEM,"Fail to new timer_thread");
                }
                const TimerThreadOptions *options_ptr = options;
                TimerThreadOptions default_options;
                if (options_ptr == nullptr) {
                    options_ptr = &default_options;
                }
                const auto rc = instance->timer_thread->start(options_ptr);
                if (!rc.ok()) {
                    //TLOG_CRITICAL("Fail to start timer_thread, {}", rc.to_string());
                    delete instance->timer_thread;
                    instance->timer_thread = nullptr;
                    return rc;
                }
            }
            return turbo::ok_status();
        }

        static TimerThread * get_timer_thread() {
            return get_instance()->timer_thread;
        }
    private:
        TimerThread *timer_thread{nullptr};
        std::mutex  mutex;
    };

    struct global_timer_thread_tag {};

    template<typename Tag>
    inline turbo::Status init_timer_thread(const TimerThreadOptions *options = nullptr) {
        return TimerThreadInstance<Tag>::init_timer_thread(options);
    }

    template<typename Tag>
    inline TimerThread * get_timer_thread() {
        return TimerThreadInstance<Tag>::get_timer_thread();
    }

    extern template turbo::Status init_timer_thread<global_timer_thread_tag>(const TimerThreadOptions *options = nullptr);
    extern template TimerThread * get_timer_thread<global_timer_thread_tag>();

    inline turbo::Status init_global_timer_thread(const TimerThreadOptions *options = nullptr) {
        return init_timer_thread<global_timer_thread_tag>(options);
    }
    inline TimerThread *get_global_timer_thread() {
        return get_timer_thread<global_timer_thread_tag>();
    }

}   // namespace turbo

#endif  // TURBO_TIMES_TIMER_THREAD_H_
