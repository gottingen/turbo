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
//
// Created by jeff on 23-12-16.
//


#ifndef TURBO_FIBER_INTERNAL_SCHEDULE_GROUP_H_
#define TURBO_FIBER_INTERNAL_SCHEDULE_GROUP_H_

#ifndef NDEBUG

#include <iostream>                             // std::ostream

#endif

#include <stddef.h>                             // size_t
#include <atomic>
#include "turbo/fiber/internal/fiber_entity.h"                  // FiberEntity
#include "turbo/memory/resource_pool.h"                 // ResourcePool
#include "turbo/concurrent/work_stealing_queue.h"        // WorkStealingQueue
#include "turbo/fiber/internal/parking_lot.h"

namespace turbo::fiber_internal {

    class FiberWorker;

    class ScheduleGroup {
        friend class FiberWorker;

    public:
        ScheduleGroup();

        ~ScheduleGroup();

        // Must be called before using. `nconcurrency' is # of worker pthreads.
        int init(int nconcurrency);

        // Create a FiberWorker in this control.
        FiberWorker *create_group();

        // Steal a task from a "random" group.
        bool steal_task(fiber_id_t *tid, size_t *seed, size_t offset);

        // Tell other groups that `n' tasks was just added to caller's runqueue
        void signal_task(int num_task);

        // Stop and join worker threads in ScheduleGroup.
        void stop_and_join();

        // Get # of worker threads.
        int concurrency() const { return _concurrency.load(std::memory_order_acquire); }

        void print_rq_sizes(std::ostream &os);

        double get_cumulated_worker_time();

        int64_t get_cumulated_switch_count();

        int64_t get_cumulated_signal_count();

        // [Not thread safe] Add more worker threads.
        // Return the number of workers actually added, which may be less than |num|
        int add_workers(int num);

        // Choose one FiberWorker (randomly right now).
        // If this method is called after init(), it never returns nullptr.
        FiberWorker *choose_one_group();

    private:

        // Add/Remove a FiberWorker.
        // Returns 0 on success, -1 otherwise.
        int _add_group(FiberWorker *);

        int _destroy_group(FiberWorker *);

        static void delete_task_group(void *arg);

        static void *worker_thread(void *task_control);

        std::atomic<size_t> _ngroup;
        FiberWorker **_groups;
        std::mutex _modify_group_mutex;

        bool _stop;
        std::atomic<int> _concurrency;
        std::vector<pthread_t> _workers;

        static constexpr int PARKING_LOT_NUM = 4;
        ParkingLot _pl[PARKING_LOT_NUM];
    };

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_SCHEDULE_GROUP_H_
