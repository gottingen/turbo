// Copyright 2023 The turbo Authors.
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

#ifndef TURBO_NETWORK_THREAD_THREAD_POOL_H_
#define TURBO_NETWORK_THREAD_THREAD_POOL_H_


#include "turbo/network/thread/thread_group.h"
#include "turbo/network/thread/task_executor.h"
#include "turbo/network/thread/task_queue.h"
#include "turbo/network/util/util.h"
#include "turbo/network/util/logger.h"


namespace turbo {

class ThreadPool : public TaskExecutor {
public:
    enum class Priority {
        PRIORITY_LOWEST = 0,
        PRIORITY_LOW,
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_HIGHEST
    };

    ThreadPool(int num = 1, Priority priority = Priority::PRIORITY_HIGHEST, bool auto_run = true, bool set_affinity = true,
               const std::string &pool_name = "thread pool") {
        _thread_num = num;
        _on_setup = [pool_name, priority, set_affinity](int index) {
            std::string name = pool_name + ' ' + std::to_string(index);
            setPriority(priority);
            setThreadName(name.data());
            if (set_affinity) {
                setThreadAffinity(index % std::thread::hardware_concurrency());
            }
        };
        _logger = Logger::Instance().shared_from_this();
        if (auto_run) {
            start();
        }
    }

    ~ThreadPool() {
        shutdown();
        wait();
    }

    //把任务打入线程池并异步执行
    Task::Ptr async(TaskIn task, bool may_sync = true) override {
        if (may_sync && _thread_group.is_this_thread_in()) {
            task();
            return nullptr;
        }
        auto ret = std::make_shared<Task>(std::move(task));
        _queue.push_task(ret);
        return ret;
    }

    Task::Ptr async_first(TaskIn task, bool may_sync = true) override {
        if (may_sync && _thread_group.is_this_thread_in()) {
            task();
            return nullptr;
        }

        auto ret = std::make_shared<Task>(std::move(task));
        _queue.push_task_first(ret);
        return ret;
    }

    size_t size() {
        return _queue.size();
    }

    static bool setPriority(Priority priority = Priority::PRIORITY_NORMAL, std::thread::native_handle_type threadId = 0) {
        // set priority
#if defined(_WIN32)
        static int Priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST };
        if (priority != PRIORITY_NORMAL && SetThreadPriority(GetCurrentThread(), Priorities[priority]) == 0) {
            return false;
        }
        return true;
#else
        static int Min = sched_get_priority_min(SCHED_FIFO);
        if (Min == -1) {
            return false;
        }
        static int Max = sched_get_priority_max(SCHED_FIFO);
        if (Max == -1) {
            return false;
        }
        static int Priorities[] = {Min, Min + (Max - Min) / 4, Min + (Max - Min) / 2, Min + (Max - Min) * 3 / 4, Max};

        if (threadId == 0) {
            threadId = pthread_self();
        }
        struct sched_param params;
        params.sched_priority = Priorities[static_cast<int>(priority)];
        return pthread_setschedparam(threadId, SCHED_FIFO, &params) == 0;
#endif
    }

    void start() {
        if (_thread_num <= 0) {
            return;
        }
        size_t total = _thread_num - _thread_group.size();
        for (size_t i = 0; i < total; ++i) {
            _thread_group.create_thread([this, i]() {run(i);});
        }
    }

private:
    void run(size_t index) {
        _on_setup(index);
        Task::Ptr task;
        while (true) {
            //startSleep();
            if (!_queue.get_task(task)) {
                //空任务，退出线程
                break;
            }
            //sleepWakeUp();
            try {
                (*task)();
                task = nullptr;
            } catch (std::exception &ex) {
                ErrorL << "ThreadPool catch a exception: " << ex.what();
            }
        }
    }

    void wait() {
        _thread_group.join_all();
    }

    void shutdown() {
        _queue.push_exit(_thread_num);
    }

private:
    size_t _thread_num;
    Logger::Ptr _logger;
    thread_group _thread_group;
    TaskQueue<Task::Ptr> _queue;
    std::function<void(int)> _on_setup;
};

} /* namespace turbo */
#endif /* TURBO_NETWORK_THREAD_THREAD_POOL_H_ */