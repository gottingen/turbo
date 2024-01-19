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

#include <atomic>
#include "turbo/container/intrusive_list.h"
#include "turbo/log/logging.h"
#include "turbo/memory/object_pool.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/base/processor.h"             // cpu_relax
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_worker.h"            // FiberWorker
#include "turbo/fiber/internal/timer.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/status/error.h"
#include "turbo/concurrent/lock.h"

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


    // If a thread would suspend for less than so many microseconds, return
    // ETIMEDOUT directly.
    // Use 1: sleeping for less than 2 microsecond is inefficient and useless.
    static const turbo::Duration MIN_SLEEP = turbo::Duration::microseconds(2);


    enum WaiterState {
        WAITER_STATE_NONE,
        WAITER_STATE_READY,
        WAITER_STATE_TIMEDOUT,
        WAITER_STATE_UNMATCHEDVALUE,
        WAITER_STATE_INTERRUPTED,
    };

    struct waitable_event;

    struct EventWaiterNode : public turbo::intrusive_list_node {
        // tids of pthreads are 0
        fiber_id_t tid;

        // Erasing node from middle of linked_list is thread-unsafe, we need
        // to hold its container's lock.
        std::atomic<waitable_event *> container;
    };

    // non_pthread_task allocates this structure on stack and queue it in
    // waitable_event::waiters.
    struct FiberEventWaiterNode : public EventWaiterNode {
        FiberEntity *task_meta;
        TimerId sleep_id;
        WaiterState waiter_state;
        int expected_value;
        waitable_event *initial_event;
        ScheduleGroup *control;
        turbo::Time   abstime;
    };

    // pthread_task or main_task allocates this structure on stack and queue it
    // in waitable_event::waiters.
    struct PthreadEventWaiterNode : public EventWaiterNode {
        turbo::SpinWaiter sig;
    };

    typedef turbo::intrusive_list<EventWaiterNode> event_waiter_list;

    enum event_pthread_signal {
        PTHREAD_NOT_SIGNALLED, PTHREAD_SIGNALLED
    };

    struct TURBO_CACHE_LINE_ALIGNED waitable_event {
        waitable_event() {}

        ~waitable_event() {}

        std::atomic<int> value;
        event_waiter_list waiters;
        turbo::SpinLock waiter_lock;
    };

    static_assert(sizeof(waitable_event) == TURBO_CACHE_LINE_SIZE, "event fits in one cacheline");

    static void wakeup_pthread(PthreadEventWaiterNode *pw) {
        // release fence makes wait_pthread see changes before wakeup.
        pw->sig.store(PTHREAD_SIGNALLED, std::memory_order_release);
        // At this point, wait_pthread() possibly has woken up and destroyed `pw'.
        // In which case, futex_wake_private() should return EFAULT.
        // If crash happens in future, `pw' can be made TLS and never destroyed
        // to solve the issue.
        pw->sig.wake(1);
    }

    bool erase_from_event(EventWaiterNode *, bool, WaiterState);

    turbo::Status wait_pthread(PthreadEventWaiterNode &pw, turbo::Time abstime) {
        timespec *ptimeout;
        timespec timeout;
        turbo::Duration delta;
        int rc = 0;
        while (true) {
            ptimeout = nullptr;
            if (abstime != turbo::Time::infinite_future()) {
                delta = abstime - turbo::time_now();
                timeout = delta.to_timespec();
                ptimeout = &timeout;
            }

            if (abstime == turbo::Time::infinite_future() || delta > MIN_SLEEP) {
                errno = 0;
                rc = turbo::concurrent_internal::futex_wait_private(&pw.sig, PTHREAD_NOT_SIGNALLED, ptimeout);
                if (PTHREAD_NOT_SIGNALLED != pw.sig.load(std::memory_order_acquire)) {
                    // If `sig' is changed, wakeup_pthread() must be called and `pw'
                    // is already removed from the event.
                    // Acquire fence makes this thread sees changes before wakeup.
                    return turbo::make_status();
                }
            } else {
                errno = ETIMEDOUT;
                rc = -1;
            }
            if (rc != 0 && errno == ETIMEDOUT) {
                // Note that we don't handle the EINTR from futex_wait here since
                // pthreads waiting on a event should behave similarly as fibers
                // which are not able to be woken-up by signals.
                // EINTR on event is only producible by FiberWorker::interrupt().
                // `pw' is still in the queue, remove it.
                if (!erase_from_event(&pw, false, WAITER_STATE_TIMEDOUT)) {
                    // Another thread is erasing `pw' as well, wait for the signal.
                    // Acquire fence makes this thread sees changes before wakeup.
                    if (pw.sig.load(std::memory_order_acquire) == PTHREAD_NOT_SIGNALLED) {
                        continue;
                    }
                }
                return turbo::make_status();
            }
        }
    }

    extern TURBO_THREAD_LOCAL FiberWorker *tls_task_group;

    // Returns 0 when no need to unschedule or successfully unscheduled,
    // -1 otherwise.
    inline int unsleep_if_necessary(FiberEventWaiterNode *w,
                                    TimerThread *timer_thread) {
        if (!w->sleep_id) {
            return 0;
        }
        if (timer_thread->unschedule(w->sleep_id).code() == kEBUSY) {
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
        waitable_event *b = static_cast<waitable_event *>(event);
        turbo::return_object(b);
    }

    inline FiberWorker *get_task_group(ScheduleGroup *c) {
        FiberWorker *g = tls_task_group;
        return g ? g : c->choose_one_group();
    }

    int waitable_event_wake(void *arg) {
        waitable_event *b = static_cast<waitable_event *>(arg);
        event_waiter_list::iterator front;
        {
            turbo::SpinLockHolder sl(&b->waiter_lock);
            if (b->waiters.empty()) {
                return 0;
            }
            front = b->waiters.begin();
            b->waiters.remove(*front);
            front->container.store(nullptr, std::memory_order_relaxed);
        }
        if (front->tid == 0) {
            wakeup_pthread(reinterpret_cast<PthreadEventWaiterNode *>(&(*front)));
            return 1;
        }
        FiberEventWaiterNode *bbw = reinterpret_cast<FiberEventWaiterNode *>(&(*front));
        unsleep_if_necessary(bbw, get_fiber_timer_thread());
        FiberWorker *g = tls_task_group;
        if (g) {
            FiberWorker::exchange(&g, bbw->tid);
        } else {
            bbw->control->choose_one_group()->ready_to_run_remote(bbw->tid);
        }
        return 1;
    }

    int waitable_event_wake_all(void *arg) {
        waitable_event *b = static_cast<waitable_event *>(arg);

        event_waiter_list fiber_waiters;
        event_waiter_list pthread_waiters;
        {
            turbo::SpinLockHolder sl(&b->waiter_lock);
            while (!b->waiters.empty()) {
                EventWaiterNode *bw = &(*b->waiters.begin());
                b->waiters.remove(*bw);
                bw->container.store(nullptr, std::memory_order_relaxed);
                if (bw->tid) {
                    fiber_waiters.push_back(*bw);
                } else {
                    pthread_waiters.push_back(*bw);
                }
            }
        }

        int nwakeup = 0;
        while (!pthread_waiters.empty()) {
            PthreadEventWaiterNode *bw = reinterpret_cast<PthreadEventWaiterNode *>(&(*pthread_waiters.begin()));
            pthread_waiters.remove(*bw);
            wakeup_pthread(bw);
            ++nwakeup;
        }
        if (fiber_waiters.empty()) {
            return nwakeup;
        }
        // We will exchange with first waiter in the end.
        FiberEventWaiterNode *next = reinterpret_cast<FiberEventWaiterNode *>(&(*fiber_waiters.begin()));
        fiber_waiters.remove(*next);
        unsleep_if_necessary(next, get_fiber_timer_thread());
        ++nwakeup;
        FiberWorker *g = get_task_group(next->control);
        const int saved_nwakeup = nwakeup;
        while (!fiber_waiters.empty()) {
            // pop reversely
            FiberEventWaiterNode *w = reinterpret_cast<FiberEventWaiterNode *>(&(*fiber_waiters.rbegin()));
            fiber_waiters.remove(*w);
            unsleep_if_necessary(w, get_fiber_timer_thread());
            g->ready_to_run_general(w->tid, true);
            ++nwakeup;
        }
        if (saved_nwakeup != nwakeup) {
            g->flush_nosignal_tasks_general();
        }
        if (g == tls_task_group) {
            FiberWorker::exchange(&g, next->tid);
        } else {
            g->ready_to_run_remote(next->tid);
        }
        return nwakeup;
    }

    int waitable_event_wake_except(void *arg, fiber_id_t excluded_fiber) {
        waitable_event *b = reinterpret_cast<waitable_event *>(arg);

        event_waiter_list fiber_waiters;
        event_waiter_list pthread_waiters;
        {
            EventWaiterNode *excluded_waiter = nullptr;
            turbo::SpinLockHolder sl(&b->waiter_lock);
            while (!b->waiters.empty()) {
                EventWaiterNode *bw = &(*b->waiters.begin());
                b->waiters.remove(*bw);
                if (bw->tid) {
                    if (bw->tid != excluded_fiber) {
                        fiber_waiters.push_back(*bw);
                        bw->container.store(nullptr, std::memory_order_relaxed);
                    } else {
                        excluded_waiter = bw;
                    }
                } else {
                    bw->container.store(nullptr, std::memory_order_relaxed);
                    pthread_waiters.push_back(*bw);
                }
            }

            if (excluded_waiter) {
                b->waiters.push_back(*excluded_waiter);
            }
        }

        int nwakeup = 0;
        while (!pthread_waiters.empty()) {
            PthreadEventWaiterNode *bw = reinterpret_cast<PthreadEventWaiterNode *>(&(*pthread_waiters.begin()));
            pthread_waiters.remove(*bw);
            wakeup_pthread(bw);
            ++nwakeup;
        }

        if (fiber_waiters.empty()) {
            return nwakeup;
        }

        FiberEventWaiterNode *front = reinterpret_cast<FiberEventWaiterNode *>(&(*fiber_waiters.begin()));

        FiberWorker *g = get_task_group(front->control);
        const int saved_nwakeup = nwakeup;
        do {
            // pop reversely
            FiberEventWaiterNode *w = reinterpret_cast<FiberEventWaiterNode *>(&(*fiber_waiters.rbegin()));
            fiber_waiters.remove(*w);
            unsleep_if_necessary(w, get_fiber_timer_thread());
            g->ready_to_run_general(w->tid, true);
            ++nwakeup;
        } while (!fiber_waiters.empty());
        if (saved_nwakeup != nwakeup) {
            g->flush_nosignal_tasks_general();
        }
        return nwakeup;
    }

    int waitable_event_requeue(void *arg, void *arg2) {
        waitable_event *b = static_cast<waitable_event *>(arg);
        waitable_event *m = static_cast<waitable_event *>(arg2);

        EventWaiterNode *front = nullptr;
        {
            std::unique_lock<turbo::SpinLock> lck1(b->waiter_lock, std::defer_lock);
            std::unique_lock<turbo::SpinLock> lck2(m->waiter_lock, std::defer_lock);
            turbo::double_lock(lck1, lck2);
            if (b->waiters.empty()) {
                return 0;
            }

            front = &(*b->waiters.begin());
            b->waiters.remove(*front);
            front->container.store(nullptr, std::memory_order_relaxed);

            while (!b->waiters.empty()) {
                EventWaiterNode *bw = &(*b->waiters.begin());
                b->waiters.remove(*bw);
                m->waiters.push_back(*bw);
                bw->container.store(m, std::memory_order_relaxed);
            }
        }

        if (front->tid == 0) {  // which is a pthread
            wakeup_pthread(static_cast<PthreadEventWaiterNode *>(front));
            return 1;
        }
        FiberEventWaiterNode *bbw = static_cast<FiberEventWaiterNode *>(front);
        unsleep_if_necessary(bbw, get_fiber_timer_thread());
        FiberWorker *g = tls_task_group;
        if (g) {
            FiberWorker::exchange(&g, front->tid);
        } else {
            bbw->control->choose_one_group()->ready_to_run_remote(front->tid);
        }
        return 1;
    }

    // Callable from multiple threads, at most one thread may wake up the waiter.
    static void erase_from_event_and_wakeup(void *arg) {
        erase_from_event(static_cast<EventWaiterNode *>(arg), true, WAITER_STATE_TIMEDOUT);
    }

    // Used in task_group.cpp
    bool erase_from_event_because_of_interruption(EventWaiterNode *bw) {
        return erase_from_event(bw, true, WAITER_STATE_INTERRUPTED);
    }

    inline bool erase_from_event(EventWaiterNode *bw, bool wakeup, WaiterState state) {
        // `bw' is guaranteed to be valid inside this function because waiter
        // will wait until this function being cancelled or finished.
        // NOTE: This function must be no-op when bw->container is nullptr.
        bool erased = false;
        waitable_event *b;
        int saved_errno = errno;
        while ((b = bw->container.load(std::memory_order_acquire))) {
            // b can be nullptr when the waiter is scheduled but queued.
            std::unique_lock sl(b->waiter_lock);
            if (b == bw->container.load(std::memory_order_relaxed)) {
                event_waiter_list::remove(*bw);
                bw->container.store(nullptr, std::memory_order_relaxed);
                if (bw->tid) {
                    static_cast<FiberEventWaiterNode *>(bw)->waiter_state = state;
                }
                erased = true;
                break;
            }
        }
        if (erased && wakeup) {
            if (bw->tid) {
                FiberEventWaiterNode *bbw = static_cast<FiberEventWaiterNode *>(bw);
                get_task_group(bbw->control)->ready_to_run_general(bw->tid);
            } else {
                PthreadEventWaiterNode *pw = static_cast<PthreadEventWaiterNode *>(bw);
                wakeup_pthread(pw);
            }
        }
        errno = saved_errno;
        return erased;
    }

    static void wait_for_event(void *arg) {
        FiberEventWaiterNode *const bw = static_cast<FiberEventWaiterNode *>(arg);
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
            turbo::SpinLockHolder sl(&b->waiter_lock);
            if (b->value.load(std::memory_order_relaxed) != bw->expected_value) {
                bw->waiter_state = WAITER_STATE_UNMATCHEDVALUE;
            } else if (bw->waiter_state == WAITER_STATE_READY/*1*/ &&
                       !bw->task_meta->interrupted) {
                b->waiters.push_back(*bw);
                bw->container.store(b, std::memory_order_relaxed);
                bw->sleep_id = get_fiber_timer_thread()->schedule(
                        erase_from_event_and_wakeup, bw, bw->abstime);
                if (!bw->sleep_id) {  // TimerThread stopped.
                    erase_from_event_and_wakeup(bw);
                }
                return;
            }
        }

        // b->container is nullptr which makes erase_from_event_and_wakeup() and
        // FiberWorker::interrupt() no-op, there's no race between following code and
        // the two functions. The on-stack FiberEventWaiterNode is safe to use and
        // bw->waiter_state will not change again.
        unsleep_if_necessary(bw, get_fiber_timer_thread());
        tls_task_group->ready_to_run(bw->tid);
        // FIXME: jump back to original thread is buggy.

        // // Value unmatched or waiter is already woken up by TimerThread, jump
        // // back to original fiber.
        // FiberWorker* g = tls_task_group;
        // ReadyToRunArgs args = { g->current_fid(), false };
        // g->set_remained(FiberWorker::ready_to_run_in_worker, &args);
        // // 2: Don't run remained because we're already in a remained function
        // //    otherwise stack may overflow.
        // FiberWorker::sched_to(&g, bw->tid, false/*2*/);
    }

    static turbo::Status
    event_wait_from_pthread(FiberWorker *g, waitable_event *b, int expected_value, turbo::Time abstime) {
        FiberEntity *task = nullptr;
        PthreadEventWaiterNode pw;
        pw.tid = 0;
        pw.sig.store(PTHREAD_NOT_SIGNALLED, std::memory_order_relaxed);
        turbo::Status rc;

        if (g) {
            task = g->current_fiber();
            task->current_waiter.store(&pw, std::memory_order_release);
        }
        b->waiter_lock.lock();
        if (b->value.load(std::memory_order_relaxed) != expected_value) {
            b->waiter_lock.unlock();
            errno = EWOULDBLOCK;
            return turbo::make_status();
        } else if (task != nullptr && task->interrupted) {
            b->waiter_lock.unlock();
            // Race with set and may consume multiple interruptions, which are OK.
            task->interrupted = false;
            errno = EINTR;
            return turbo::make_status();
        } else {
            b->waiters.push_back(pw);
            pw.container.store(b, std::memory_order_relaxed);
            b->waiter_lock.unlock();
            rc = wait_pthread(pw, abstime);
        }
        if (task) {
            // If current_waiter is nullptr, FiberWorker::interrupt() is running and
            // using pw, spin until current_waiter != nullptr.
            TURBO_LOOP_WHEN(task->current_waiter.exchange(
                    nullptr, std::memory_order_acquire) == nullptr,30/*nops before sched_yield*/);
            if (task->interrupted) {
                task->interrupted = false;
                if (rc.ok()) {
                    errno = EINTR;
                    return make_status();
                }
            }
        }
        return rc;
    }

    turbo::Status waitable_event_wait(void *arg, int expected_value, turbo::Time abstime) {

        if (abstime != turbo::Time::infinite_future()) {
            const auto timeout_duration = abstime - turbo::time_now();
            if (timeout_duration <= MIN_SLEEP) {
                errno = ETIMEDOUT;
                return turbo::make_status();
            }
        }

        waitable_event *b = static_cast<waitable_event *>(arg);
        if (b->value.load(std::memory_order_relaxed) != expected_value) {
            std::atomic_thread_fence(std::memory_order_acquire);
            errno = EWOULDBLOCK;
            turbo::make_status();
        }
        FiberWorker *g = tls_task_group;
        if (nullptr == g || g->is_current_pthread_task()) {
            return event_wait_from_pthread(g, b, expected_value, abstime);
        }
        FiberEventWaiterNode bbw;
        // tid is 0 iff the thread is non-fiber
        bbw.tid = g->current_fid();
        bbw.container.store(nullptr, std::memory_order_relaxed);
        bbw.task_meta = g->current_fiber();
        bbw.sleep_id = 0;
        bbw.waiter_state = WAITER_STATE_READY;
        bbw.expected_value = expected_value;
        bbw.initial_event = b;
        bbw.control = g->control();
        bbw.abstime = abstime;
        // release fence matches with acquire fence in interrupt_and_consume_waiters
        // in task_group.cpp to guarantee visibility of `interrupted'.
        bbw.task_meta->current_waiter.store(&bbw, std::memory_order_release);
        g->set_remained(wait_for_event, &bbw);
        FiberWorker::sched(&g);

        // erase_from_event_and_wakeup (called by TimerThread) is possibly still
        // running and using bbw. The chance is small, just spin until it's done.
        TURBO_LOOP_WHEN(unsleep_if_necessary(&bbw, get_fiber_timer_thread()) < 0, 30/*nops before sched_yield*/);

        // If current_waiter is nullptr, FiberWorker::interrupt() is running and using bbw.
        // Spin until current_waiter != nullptr.
        TURBO_LOOP_WHEN(bbw.task_meta->current_waiter.exchange(
                nullptr, std::memory_order_acquire) == nullptr, 30/*nops before sched_yield*/);

        bool is_interrupted = false;
        if (bbw.task_meta->interrupted) {
            // Race with set and may consume multiple interruptions, which are OK.
            bbw.task_meta->interrupted = false;
            is_interrupted = true;
        }
        // If timed out as well as value unmatched, return ETIMEDOUT.
        if (WAITER_STATE_TIMEDOUT == bbw.waiter_state) {
            errno = ETIMEDOUT;
            return turbo::make_status();
        } else if (WAITER_STATE_UNMATCHEDVALUE == bbw.waiter_state) {
            errno = EWOULDBLOCK;
            return turbo::make_status();
        } else if (is_interrupted) {
            errno = EINTR;
            return turbo::make_status();
        }
        return turbo::ok_status();
    }

}  // namespace turbo::fiber_internal

namespace turbo {
    template<>
    struct ObjectPoolTraits<turbo::fiber_internal::waitable_event>
            : public ObjectPoolTraitsBase<turbo::fiber_internal::waitable_event> {
        static const size_t value = 128;

        static constexpr size_t block_max_items() {
            return 128;
        }
    };
}
