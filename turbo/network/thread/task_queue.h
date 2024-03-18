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

#ifndef TURBO_NETWORK_THREAD_TASK_QUEUE_H_
#define TURBO_NETWORK_THREAD_TASK_QUEUE_H_

#include <mutex>
#include "turbo/network/util/list.h"
#include "semaphore.h"

namespace turbo {

//实现了一个基于函数对象的任务列队，该列队是线程安全的，任务列队任务数由信号量控制
template<typename T>
class TaskQueue {
public:
    //打入任务至列队
    template<typename C>
    void push_task(C &&task_func) {
        {
            std::lock_guard<decltype(_mutex)> lock(_mutex);
            _queue.emplace_back(std::forward<C>(task_func));
        }
        _sem.post();
    }

    template<typename C>
    void push_task_first(C &&task_func) {
        {
            std::lock_guard<decltype(_mutex)> lock(_mutex);
            _queue.emplace_front(std::forward<C>(task_func));
        }
        _sem.post();
    }

    //清空任务列队
    void push_exit(size_t n) {
        _sem.post(n);
    }

    //从列队获取一个任务，由执行线程执行
    bool get_task(T &tsk) {
        _sem.wait();
        std::lock_guard<decltype(_mutex)> lock(_mutex);
        if (_queue.empty()) {
            return false;
        }
        tsk = std::move(_queue.front());
        _queue.pop_front();
        return true;
    }

    size_t size() const {
        std::lock_guard<decltype(_mutex)> lock(_mutex);
        return _queue.size();
    }

private:
    List <T> _queue;
    // mutable是因为要在const成员函数中修改成员变量，对mutex加锁和解锁
    mutable std::mutex _mutex;
    semaphore _sem;
};

} // namespace turbo

#endif //TURBO_NETWORK_THREAD_TASK_QUEUE_H_