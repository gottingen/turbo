
#include "turbo/base/static_atomic.h"                // std::atomic
#include "turbo/base/scoped_lock.h"              // TURBO_SCOPED_LOCK
#include "turbo/base/profile.h"
#include "turbo/container/linked_list.h"   // link_node

#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
#include "turbo/base/singleton_on_pthread_once.h"
#endif

#include "turbo/log/logging.h"
#include "turbo/memory/object_pool.h"
#include "turbo/fiber/internal/errno.h"                 // EWOULDBLOCK
#include "turbo/fiber/internal/sys_futex.h"             // futex_*
#include "turbo/fiber/internal/processor.h"             // cpu_relax
#include "turbo/fiber/internal/schedule_group.h"          // schedule_group
#include "turbo/fiber/internal/fiber_worker.h"            // fiber_worker
#include "turbo/fiber/internal/timer_thread.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/mutex.h"

// This file implements waitable_event.h
// Provides futex-like semantics which is sequenced wait and wake operations
// and guaranteed visibilities.
//
// If wait is sequenced before wake:
//    [thread1]             [thread2]
//    wait()                value = new_value
//                          wake()
// wait() sees unmatched value(fail to wait), or wake() sees the waiter.
//
// If wait is sequenced after wake:
//    [thread1]             [thread2]
//                          value = new_value
//                          wake()
//    wait()
// wake() must provide some sort of memory fence to prevent assignment
// of value to be reordered after it. Thus the value is visible to wait()
// as well.

namespace turbo::fiber_internal {

#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
    struct waitable_event_count : public turbo::gauge<int64_t> {
        waitable_event_count() : turbo::Adder<int64_t>("fiber_waitable_event_count") {}
    };
    inline turbo::gauge<int64_t>& get_waitable_event_count() {
        return *turbo::get_leaky_singleton<waitable_event_count>();
    }
#endif

// If a thread would suspend for less than so many microseconds, return
// ETIMEDOUT directly.
// Use 1: sleeping for less than 2 microsecond is inefficient and useless.
    static const int64_t MIN_SLEEP_US = 2;

    enum WaiterState {
        WAITER_STATE_NONE,
        WAITER_STATE_READY,
        WAITER_STATE_TIMEDOUT,
        WAITER_STATE_UNMATCHEDVALUE,
        WAITER_STATE_INTERRUPTED,
    };

    struct waitable_event;

    struct fiber_mutex_waiter : public turbo::container::link_node<fiber_mutex_waiter> {
        // tids of pthreads are 0
        fiber_id_t tid;

        // Erasing node from middle of linked_list is thread-unsafe, we need
        // to hold its container's lock.
        std::atomic<waitable_event *> container;
    };

    // non_pthread_task allocates this structure on stack and queue it in
    // waitable_event::waiters.
    struct event_fiber_waiter : public fiber_mutex_waiter {
        fiber_entity *task_meta;
        TimerThread::TaskId sleep_id;
        WaiterState waiter_state;
        int expected_value;
        waitable_event *initial_event;
        schedule_group *control;
    };

    // pthread_task or main_task allocates this structure on stack and queue it
    // in waitable_event::waiters.
    struct event_pthread_waiter : public fiber_mutex_waiter {
        std::atomic<int> sig;
    };

    typedef turbo::container::linked_list<fiber_mutex_waiter> event_waiter_list;

    enum event_pthread_signal {
        PTHREAD_NOT_SIGNALLED, PTHREAD_SIGNALLED
    };

    struct TURBO_CACHELINE_ALIGNMENT waitable_event {
        waitable_event() {}

        ~waitable_event() {}

        std::atomic<int> value;
        event_waiter_list waiters;
        internal::FastPthreadMutex waiter_lock;
    };

    static_assert(offsetof(waitable_event, value) == 0, "offsetof value must be 0");
    static_assert(sizeof(waitable_event) == TURBO_CACHE_LINE_SIZE, "event fits in one cacheline");

    static void wakeup_pthread(event_pthread_waiter *pw) {
        // release fence makes wait_pthread see changes before wakeup.
        pw->sig.store(PTHREAD_SIGNALLED, std::memory_order_release);
        // At this point, wait_pthread() possibly has woken up and destroyed `pw'.
        // In which case, futex_wake_private() should return EFAULT.
        // If crash happens in future, `pw' can be made TLS and never destroyed
        // to solve the issue.
        futex_wake_private(&pw->sig, 1);
    }

    bool erase_from_event(fiber_mutex_waiter *, bool, WaiterState);

    int wait_pthread(event_pthread_waiter &pw, timespec *ptimeout) {
        while (true) {
            const int rc = futex_wait_private(&pw.sig, PTHREAD_NOT_SIGNALLED, ptimeout);
            if (PTHREAD_NOT_SIGNALLED != pw.sig.load(std::memory_order_acquire)) {
                // If `sig' is changed, wakeup_pthread() must be called and `pw'
                // is already removed from the event.
                // Acquire fence makes this thread sees changes before wakeup.
                return rc;
            }
            if (rc != 0 && errno == ETIMEDOUT) {
                // Note that we don't handle the EINTR from futex_wait here since
                // pthreads waiting on a event should behave similarly as fibers
                // which are not able to be woken-up by signals.
                // EINTR on event is only producible by fiber_worker::interrupt().

                // `pw' is still in the queue, remove it.
                if (!erase_from_event(&pw, false, WAITER_STATE_TIMEDOUT)) {
                    // Another thread is erasing `pw' as well, wait for the signal.
                    // Acquire fence makes this thread sees changes before wakeup.
                    if (pw.sig.load(std::memory_order_acquire) == PTHREAD_NOT_SIGNALLED) {
                        ptimeout = nullptr; // already timedout, ptimeout is expired.
                        continue;
                    }
                }
                return rc;
            }
        }
    }

    extern TURBO_THREAD_LOCAL fiber_worker *tls_task_group;

// Returns 0 when no need to unschedule or successfully unscheduled,
// -1 otherwise.
    inline int unsleep_if_necessary(event_fiber_waiter *w,
                                    TimerThread *timer_thread) {
        if (!w->sleep_id) {
            return 0;
        }
        if (timer_thread->unschedule(w->sleep_id) > 0) {
            // the callback is running.
            return -1;
        }
        w->sleep_id = 0;
        return 0;
    }

// Use ObjectPool(which never frees memory) to solve the race between
// waitable_event_wake() and waitable_event_destroy(). The race is as follows:
//
//   class Event {
//   public:
//     void wait() {
//       _mutex.lock();
//       if (!_done) {
//         _cond.wait(&_mutex);
//       }
//       _mutex.unlock();
//     }
//     void signal() {
//       _mutex.lock();
//       if (!_done) {
//         _done = true;
//         _cond.signal();
//       }
//       _mutex.unlock();  /*1*/
//     }
//   private:
//     bool _done = false;
//     Mutex _mutex;
//     Condition _cond;
//   };
//
//   [Thread1]                         [Thread2]
//   foo() {
//     Event event;
//     pass_to_thread2(&event);  --->  event.signal();
//     event.wait();
//   } <-- event destroyed
//   
// Summary: Thread1 passes a stateful condition to Thread2 and waits until
// the condition being signalled, which basically means the associated
// job is done and Thread1 can release related resources including the mutex
// and condition. The scenario is fine and the code is correct.
// The race needs a closer look. The unlock at /*1*/ may have different 
// implementations, but in which the last step is probably an atomic store
// and waitable_event_wake(), like this:
//
//   locked->store(0);
//   waitable_event_wake(locked);
//
// The `locked' represents the locking status of the mutex. The issue is that
// just after the store(), the mutex is already unlocked and the code in
// Event.wait() may successfully grab the lock and go through everything
// left and leave foo() function, destroying the mutex and event, making
// the waitable_event_wake(locked) crash.
// To solve this issue, one method is to add reference before store and
// release the reference after waitable_event_wake. However reference countings need
// to be added in nearly every user scenario of waitable_event_wake(), which is very
// error-prone. Another method is never freeing event, with the side effect
// that waitable_event_wake() may wake up an unrelated event(the one reuses the memory)
// and cause spurious wakeups. According to our observations, the race is 
// infrequent, even rare. The extra spurious wakeups should be acceptable.

    void *waitable_event_create() {
        waitable_event *b = turbo::get_object<waitable_event>();
        if (b) {
            return &b->value;
        }
        return nullptr;
    }

    void waitable_event_destroy(void *event) {
        if (!event) {
            return;
        }
        waitable_event *b = static_cast<waitable_event *>(
                TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(event), waitable_event, value));
        turbo::return_object(b);
    }

    inline fiber_worker *get_task_group(schedule_group *c) {
        fiber_worker *g = tls_task_group;
        return g ? g : c->choose_one_group();
    }

    int waitable_event_wake(void *arg) {
        waitable_event *b = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg), waitable_event, value);
        fiber_mutex_waiter *front = nullptr;
        {
            TURBO_SCOPED_LOCK(b->waiter_lock);
            if (b->waiters.empty()) {
                return 0;
            }
            front = b->waiters.head()->value();
            front->remove_from_list();
            front->container.store(nullptr, std::memory_order_relaxed);
        }
        if (front->tid == 0) {
            wakeup_pthread(static_cast<event_pthread_waiter *>(front));
            return 1;
        }
        event_fiber_waiter *bbw = static_cast<event_fiber_waiter *>(front);
        unsleep_if_necessary(bbw, get_global_timer_thread());
        fiber_worker *g = tls_task_group;
        if (g) {
            fiber_worker::exchange(&g, bbw->tid);
        } else {
            bbw->control->choose_one_group()->ready_to_run_remote(bbw->tid);
        }
        return 1;
    }

    int waitable_event_wake_all(void *arg) {
        waitable_event *b = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg), waitable_event, value);

        event_waiter_list fiber_waiters;
        event_waiter_list pthread_waiters;
        {
            TURBO_SCOPED_LOCK(b->waiter_lock);
            while (!b->waiters.empty()) {
                fiber_mutex_waiter *bw = b->waiters.head()->value();
                bw->remove_from_list();
                bw->container.store(nullptr, std::memory_order_relaxed);
                if (bw->tid) {
                    fiber_waiters.append(bw);
                } else {
                    pthread_waiters.append(bw);
                }
            }
        }

        int nwakeup = 0;
        while (!pthread_waiters.empty()) {
            event_pthread_waiter *bw = static_cast<event_pthread_waiter *>(
                    pthread_waiters.head()->value());
            bw->remove_from_list();
            wakeup_pthread(bw);
            ++nwakeup;
        }
        if (fiber_waiters.empty()) {
            return nwakeup;
        }
        // We will exchange with first waiter in the end.
        event_fiber_waiter *next = static_cast<event_fiber_waiter *>(
                fiber_waiters.head()->value());
        next->remove_from_list();
        unsleep_if_necessary(next, get_global_timer_thread());
        ++nwakeup;
        fiber_worker *g = get_task_group(next->control);
        const int saved_nwakeup = nwakeup;
        while (!fiber_waiters.empty()) {
            // pop reversely
            event_fiber_waiter *w = static_cast<event_fiber_waiter *>(
                    fiber_waiters.tail()->value());
            w->remove_from_list();
            unsleep_if_necessary(w, get_global_timer_thread());
            g->ready_to_run_general(w->tid, true);
            ++nwakeup;
        }
        if (saved_nwakeup != nwakeup) {
            g->flush_nosignal_tasks_general();
        }
        if (g == tls_task_group) {
            fiber_worker::exchange(&g, next->tid);
        } else {
            g->ready_to_run_remote(next->tid);
        }
        return nwakeup;
    }

    int waitable_event_wake_except(void *arg, fiber_id_t excluded_fiber) {
        waitable_event *b = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg), waitable_event, value);

        event_waiter_list fiber_waiters;
        event_waiter_list pthread_waiters;
        {
            fiber_mutex_waiter *excluded_waiter = nullptr;
            TURBO_SCOPED_LOCK(b->waiter_lock);
            while (!b->waiters.empty()) {
                fiber_mutex_waiter *bw = b->waiters.head()->value();
                bw->remove_from_list();

                if (bw->tid) {
                    if (bw->tid != excluded_fiber) {
                        fiber_waiters.append(bw);
                        bw->container.store(nullptr, std::memory_order_relaxed);
                    } else {
                        excluded_waiter = bw;
                    }
                } else {
                    bw->container.store(nullptr, std::memory_order_relaxed);
                    pthread_waiters.append(bw);
                }
            }

            if (excluded_waiter) {
                b->waiters.append(excluded_waiter);
            }
        }

        int nwakeup = 0;
        while (!pthread_waiters.empty()) {
            event_pthread_waiter *bw = static_cast<event_pthread_waiter *>(
                    pthread_waiters.head()->value());
            bw->remove_from_list();
            wakeup_pthread(bw);
            ++nwakeup;
        }

        if (fiber_waiters.empty()) {
            return nwakeup;
        }
        event_fiber_waiter *front = static_cast<event_fiber_waiter *>(
                fiber_waiters.head()->value());

        fiber_worker *g = get_task_group(front->control);
        const int saved_nwakeup = nwakeup;
        do {
            // pop reversely
            event_fiber_waiter *w = static_cast<event_fiber_waiter *>(
                    fiber_waiters.tail()->value());
            w->remove_from_list();
            unsleep_if_necessary(w, get_global_timer_thread());
            g->ready_to_run_general(w->tid, true);
            ++nwakeup;
        } while (!fiber_waiters.empty());
        if (saved_nwakeup != nwakeup) {
            g->flush_nosignal_tasks_general();
        }
        return nwakeup;
    }

    int waitable_event_requeue(void *arg, void *arg2) {
        waitable_event *b = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg), waitable_event, value);
        waitable_event *m = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg2), waitable_event, value);

        fiber_mutex_waiter *front = nullptr;
        {
            std::unique_lock<internal::FastPthreadMutex> lck1(b->waiter_lock, std::defer_lock);
            std::unique_lock<internal::FastPthreadMutex> lck2(m->waiter_lock, std::defer_lock);
            turbo::base::double_lock(lck1, lck2);
            if (b->waiters.empty()) {
                return 0;
            }

            front = b->waiters.head()->value();
            front->remove_from_list();
            front->container.store(nullptr, std::memory_order_relaxed);

            while (!b->waiters.empty()) {
                fiber_mutex_waiter *bw = b->waiters.head()->value();
                bw->remove_from_list();
                m->waiters.append(bw);
                bw->container.store(m, std::memory_order_relaxed);
            }
        }

        if (front->tid == 0) {  // which is a pthread
            wakeup_pthread(static_cast<event_pthread_waiter *>(front));
            return 1;
        }
        event_fiber_waiter *bbw = static_cast<event_fiber_waiter *>(front);
        unsleep_if_necessary(bbw, get_global_timer_thread());
        fiber_worker *g = tls_task_group;
        if (g) {
            fiber_worker::exchange(&g, front->tid);
        } else {
            bbw->control->choose_one_group()->ready_to_run_remote(front->tid);
        }
        return 1;
    }

// Callable from multiple threads, at most one thread may wake up the waiter.
    static void erase_from_event_and_wakeup(void *arg) {
        erase_from_event(static_cast<fiber_mutex_waiter *>(arg), true, WAITER_STATE_TIMEDOUT);
    }

// Used in task_group.cpp
    bool erase_from_event_because_of_interruption(fiber_mutex_waiter *bw) {
        return erase_from_event(bw, true, WAITER_STATE_INTERRUPTED);
    }

    inline bool erase_from_event(fiber_mutex_waiter *bw, bool wakeup, WaiterState state) {
        // `bw' is guaranteed to be valid inside this function because waiter
        // will wait until this function being cancelled or finished.
        // NOTE: This function must be no-op when bw->container is nullptr.
        bool erased = false;
        waitable_event *b;
        int saved_errno = errno;
        while ((b = bw->container.load(std::memory_order_acquire))) {
            // b can be nullptr when the waiter is scheduled but queued.
            TURBO_SCOPED_LOCK(b->waiter_lock);
            if (b == bw->container.load(std::memory_order_relaxed)) {
                bw->remove_from_list();
                bw->container.store(nullptr, std::memory_order_relaxed);
                if (bw->tid) {
                    static_cast<event_fiber_waiter *>(bw)->waiter_state = state;
                }
                erased = true;
                break;
            }
        }
        if (erased && wakeup) {
            if (bw->tid) {
                event_fiber_waiter *bbw = static_cast<event_fiber_waiter *>(bw);
                get_task_group(bbw->control)->ready_to_run_general(bw->tid);
            } else {
                event_pthread_waiter *pw = static_cast<event_pthread_waiter *>(bw);
                wakeup_pthread(pw);
            }
        }
        errno = saved_errno;
        return erased;
    }

    static void wait_for_event(void *arg) {
        event_fiber_waiter *const bw = static_cast<event_fiber_waiter *>(arg);
        waitable_event *const b = bw->initial_event;
        // 1: waiter with timeout should have waiter_state == WAITER_STATE_READY
        //    before they're queued, otherwise the waiter is already timedout
        //    and removed by TimerThread, in which case we should stop queueing.
        //
        // Visibility of waiter_state:
        //    [fiber]                         [TimerThread]
        //    waiter_state = TIMED
        //    tt_lock { add task }
        //                                      tt_lock { get task }
        //                                      waiter_lock { waiter_state=TIMEDOUT }
        //    waiter_lock { use waiter_state }
        // tt_lock represents TimerThread::_mutex. Visibility of waiter_state is
        // sequenced by two locks, both threads are guaranteed to see the correct
        // value.
        {
            TURBO_SCOPED_LOCK(b->waiter_lock);
            if (b->value.load(std::memory_order_relaxed) != bw->expected_value) {
                bw->waiter_state = WAITER_STATE_UNMATCHEDVALUE;
            } else if (bw->waiter_state == WAITER_STATE_READY/*1*/ &&
                       !bw->task_meta->interrupted) {
                b->waiters.append(bw);
                bw->container.store(b, std::memory_order_relaxed);
                return;
            }
        }

        // b->container is nullptr which makes erase_from_event_and_wakeup() and
        // fiber_worker::interrupt() no-op, there's no race between following code and
        // the two functions. The on-stack event_fiber_waiter is safe to use and
        // bw->waiter_state will not change again.
        unsleep_if_necessary(bw, get_global_timer_thread());
        tls_task_group->ready_to_run(bw->tid);
        // FIXME: jump back to original thread is buggy.

        // // Value unmatched or waiter is already woken up by TimerThread, jump
        // // back to original fiber.
        // fiber_worker* g = tls_task_group;
        // ReadyToRunArgs args = { g->current_fid(), false };
        // g->set_remained(fiber_worker::ready_to_run_in_worker, &args);
        // // 2: Don't run remained because we're already in a remained function
        // //    otherwise stack may overflow.
        // fiber_worker::sched_to(&g, bw->tid, false/*2*/);
    }

    static int event_wait_from_pthread(fiber_worker *g, waitable_event *b, int expected_value,
                                       const timespec *abstime) {
        // sys futex needs relative timeout.
        // Compute diff between abstime and now.
        timespec *ptimeout = nullptr;
        timespec timeout;
        if (abstime != nullptr) {
            const int64_t timeout_us =  turbo::time_point::from_timespec(*abstime).to_unix_micros() -
                                       turbo::get_current_time_micros();
            if (timeout_us < MIN_SLEEP_US) {
                errno = ETIMEDOUT;
                return -1;
            }
            timeout = turbo::time_point::from_unix_micros(timeout_us).to_timespec();
            ptimeout = &timeout;
        }

        fiber_entity *task = nullptr;
        event_pthread_waiter pw;
        pw.tid = 0;
        pw.sig.store(PTHREAD_NOT_SIGNALLED, std::memory_order_relaxed);
        int rc = 0;

        if (g) {
            task = g->current_task();
            task->current_waiter.store(&pw, std::memory_order_release);
        }
        b->waiter_lock.lock();
        if (b->value.load(std::memory_order_relaxed) != expected_value) {
            b->waiter_lock.unlock();
            errno = EWOULDBLOCK;
            rc = -1;
        } else if (task != nullptr && task->interrupted) {
            b->waiter_lock.unlock();
            // Race with set and may consume multiple interruptions, which are OK.
            task->interrupted = false;
            errno = EINTR;
            rc = -1;
        } else {
            b->waiters.append(&pw);
            pw.container.store(b, std::memory_order_relaxed);
            b->waiter_lock.unlock();

#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
            turbo::Adder<int64_t>& num_waiters = get_waitable_event_count();
            num_waiters << 1;
#endif
            rc = wait_pthread(pw, ptimeout);
#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
            num_waiters << -1;
#endif
        }
        if (task) {
            // If current_waiter is nullptr, fiber_worker::interrupt() is running and
            // using pw, spin until current_waiter != nullptr.
            BT_LOOP_WHEN(task->current_waiter.exchange(
                    nullptr, std::memory_order_acquire) == nullptr,
                         30/*nops before sched_yield*/);
            if (task->interrupted) {
                task->interrupted = false;
                if (rc == 0) {
                    errno = EINTR;
                    return -1;
                }
            }
        }
        return rc;
    }

    int waitable_event_wait(void *arg, int expected_value, const timespec *abstime) {
        waitable_event *b = TURBO_CONTAINER_OF(static_cast<std::atomic<int> *>(arg), waitable_event, value);
        if (b->value.load(std::memory_order_relaxed) != expected_value) {
            errno = EWOULDBLOCK;
            // Sometimes we may take actions immediately after unmatched event,
            // this fence makes sure that we see changes before changing event.
            std::atomic_thread_fence(std::memory_order_acquire);
            return -1;
        }
        fiber_worker *g = tls_task_group;
        if (nullptr == g || g->is_current_pthread_task()) {
            return event_wait_from_pthread(g, b, expected_value, abstime);
        }
        event_fiber_waiter bbw;
        // tid is 0 iff the thread is non-fiber
        bbw.tid = g->current_fid();
        bbw.container.store(nullptr, std::memory_order_relaxed);
        bbw.task_meta = g->current_task();
        bbw.sleep_id = 0;
        bbw.waiter_state = WAITER_STATE_READY;
        bbw.expected_value = expected_value;
        bbw.initial_event = b;
        bbw.control = g->control();

        if (abstime != nullptr) {
            // Schedule timer before queueing. If the timer is triggered before
            // queueing, cancel queueing. This is a kind of optimistic locking.
            if ( turbo::time_point::from_timespec(*abstime).to_unix_micros() <
                (turbo::get_current_time_micros() + MIN_SLEEP_US)) {
                // Already timed out.
                errno = ETIMEDOUT;
                return -1;
            }
            bbw.sleep_id = get_global_timer_thread()->schedule(
                    erase_from_event_and_wakeup, &bbw, *abstime);
            if (!bbw.sleep_id) {  // TimerThread stopped.
                errno = ESTOP;
                return -1;
            }
        }
#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
        turbo::Adder<int64_t>& num_waiters = get_waitable_event_count();
        num_waiters << 1;
#endif

        // release fence matches with acquire fence in interrupt_and_consume_waiters
        // in task_group.cpp to guarantee visibility of `interrupted'.
        bbw.task_meta->current_waiter.store(&bbw, std::memory_order_release);
        g->set_remained(wait_for_event, &bbw);
        fiber_worker::sched(&g);

        // erase_from_event_and_wakeup (called by TimerThread) is possibly still
        // running and using bbw. The chance is small, just spin until it's done.
        BT_LOOP_WHEN(unsleep_if_necessary(&bbw, get_global_timer_thread()) < 0,
                     30/*nops before sched_yield*/);

        // If current_waiter is nullptr, fiber_worker::interrupt() is running and using bbw.
        // Spin until current_waiter != nullptr.
        BT_LOOP_WHEN(bbw.task_meta->current_waiter.exchange(
                nullptr, std::memory_order_acquire) == nullptr,
                     30/*nops before sched_yield*/);
#ifdef SHOW_FIBER_EVENT_WAITER_COUNT_IN_VARS
        num_waiters << -1;
#endif

        bool is_interrupted = false;
        if (bbw.task_meta->interrupted) {
            // Race with set and may consume multiple interruptions, which are OK.
            bbw.task_meta->interrupted = false;
            is_interrupted = true;
        }
        // If timed out as well as value unmatched, return ETIMEDOUT.
        if (WAITER_STATE_TIMEDOUT == bbw.waiter_state) {
            errno = ETIMEDOUT;
            return -1;
        } else if (WAITER_STATE_UNMATCHEDVALUE == bbw.waiter_state) {
            errno = EWOULDBLOCK;
            return -1;
        } else if (is_interrupted) {
            errno = EINTR;
            return -1;
        }
        return 0;
    }

}  // namespace turbo::fiber_internal

namespace turbo {
    template<>
    struct ObjectPoolBlockMaxItem<turbo::fiber_internal::waitable_event> {
        static const size_t value = 128;
    };
}
