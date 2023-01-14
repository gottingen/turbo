
#ifndef TURBO_FIBER_INTERNAL_FIBER_WORKER_H_
#define TURBO_FIBER_INTERNAL_FIBER_WORKER_H_

#include "turbo/times/time.h"                             // cpuwide_time_ns
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_entity.h"                     // fiber_id_t, fiber_entity
#include "turbo/fiber/internal/work_stealing_queue.h"           // WorkStealingQueue
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
    class fiber_worker {
    public:
        // Create task `fn(arg)' with attributes `attr' in fiber_worker *pg and put
        // the identifier into `tid'. Switch to the new task and schedule old task
        // to run.
        // Return 0 on success, errno otherwise.
        static int start_foreground(fiber_worker **pg,
                                    fiber_id_t *__restrict tid,
                                    const fiber_attribute *__restrict attr,
                                    std::function<void*(void*)> && fn,
                                    void *__restrict arg);

        // Create task `fn(arg)' with attributes `attr' in this fiber_worker, put the
        // identifier into `tid'. Schedule the new thread to run.
        //   Called from worker: start_background<false>
        //   Called from non-worker: start_background<true>
        // Return 0 on success, errno otherwise.
        template<bool REMOTE>
        int start_background(fiber_id_t *__restrict tid,
                             const fiber_attribute *__restrict attr,
                             std::function<void*(void*)> && fn,
                             void *__restrict arg);

        // Suspend caller and run next fiber in fiber_worker *pg.
        static void sched(fiber_worker **pg);

        static void ending_sched(fiber_worker **pg);

        // Suspend caller and run fiber `next_tid' in fiber_worker *pg.
        // Purpose of this function is to avoid pushing `next_tid' to _rq and
        // then being popped by sched(pg), which is not necessary.
        static void sched_to(fiber_worker **pg, fiber_entity *next_meta);

        static void sched_to(fiber_worker **pg, fiber_id_t next_tid);

        static void exchange(fiber_worker **pg, fiber_id_t next_tid);

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
        // instead. This function does not create thread-local fiber_worker.
        // Returns: 0 on success, -1 otherwise and errno is set.
        static int usleep(fiber_worker **pg, uint64_t timeout_us);

        // Suspend caller and run another fiber. When the caller will resume
        // is undefined.
        static void yield(fiber_worker **pg);

        // Suspend caller until fiber `tid' terminates.
        static int join(fiber_id_t tid, void **return_value);

        // Returns true iff the fiber `tid' still exists. Notice that it is
        // just the result at this very moment which may change soon.
        // Don't use this function unless you have to. Never write code like this:
        //    if (exists(tid)) {
        //        Wait for events of the thread.   // Racy, may block indefinitely.
        //    }
        static bool exists(fiber_id_t tid);

        // Put attribute associated with `tid' into `*attr'.
        // Returns 0 on success, -1 otherwise and errno is set.
        static int get_attr(fiber_id_t tid, fiber_attribute *attr);

        // Get/set fiber_entity.stop of the tid.
        static void set_stopped(fiber_id_t tid);

        static bool is_stopped(fiber_id_t tid);

        // The fiber running run_main_task();
        fiber_id_t main_tid() const { return _main_tid; }

        fiber_statistics main_stat() const;

        // Routine of the main task which should be called from a dedicated pthread.
        void run_main_task();

        // current_task is a function in macOS 10.0+
#ifdef current_task
#undef current_task
#endif

        // Meta/Identifier of current task in this group.
        fiber_entity *current_task() const { return _cur_meta; }

        fiber_id_t current_fid() const { return _cur_meta->tid; }

        // Uptime of current task in nanoseconds.
        int64_t current_uptime_ns() const { return turbo::get_current_time_nanos() - _cur_meta->cpuwide_start_ns; }

        // True iff current task is the one running run_main_task()
        bool is_current_main_task() const { return current_fid() == _main_tid; }

        // True iff current task is in pthread-mode.
        bool is_current_pthread_task() const { return _cur_meta->stack == _main_stack; }

        // Active time in nanoseconds spent by this fiber_worker.
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

        // The schedule_group that this fiber_worker belongs to.
        schedule_group *control() const { return _control; }

        // Call this instead of delete.
        void destroy_self();

        // Wake up blocking ops in the thread.
        // Returns 0 on success, errno otherwise.
        static int interrupt(fiber_id_t tid, schedule_group *c);

        // Get the meta associate with the task.
        static fiber_entity *address_meta(fiber_id_t tid);

        // Push a task into _rq, if _rq is full, retry after some time. This
        // process make go on indefinitely.
        void push_rq(fiber_id_t tid);

    private:

        friend class schedule_group;

        // You shall use schedule_group::create_group to create new instance.
        explicit fiber_worker(schedule_group *);

        int init(size_t runqueue_capacity);

        // You shall call destroy_self() instead of destructor because deletion
        // of groups are postponed to avoid race.
        ~fiber_worker();

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

        fiber_entity *_cur_meta;

        // the control that this group belongs to
        schedule_group *_control;
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
        fiber_contextual_stack *_main_stack;
        fiber_id_t _main_tid;
        WorkStealingQueue<fiber_id_t> _rq;
        RemoteTaskQueue _remote_rq;
        int _remote_num_nosignal;
        int _remote_nsignaled;
    };

}  // namespace turbo::fiber_internal

#include "turbo/fiber/internal/fiber_worker_inl.h"

#endif  // TURBO_FIBER_INTERNAL_FIBER_WORKER_H_
