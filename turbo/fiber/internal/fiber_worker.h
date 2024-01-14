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

#ifndef TURBO_FIBER_INTERNAL_FIBER_WORKER_H_
#define TURBO_FIBER_INTERNAL_FIBER_WORKER_H_

#include "turbo/times/time.h"                             // cpuwide_time_ns
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_entity.h"
#include "turbo/concurrent/work_stealing_queue.h"           // WorkStealingQueue
#include "turbo/fiber/internal/remote_task_queue.h"             // RemoteTaskQueue
#include "turbo/memory/resource_pool.h"                    // ResourceId
#include "turbo/fiber/internal/parking_lot.h"

namespace turbo::fiber_internal {

    // For exiting a fiber.
    class ExitException : public std::exception {
    public:
        explicit ExitException(void *value) : _value(value) {}

        ~ExitException() throw() {}

        const char *what() const throw() override {
            return "ExitException";
        }

        void *value() const {
            return _value;
        }

    private:
        void *_value;
    };

    // Thread-local group of tasks.
    // Notice that most methods involving context switching are static otherwise
    // pointer `this' may change after wakeup. The **pg parameters in following
    // function are updated before returning.
    class FiberWorker {
    public:
        // Create task `fn(arg)' with attributes `attr' in FiberWorker *pg and put
        // the identifier into `tid'. Switch to the new task and schedule old task
        // to run.
        // Return 0 on success, errno otherwise.
        static turbo::Status start_foreground(FiberWorker **pg,
                                    fiber_id_t *__restrict tid,
                                    const FiberAttribute *__restrict attr,
                                    std::function<void*(void*)> && fn,
                                    void *__restrict arg);

        // Create task `fn(arg)' with attributes `attr' in this FiberWorker, put the
        // identifier into `tid'. Schedule the new thread to run.
        //   Called from worker: start_background<false>
        //   Called from non-worker: start_background<true>
        // Return 0 on success, errno otherwise.
        template<bool REMOTE>
        turbo::Status start_background(fiber_id_t *__restrict tid,
                             const FiberAttribute *__restrict attr,
                             std::function<void*(void*)> && fn,
                             void *__restrict arg);

        // Suspend caller and run next fiber in FiberWorker *pg.
        static void sched(FiberWorker **pg);

        static void ending_sched(FiberWorker **pg);

        // Suspend caller and run fiber `next_tid' in FiberWorker *pg.
        // Purpose of this function is to avoid pushing `next_tid' to _rq and
        // then being popped by sched(pg), which is not necessary.
        static void sched_to(FiberWorker **pg, FiberEntity *next_meta);

        static void sched_to(FiberWorker **pg, fiber_id_t next_tid);

        static void exchange(FiberWorker **pg, fiber_id_t next_tid);

        // The callback will be run in the beginning of next-run fiber.
        // Can't be called by current fiber directly because it often needs
        // the target to be suspended already.
        typedef void (*RemainedFn)(void *);

        void set_remained(RemainedFn cb, void *arg) {
            _last_context_remained = cb;
            _last_context_remained_arg = arg;
        }

        // Suspend caller for at least |timeout_us| microseconds.
        // If |timeout_us| is 0, this function does nothing.
        // If |group| is nullptr or current thread is non-fiber, call usleep(3)
        // instead. This function does not create thread-local FiberWorker.
        // Returns: 0 on success, -1 otherwise and errno is set.
        static int usleep(FiberWorker **pg, uint64_t timeout_us);

        static turbo::Status sleep(FiberWorker **pg, const turbo::Time & deadline);

        static turbo::Status sleep(FiberWorker **pg, const turbo::Duration & span);

        // Suspend caller and run another fiber. When the caller will resume
        // is undefined.
        static void yield(FiberWorker **pg);

        // Suspend caller until fiber `tid' terminates.
        static turbo::Status join(fiber_id_t tid, void **return_value);

        // Returns true iff the fiber `tid' still exists. Notice that it is
        // just the result at this very moment which may change soon.
        // Don't use this function unless you have to. Never write code like this:
        //    if (exists(tid)) {
        //        Wait for events of the thread.   // Racy, may block indefinitely.
        //    }
        static bool exists(fiber_id_t tid);

        // Put attribute associated with `tid' into `*attr'.
        // Returns 0 on success, -1 otherwise and errno is set.
        static int get_attr(fiber_id_t tid, FiberAttribute *attr);

        // Get/set FiberEntity.stop of the tid.
        static void set_stopped(fiber_id_t tid);

        static bool is_stopped(fiber_id_t tid);

        // The fiber running run_main_task();
        fiber_id_t main_tid() const { return _main_tid; }

        FiberStatistics main_stat() const;

        // Routine of the main task which should be called from a dedicated pthread.
        void run_main_task();

        // current_task is a function in macOS 10.0+
#ifdef current_task
#undef current_task
#endif

        // Meta/Identifier of current task in this group.
        FiberEntity *current_task() const { return _cur_meta; }

        fiber_id_t current_fid() const { return _cur_meta->tid; }

        // Uptime of current task in nanoseconds.
        int64_t current_uptime_ns() const { return turbo::get_current_time_nanos() - _cur_meta->cpuwide_start_ns; }

        // True iff current task is the one running run_main_task()
        bool is_current_main_task() const { return current_fid() == _main_tid; }

        // True iff current task is in pthread-mode.
        bool is_current_pthread_task() const { return _cur_meta->stack == _main_stack; }

        // Active time in nanoseconds spent by this FiberWorker.
        int64_t cumulated_cputime_ns() const { return _cumulated_cputime_ns; }

        // Push a fiber into the runqueue
        void ready_to_run(fiber_id_t tid, bool nosignal = false);

        // Flush tasks pushed to rq but signalled.
        void flush_nosignal_tasks();

        // Push a fiber into the runqueue from another non-worker thread.
        void ready_to_run_remote(fiber_id_t tid, bool nosignal = false);

        void flush_nosignal_tasks_remote_locked(std::mutex &locked_mutex);

        void flush_nosignal_tasks_remote();

        // Automatically decide the caller is remote or local, and call
        // the corresponding function.
        void ready_to_run_general(fiber_id_t tid, bool nosignal = false);

        void flush_nosignal_tasks_general();

        // The ScheduleGroup that this FiberWorker belongs to.
        ScheduleGroup *control() const { return _control; }

        // Call this instead of delete.
        void destroy_self();

        // Wake up blocking ops in the thread.
        // Returns 0 on success, errno otherwise.
        static turbo::Status interrupt(fiber_id_t tid, ScheduleGroup *c);

        // Get the meta associate with the task.
        static FiberEntity *address_meta(fiber_id_t tid);

        // Push a task into _rq, if _rq is full, retry after some time. This
        // process make go on indefinitely.
        void push_rq(fiber_id_t tid);

    private:

        friend class ScheduleGroup;

        // You shall use ScheduleGroup::create_group to create new instance.
        explicit FiberWorker(ScheduleGroup *);

        int init(size_t runqueue_capacity);

        // You shall call destroy_self() instead of destructor because deletion
        // of groups are postponed to avoid race.
        ~FiberWorker();

        static void task_runner(intptr_t skip_remained);

        // Callbacks for set_remained()
        static void _release_last_context(void *);

        static void _add_sleep_event(void *);

        struct ReadyToRunArgs {
            fiber_id_t tid;
            bool nosignal;
        };

        static void ready_to_run_in_worker(void *);

        static void ready_to_run_in_worker_ignoresignal(void *);

        // Wait for a task to run.
        // Returns true on success, false is treated as permanent error and the
        // loop calling this function should end.
        bool wait_task(fiber_id_t *tid);

        bool steal_task(fiber_id_t *tid) {
            if (_remote_rq.pop(tid)) {
                return true;
            }
#ifndef FIBER_DONT_SAVE_PARKING_STATE
            _last_pl_state = _pl->get_state();
#endif
            return _control->steal_task(tid, &_steal_seed, _steal_offset);
        }

#ifndef NDEBUG
        int _sched_recursive_guard;
#endif

        FiberEntity *_cur_meta;

        // the control that this group belongs to
        ScheduleGroup *_control;
        int _num_nosignal;
        int _nsignaled;
        // last scheduling time
        int64_t _last_run_ns;
        int64_t _cumulated_cputime_ns;

        size_t _nswitch;
        RemainedFn _last_context_remained;
        void *_last_context_remained_arg;

        ParkingLot *_pl;
#ifndef FIBER_DONT_SAVE_PARKING_STATE
        ParkingLot::State _last_pl_state;
#endif
        size_t _steal_seed;
        size_t _steal_offset;
        ContextualStack *_main_stack;
        fiber_id_t _main_tid;
        WorkStealingQueue<fiber_id_t> _rq;
        RemoteTaskQueue _remote_rq;
        int _remote_num_nosignal;
        int _remote_nsignaled;
    };

}  // namespace turbo::fiber_internal


namespace turbo::fiber_internal {

    // Utilities to manipulate fiber_id_t
    inline fiber_id_t make_tid(uint32_t version, turbo::ResourceId<FiberEntity> slot) {
        return (((fiber_id_t) version) << 32) | (fiber_id_t) slot.value;
    }

    inline turbo::ResourceId<FiberEntity> get_slot(fiber_id_t tid) {
        turbo::ResourceId<FiberEntity> id = {(tid & 0xFFFFFFFFul)};
        return id;
    }

    inline uint32_t get_version(fiber_id_t tid) {
        return (uint32_t) ((tid >> 32) & 0xFFFFFFFFul);
    }

    inline FiberEntity *FiberWorker::address_meta(fiber_id_t tid) {
        // FiberEntity * m = address_resource<FiberEntity>(get_slot(tid));
        // if (m != nullptr && m->version == get_version(tid)) {
        //     return m;
        // }
        // return nullptr;
        return address_resource(get_slot(tid));
    }

    inline void FiberWorker::exchange(FiberWorker **pg, fiber_id_t next_tid) {
        FiberWorker *g = *pg;
        if (g->is_current_pthread_task()) {
            return g->ready_to_run(next_tid);
        }
        ReadyToRunArgs args = {g->current_fid(), false};
        g->set_remained((g->current_task()->about_to_quit
                         ? ready_to_run_in_worker_ignoresignal
                         : ready_to_run_in_worker),
                        &args);
        FiberWorker::sched_to(pg, next_tid);
    }

    inline void FiberWorker::sched_to(FiberWorker **pg, fiber_id_t next_tid) {
        FiberEntity *next_meta = address_meta(next_tid);
        if (next_meta->stack == nullptr) {
            ContextualStack *stk = get_stack(next_meta->stack_type(), task_runner);
            if (stk) {
                next_meta->set_stack(stk);
            } else {
                next_meta->attr.stack_type = StackType::STACK_TYPE_PTHREAD;
                next_meta->set_stack((*pg)->_main_stack);
            }
        }
        // Update now_ns only when wait_task did yield.
        sched_to(pg, next_meta);
    }

    inline void FiberWorker::push_rq(fiber_id_t tid) {
        while (!_rq.push(tid)) {
            // Created too many fibers: a promising approach is to insert the
            // task into another FiberWorker, but we don't use it because:
            // * There're already many fibers to run, inserting the fiber
            //   into other FiberWorker does not help.
            // * Insertions into other TaskGroups perform worse when all workers
            //   are busy at creating fibers (proved by test_input_messenger in
            //   turbo)
            flush_nosignal_tasks();
            TLOG_ERROR_EVERY_SEC("_rq is full, capacity={}", _rq.capacity());
            // A better solution is to pop and run existing fibers, however which
            // make set_remained()-callbacks do context switches and need extensive
            // reviews on related code.
            turbo::sleep_for(turbo::Duration::milliseconds(1));
        }
    }

    inline void FiberWorker::flush_nosignal_tasks_remote() {
        if (_remote_num_nosignal) {
            _remote_rq._mutex.lock();
            flush_nosignal_tasks_remote_locked(_remote_rq._mutex);
        }
    }

}  // namespace turbo::fiber_internal


#endif  // TURBO_FIBER_INTERNAL_FIBER_WORKER_H_
