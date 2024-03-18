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

#include <memory>
#include "turbo/network/thread/task_executor.h"
#include "turbo/network/poller/event_poller.h"
#include "turbo/network/util/notice_center.h"
#include "turbo/network/util/time_ticker.h"
#include "turbo/network/util/once_token.h"

using namespace std;

namespace turbo {

ThreadLoadCounter::ThreadLoadCounter(uint64_t max_size, uint64_t max_usec) {
    _last_sleep_time = _last_wake_time = getCurrentMicrosecond();
    _max_size = max_size;
    _max_usec = max_usec;
}

void ThreadLoadCounter::startSleep() {
    lock_guard<mutex> lck(_mtx);
    _sleeping = true;
    auto current_time = getCurrentMicrosecond();
    auto run_time = current_time - _last_wake_time;
    _last_sleep_time = current_time;
    _time_list.emplace_back(run_time, false);
    if (_time_list.size() > _max_size) {
        _time_list.pop_front();
    }
}

void ThreadLoadCounter::sleepWakeUp() {
    lock_guard<mutex> lck(_mtx);
    _sleeping = false;
    auto current_time = getCurrentMicrosecond();
    auto sleep_time = current_time - _last_sleep_time;
    _last_wake_time = current_time;
    _time_list.emplace_back(sleep_time, true);
    if (_time_list.size() > _max_size) {
        _time_list.pop_front();
    }
}

int ThreadLoadCounter::load() {
    lock_guard<mutex> lck(_mtx);
    uint64_t totalSleepTime = 0;
    uint64_t totalRunTime = 0;
    _time_list.for_each([&](const TimeRecord &rcd) {
        if (rcd._sleep) {
            totalSleepTime += rcd._time;
        } else {
            totalRunTime += rcd._time;
        }
    });

    if (_sleeping) {
        totalSleepTime += (getCurrentMicrosecond() - _last_sleep_time);
    } else {
        totalRunTime += (getCurrentMicrosecond() - _last_wake_time);
    }

    uint64_t totalTime = totalRunTime + totalSleepTime;
    while ((_time_list.size() != 0) && (totalTime > _max_usec || _time_list.size() > _max_size)) {
        TimeRecord &rcd = _time_list.front();
        if (rcd._sleep) {
            totalSleepTime -= rcd._time;
        } else {
            totalRunTime -= rcd._time;
        }
        totalTime -= rcd._time;
        _time_list.pop_front();
    }
    if (totalTime == 0) {
        return 0;
    }
    return (int) (totalRunTime * 100 / totalTime);
}

////////////////////////////////////////////////////////////////////////////

Task::Ptr TaskExecutorInterface::async_first(TaskIn task, bool may_sync) {
    return async(std::move(task), may_sync);
}

void TaskExecutorInterface::sync(const TaskIn &task) {
    semaphore sem;
    auto ret = async([&]() {
        onceToken token(nullptr, [&]() {
            //通过RAII原理防止抛异常导致不执行这句代码
            sem.post();
        });
        task();
    });
    if (ret && *ret) {
        sem.wait();
    }
}

void TaskExecutorInterface::sync_first(const TaskIn &task) {
    semaphore sem;
    auto ret = async_first([&]() {
        onceToken token(nullptr, [&]() {
            //通过RAII原理防止抛异常导致不执行这句代码
            sem.post();
        });
        task();
    });
    if (ret && *ret) {
        sem.wait();
    }
}

//////////////////////////////////////////////////////////////////

TaskExecutor::TaskExecutor(uint64_t max_size, uint64_t max_usec) : ThreadLoadCounter(max_size, max_usec) {}

//////////////////////////////////////////////////////////////////

TaskExecutor::Ptr TaskExecutorGetterImp::getExecutor() {
    auto thread_pos = _thread_pos;
    if (thread_pos >= _threads.size()) {
        thread_pos = 0;
    }

    TaskExecutor::Ptr executor_min_load = _threads[thread_pos];
    auto min_load = executor_min_load->load();

    for (size_t i = 0; i < _threads.size(); ++i) {
        ++thread_pos;
        if (thread_pos >= _threads.size()) {
            thread_pos = 0;
        }

        auto th = _threads[thread_pos];
        auto load = th->load();

        if (load < min_load) {
            min_load = load;
            executor_min_load = th;
        }
        if (min_load == 0) {
            break;
        }
    }
    _thread_pos = thread_pos;
    return executor_min_load;
}

vector<int> TaskExecutorGetterImp::getExecutorLoad() {
    vector<int> vec(_threads.size());
    int i = 0;
    for (auto &executor : _threads) {
        vec[i++] = executor->load();
    }
    return vec;
}

void TaskExecutorGetterImp::getExecutorDelay(const function<void(const vector<int> &)> &callback) {
    std::shared_ptr<vector<int> > delay_vec = std::make_shared<vector<int>>(_threads.size());
    shared_ptr<void> finished(nullptr, [callback, delay_vec](void *) {
        //此析构回调触发时，说明已执行完毕所有async任务
        callback((*delay_vec));
    });
    int index = 0;
    for (auto &th : _threads) {
        std::shared_ptr<Ticker> delay_ticker = std::make_shared<Ticker>();
        th->async([finished, delay_vec, index, delay_ticker]() {
            (*delay_vec)[index] = (int) delay_ticker->elapsedTime();
        }, false);
        ++index;
    }
}

void TaskExecutorGetterImp::for_each(const function<void(const TaskExecutor::Ptr &)> &cb) {
    for (auto &th : _threads) {
        cb(th);
    }
}

size_t TaskExecutorGetterImp::getExecutorSize() const {
    return _threads.size();
}

size_t TaskExecutorGetterImp::addPoller(const string &name, size_t size, int priority, bool register_thread, bool enable_cpu_affinity) {
    auto cpus = thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    for (size_t i = 0; i < size; ++i) {
        auto full_name = name + " " + to_string(i);
        auto cpu_index = i % cpus;
        EventPoller::Ptr poller(new EventPoller(full_name));
        poller->runLoop(false, register_thread);
        poller->async([cpu_index, full_name, priority, enable_cpu_affinity]() {
            // 设置线程优先级
            ThreadPool::setPriority((ThreadPool::Priority)priority);
            // 设置线程名
            setThreadName(full_name.data());
            // 设置cpu亲和性
            if (enable_cpu_affinity) {
                //todo 由于FFmpeg软件转码，所以不能设置cpu亲和性，否则所有转码线程都运行在一个cpu核心上，导致转性能低下
                //setThreadAffinity(cpu_index);
            }
        });
        _threads.emplace_back(std::move(poller));
    }
    return size;
}

} //namespace turbo
