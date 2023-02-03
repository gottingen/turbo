// Copyright 2023 The Turbo Authors.
//
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

#ifndef TURBO_WORKFLOW_CORE_SEMAPHORE_H_
#define TURBO_WORKFLOW_CORE_SEMAPHORE_H_

#include <vector>
#include <mutex>

#include "turbo/workflow/core/declarations.h"


namespace turbo {
TURBO_NAMESPACE_BEGIN
// ----------------------------------------------------------------------------
// Semaphore
// ----------------------------------------------------------------------------

/**
@class Semaphore

@brief class to create a semophore object for building a concurrency constraint

A semaphore creates a constraint that limits the maximum concurrency,
i.e., the number of workers, in a set of tasks.
You can let a task acquire/release one or multiple semaphores before/after
executing its work.
A task can acquire and release a semaphore,
or just acquire or just release it.
A turbo::Semaphore object starts with an initial count.
As long as that count is above 0, tasks can acquire the semaphore and do
their work.
If the count is 0 or less, a task trying to acquire the semaphore will not run
but goes to a waiting list of that semaphore.
When the semaphore is released by another task,
it reschedules all tasks on that waiting list.

@code{.cpp}
turbo::Executor executor(8);   // create an executor of 8 workers
turbo::Workflow workflow;

turbo::Semaphore semaphore(1); // create a semaphore with initial count 1

std::vector<turbo::Task> tasks {
  workflow.emplace([](){ std::cout << "A" << std::endl; }),
  workflow.emplace([](){ std::cout << "B" << std::endl; }),
  workflow.emplace([](){ std::cout << "C" << std::endl; }),
  workflow.emplace([](){ std::cout << "D" << std::endl; }),
  workflow.emplace([](){ std::cout << "E" << std::endl; })
};

for(auto & task : tasks) {  // each task acquires and release the semaphore
  task.acquire(semaphore);
  task.release(semaphore);
}

executor.run(workflow).wait();
@endcode

    The above example creates five tasks with no dependencies between them.
    Under normal circumstances, the five tasks would be executed concurrently.
    However, this example has a semaphore with initial count 1,
    and all tasks need to acquire that semaphore before running and release that
    semaphore after they are done.
    This arrangement limits the number of concurrently running tasks to only one.

    */
    class Semaphore {

        friend class Node;

    public:

        /**
        @brief constructs a semaphore with the given counter

        A semaphore creates a constraint that limits the maximum concurrency,
        i.e., the number of workers, in a set of tasks.

        @code{.cpp}
        turbo::Semaphore semaphore(4);  // concurrency constraint of 4 workers
        @endcode
        */
        explicit Semaphore(size_t max_workers);

        /**
        @brief queries the counter value (not thread-safe during the run)
        */
        size_t count() const;

    private:

        std::mutex _mtx;

        size_t _counter;

        std::vector<Node *> _waiters;

        bool _try_acquire_or_wait(Node *);

        std::vector<Node *> _release();
    };

    inline Semaphore::Semaphore(size_t max_workers) :
            _counter(max_workers) {
    }

    inline bool Semaphore::_try_acquire_or_wait(Node *me) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_counter > 0) {
            --_counter;
            return true;
        } else {
            _waiters.push_back(me);
            return false;
        }
    }

    inline std::vector<Node *> Semaphore::_release() {
        std::lock_guard<std::mutex> lock(_mtx);
        ++_counter;
        std::vector<Node *> r{std::move(_waiters)};
        return r;
    }

    inline size_t Semaphore::count() const {
        return _counter;
    }
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_WORKFLOW_CORE_SEMAPHORE_H_
