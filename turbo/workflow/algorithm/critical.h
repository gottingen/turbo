#pragma once

#include "turbo/workflow/core/task.h"

/**
@file critical.hpp
@brief critical include file
*/

namespace turbo {

// ----------------------------------------------------------------------------
// CriticalSection
// ----------------------------------------------------------------------------

/**
@class CriticalSection

@brief class to create a critical region of limited workers to run tasks

turbo::CriticalSection is a warpper over turbo::Semaphore and is specialized for
limiting the maximum concurrency over a set of tasks.
A critical section starts with an initial count representing that limit.
When a task is added to the critical section,
the task acquires and releases the semaphore internal to the critical section.
This design avoids explicit call of turbo::Task::acquire and turbo::Task::release.
The following example creates a critical section of one worker and adds
the five tasks to the critical section.

@code{.cpp}
turbo::Executor executor(8);   // create an executor of 8 workers
turbo::Workflow workflow;

// create a critical section of 1 worker
turbo::CriticalSection critical_section(1);

turbo::Task A = workflow.emplace([](){ std::cout << "A" << std::endl; });
turbo::Task B = workflow.emplace([](){ std::cout << "B" << std::endl; });
turbo::Task C = workflow.emplace([](){ std::cout << "C" << std::endl; });
turbo::Task D = workflow.emplace([](){ std::cout << "D" << std::endl; });
turbo::Task E = workflow.emplace([](){ std::cout << "E" << std::endl; });

critical_section.add(A, B, C, D, E);

executor.run(workflow).wait();
@endcode

*/
    class CriticalSection : public Semaphore {

    public:

        /**
        @brief constructs a critical region of a limited number of workers
        */
        explicit CriticalSection(size_t max_workers = 1);

        /**
        @brief adds a task into the critical region
        */
        template<typename... Tasks>
        void add(Tasks...tasks);
    };

    inline CriticalSection::CriticalSection(size_t max_workers) :
            Semaphore{max_workers} {
    }

    template<typename... Tasks>
    void CriticalSection::add(Tasks... tasks) {
        (tasks.acquire(*this), ...);
        (tasks.release(*this), ...);
    }


}  // end of namespace turbo. ---------------------------------------------------


