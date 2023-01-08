

#ifndef FLARE_FIBER_INTERNAL_SCHEDULE_GROUP_H_
#define FLARE_FIBER_INTERNAL_SCHEDULE_GROUP_H_

#ifndef NDEBUG

#include <iostream>                             // std::ostream

#endif

#include <stddef.h>                             // size_t
#include "flare/base/static_atomic.h"                     // std::atomic
#include "flare/metrics/all.h"                          // flare::status_gauge
#include "flare/fiber/internal/fiber_entity.h"                  // fiber_entity
#include "flare/memory/resource_pool.h"                 // ResourcePool
#include "flare/fiber/internal/work_stealing_queue.h"        // WorkStealingQueue
#include "flare/fiber/internal/parking_lot.h"

namespace flare::fiber_internal {

    class fiber_worker;

// Control all task groups
    class schedule_group {
        friend class fiber_worker;

    public:
        schedule_group();

        ~schedule_group();

        // Must be called before using. `nconcurrency' is # of worker pthreads.
        int init(int nconcurrency);

        // Create a fiber_worker in this control.
        fiber_worker *create_group();

        // Steal a task from a "random" group.
        bool steal_task(fiber_id_t *tid, size_t *seed, size_t offset);

        // Tell other groups that `n' tasks was just added to caller's runqueue
        void signal_task(int num_task);

        // Stop and join worker threads in schedule_group.
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

        // Choose one fiber_worker (randomly right now).
        // If this method is called after init(), it never returns nullptr.
        fiber_worker *choose_one_group();

    private:

        // Add/Remove a fiber_worker.
        // Returns 0 on success, -1 otherwise.
        int _add_group(fiber_worker *);

        int _destroy_group(fiber_worker *);

        static void delete_task_group(void *arg);

        static void *worker_thread(void *task_control);

        flare::LatencyRecorder &exposed_pending_time();

        flare::LatencyRecorder *create_exposed_pending_time();

        std::atomic<size_t> _ngroup;
        fiber_worker **_groups;
        std::mutex _modify_group_mutex;

        bool _stop;
        std::atomic<int> _concurrency;
        std::vector<pthread_t> _workers;

        flare::gauge<int64_t> _nworkers;
        std::mutex _pending_time_mutex;
        std::atomic<flare::LatencyRecorder *> _pending_time;
        flare::status_gauge<double> _cumulated_worker_time;
        flare::per_second<flare::status_gauge<double> > _worker_usage_second;
        flare::status_gauge<int64_t> _cumulated_switch_count;
        flare::per_second<flare::status_gauge<int64_t> > _switch_per_second;
        flare::status_gauge<int64_t> _cumulated_signal_count;
        flare::per_second<flare::status_gauge<int64_t> > _signal_per_second;
        flare::status_gauge<std::string> _status;
        flare::gauge<int64_t> _nfibers;

        static const int PARKING_LOT_NUM = 4;
        ParkingLot _pl[PARKING_LOT_NUM];
    };

    inline flare::LatencyRecorder &schedule_group::exposed_pending_time() {
        flare::LatencyRecorder *pt = _pending_time.load(std::memory_order_consume);
        if (!pt) {
            pt = create_exposed_pending_time();
        }
        return *pt;
    }

}  // namespace flare::fiber_internal

#endif  // FLARE_FIBER_INTERNAL_SCHEDULE_GROUP_H_
