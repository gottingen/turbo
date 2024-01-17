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

#include <sys/types.h>
#include <stddef.h>
#include <inttypes.h>
#include "turbo/random/random.h"
#include <memory>
#include "turbo/hash/hash.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/base/processor.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/internal/timer.h"
#include "turbo/fiber/internal/offset_table.h"
#include "turbo/log/logging.h"
#include "turbo/system/sysinfo.h"

namespace turbo::fiber_internal {

    static bool pass_bool(const char *, bool) { return true; }

    __thread FiberWorker *tls_task_group = nullptr;
    // Sync with FiberEntity::local_storage when a fiber is created or destroyed.
    // During running, the two fields may be inconsistent, use tls_bls as the
    // groundtruth.
    thread_local fiber_local_storage tls_bls = FIBER_LOCAL_STORAGE_INITIALIZER;

    // defined in fiber/key.cpp
    extern void return_keytable(fiber_keytable_pool_t *, KeyTable *);

    // [Hacky] This is a special TLS set by fiber-rpc privately... to save
    // overhead of creation keytable, may be removed later.
    TURBO_THREAD_LOCAL void *tls_unique_user_ptr = nullptr;

    const FiberStatistics EMPTY_STAT = {0, 0};


    int FiberWorker::get_attr(fiber_id_t tid, FiberAttribute *out) {
        FiberEntity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            SpinLockHolder l(&m->version_lock);
            if (given_ver == *m->version_futex) {
                *out = m->attr;
                return 0;
            }
        }
        errno = EINVAL;
        return -1;
    }

    void FiberWorker::set_stopped(fiber_id_t tid) {
        FiberEntity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            SpinLockHolder l(&m->version_lock);
            if (given_ver == *m->version_futex) {
                m->stop = true;
            }
        }
    }

    bool FiberWorker::is_stopped(fiber_id_t tid) {
        FiberEntity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            SpinLockHolder l(&m->version_lock);
            if (given_ver == *m->version_futex) {
                return m->stop;
            }
        }
        // If the tid does not exist or version does not match, it's intuitive
        // to treat the thread as "stopped".
        return true;
    }

    bool FiberWorker::wait_task(fiber_id_t *tid) {
        do {
#ifndef FIBER_DONT_SAVE_PARKING_STATE
            if (_last_pl_state.stopped()) {
                return false;
            }
            _pl->wait(_last_pl_state);
            if (steal_task(tid)) {
                return true;
            }
#else
            const ParkingLot::State st = _pl->get_state();
            if (st.stopped()) {
                return false;
            }
            if (steal_task(tid)) {
                return true;
            }
            _pl->wait(st);
#endif
        } while (true);
    }

    static double get_cumulated_cputime_from_this(void *arg) {
        return static_cast<FiberWorker *>(arg)->cumulated_cputime_ns() / 1000000000.0;
    }

    void FiberWorker::run_main_task() {
        FiberWorker *dummy = this;
        fiber_id_t tid;
        while (wait_task(&tid)) {
            FiberWorker::sched_to(&dummy, tid);
            TDLOG_CHECK_EQ(this, dummy);
            TDLOG_CHECK_EQ(_cur_meta->stack, _main_stack);
            if (_cur_meta->tid != _main_tid) {
                FiberWorker::task_runner(1/*skip remained*/);
            }
        }
        // Don't forget to add elapse of last wait_task.
        current_task()->stat.cputime_ns += turbo::get_current_time_nanos() - _last_run_ns;
    }

    FiberWorker::FiberWorker(ScheduleGroup *c)
            :
#ifndef NDEBUG
            _sched_recursive_guard(0),
#endif
            _cur_meta(nullptr), _control(c), _num_nosignal(0), _nsignaled(0),
            _last_run_ns(turbo::get_current_time_nanos()),
            _cumulated_cputime_ns(0), _nswitch(0), _last_context_remained(nullptr), _last_context_remained_arg(nullptr),
            _pl(nullptr), _main_stack(nullptr), _main_tid(0), _remote_num_nosignal(0), _remote_nsignaled(0) {
        _steal_seed = turbo::fast_uniform<size_t>();
        _steal_offset = get_table_offset(_steal_offset);
        _pl = &c->_pl[turbo::hash_mixer8(thread_numeric_id()) % ScheduleGroup::PARKING_LOT_NUM];
        TLOG_CHECK(c);
    }

    FiberWorker::~FiberWorker() {
        if (_main_tid) {
            FiberEntity *m = address_meta(_main_tid);
            TDLOG_CHECK(_main_stack == m->stack);
            return_stack(m->release_stack());
            return_resource(get_slot(_main_tid));
            _main_tid = 0;
        }
    }

    int FiberWorker::init(size_t runqueue_capacity) {
        if (_rq.init(runqueue_capacity) != 0) {
            TLOG_CRITICAL("Fail to init _rq");
            return -1;
        }
        if (_remote_rq.init(runqueue_capacity / 2) != 0) {
            TLOG_CRITICAL("Fail to init _remote_rq");
            return -1;
        }
        ContextualStack *stk = get_stack(StackType::STACK_TYPE_MAIN, nullptr);
        if (nullptr == stk) {
            TLOG_CRITICAL("Fail to get main stack container");
            return -1;
        }
        turbo::ResourceId<FiberEntity> slot;
        FiberEntity *m = turbo::get_resource<FiberEntity>(&slot);
        if (nullptr == m) {
            TLOG_CRITICAL("Fail to get FiberEntity");
            return -1;
        }
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = nullptr;
        m->arg = nullptr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = turbo::get_current_time_nanos();
        m->stat = EMPTY_STAT;
        m->attr = FIBER_ATTR_MAIN;
        m->tid = make_tid(*m->version_futex, slot);
        m->set_stack(stk);

        _cur_meta = m;
        _main_tid = m->tid;
        _main_stack = stk;
        _last_run_ns = turbo::get_current_time_nanos();
        return 0;
    }

    void FiberWorker::task_runner(intptr_t skip_remained) {
        // NOTE: tls_task_group is volatile since tasks are moved around
        //       different groups.
        FiberWorker *g = tls_task_group;

        if (!skip_remained) {
            while (g->_last_context_remained) {
                RemainedFn fn = g->_last_context_remained;
                g->_last_context_remained = nullptr;
                fn(g->_last_context_remained_arg);
                g = tls_task_group;
            }

#ifndef NDEBUG
            --g->_sched_recursive_guard;
#endif
        }

        do {
            // A task can be stopped before it gets running, in which case
            // we may skip user function, but that may confuse user:
            // Most tasks have variables to remember running result of the task,
            // which is often initialized to values indicating success. If an
            // user function is never called, the variables will be unchanged
            // however they'd better reflect failures because the task is stopped
            // abnormally.

            // Meta and identifier of the task is persistent in this run.
            FiberEntity *const m = g->_cur_meta;

            // Not catch exceptions except ExitException which is for implementing
            // fiber_exit(). User code is intended to crash when an exception is
            // not caught explicitly. This is consistent with other threading
            // libraries.
            void *thread_return;
            try {
                thread_return = m->fn(m->arg);
            } catch (ExitException &e) {
                thread_return = e.value();
            }

            // Group is probably changed
            g = tls_task_group;

            // TODO: Save thread_return
            (void) thread_return;

            // Logging must be done before returning the keytable, since the logging lib
            // use fiber local storage internally, or will cause memory leak.
            // FIXME: the time from quiting fn to here is not counted into cputime
            if (is_log_start_and_finish(m->attr)) {
                TDLOG_INFO("Finished fiber {} , cputime={}ms", m->tid, m->stat.cputime_ns / 1000000.0);
            }

            // Clean tls variables, must be done before changing version_futex
            // otherwise another thread just joined this thread may not see side
            // effects of destructing tls variables.
            KeyTable *kt = tls_bls.keytable;
            if (kt != nullptr) {
                return_keytable(m->attr.keytable_pool, kt);
                // After deletion: tls may be set during deletion.
                tls_bls.keytable = nullptr;
                m->local_storage.keytable = nullptr; // optional
            }

            // Increase the version and wake up all joiners, if resulting version
            // is 0, change it to 1 to make fiber_id_t never be 0. Any access
            // or join to the fiber after changing version will be rejected.
            // The spinlock is for visibility of FiberWorker::get_attr.
            {
                SpinLockHolder l(&m->version_lock);
                if (0 == ++*m->version_futex) {
                    ++*m->version_futex;
                }
            }
            waitable_event_wake_except(m->version_futex, 0);

            g->set_remained(FiberWorker::_release_last_context, m);
            ending_sched(&g);

        } while (g->_cur_meta->tid != g->_main_tid);

    }

    void FiberWorker::_release_last_context(void *arg) {
        auto *m = static_cast<FiberEntity *>(arg);
        if (m->stack_type() != StackType::STACK_TYPE_PTHREAD) {
            return_stack(m->release_stack()/*may be nullptr*/);
        } else {
            // it's _main_stack, don't return.
            m->set_stack(nullptr);
        }
        return_resource(get_slot(m->tid));
    }

    turbo::Status FiberWorker::start_foreground(FiberWorker **pg,
                                       fiber_id_t *__restrict th,
                                       const FiberAttribute *__restrict attr,
                                       std::function<void *(void *)> &&fn,
                                       void *__restrict arg) {
        if (TURBO_UNLIKELY(!fn)) {
            return turbo::make_status(kEINVAL);
        }
        const int64_t start_ns = turbo::get_current_time_nanos();
        const FiberAttribute using_attr = (attr ? *attr : FIBER_ATTR_NORMAL);
        turbo::ResourceId<FiberEntity> slot;
        FiberEntity *m = turbo::get_resource(&slot);
        if (TURBO_UNLIKELY(!m)) {
            return turbo::make_status(kENOMEM);
        }
        TDLOG_CHECK(m->current_waiter.load(std::memory_order_relaxed) == nullptr);
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = std::move(fn);
        m->arg = arg;
        TDLOG_CHECK(m->stack == nullptr);
        m->attr = using_attr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = start_ns;
        m->stat = EMPTY_STAT;
        m->tid = make_tid(*m->version_futex, slot);
        *th = m->tid;
        if (is_log_start_and_finish(using_attr)) {
            TDLOG_INFO("Started fiber {}", m->tid);
        }

        FiberWorker *g = *pg;
        if (g->is_current_pthread_task()) {
            // never create foreground task in pthread.
            g->ready_to_run(m->tid, is_nosignal(using_attr));
        } else {
            // NOSIGNAL affects current task, not the new task.
            RemainedFn functor = nullptr;
            if (g->current_task()->about_to_quit) {
                functor = ready_to_run_in_worker_ignoresignal;
            } else {
                functor = ready_to_run_in_worker;
            }
            ReadyToRunArgs args = {
                    g->current_fid(),
                    is_nosignal(using_attr)
            };
            g->set_remained(functor, &args);
            FiberWorker::sched_to(pg, m->tid);
        }
        return turbo::ok_status();
    }

    template<bool REMOTE>
    turbo::Status FiberWorker::start_background(fiber_id_t *__restrict th,
                                       const FiberAttribute *__restrict attr,
                                       std::function<void *(void *)> &&fn,
                                       void *__restrict arg) {
        if (TURBO_UNLIKELY(!fn)) {
            return turbo::make_status(kEINVAL);
        }
        const int64_t start_ns = turbo::get_current_time_nanos();
        const FiberAttribute using_attr = (attr ? *attr : FIBER_ATTR_NORMAL);
        turbo::ResourceId<FiberEntity> slot;
        FiberEntity *m = turbo::get_resource(&slot);
        if (TURBO_UNLIKELY(!m)) {
            return turbo::make_status(kENOMEM);
        }
        TDLOG_CHECK(m->current_waiter.load(std::memory_order_relaxed) == nullptr);
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = std::move(fn);
        m->arg = arg;
        TDLOG_CHECK(m->stack == nullptr);
        m->attr = using_attr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = start_ns;
        m->stat = EMPTY_STAT;
        m->tid = make_tid(*m->version_futex, slot);
        *th = m->tid;
        if (is_log_start_and_finish(using_attr)) {
            TDLOG_INFO("Started fiber {}", m->tid);
        }
        if (REMOTE) {
            ready_to_run_remote(m->tid, is_nosignal(using_attr));
        } else {
            ready_to_run(m->tid, is_nosignal(using_attr));
        }
        return turbo::ok_status();
    }

    // Explicit instantiations.
    template turbo::Status
    FiberWorker::start_background<true>(fiber_id_t *__restrict th,
                                         const FiberAttribute *__restrict attr,
                                         std::function<void *(void *)> &&fn,
                                         void *__restrict arg);

    template turbo::Status
    FiberWorker::start_background<false>(fiber_id_t *__restrict th,
                                          const FiberAttribute *__restrict attr,
                                          std::function<void *(void *)> &&fn,
                                          void *__restrict arg);

    turbo::Status FiberWorker::join(fiber_id_t tid, void **return_value) {
        if (TURBO_UNLIKELY(!tid)) {  // tid of fiber is never 0.
            errno = EINVAL;
            return turbo::make_status();
        }
        FiberEntity *m = address_meta(tid);
        if (TURBO_UNLIKELY(!m)) {
            // The fiber is not created yet, this join is definitely wrong.
            errno = EINVAL;
            return turbo::make_status();
        }
        FiberWorker *g = tls_task_group;
        if (g != nullptr && g->current_fid() == tid) {
            // joining self causes indefinite waiting.
            errno = EINVAL;
            return turbo::make_status();
        }
        const uint32_t expected_version = get_version(tid);
        while (*m->version_futex == expected_version) {
            auto rs = waitable_event_wait(m->version_futex, expected_version);
            if (!rs.ok() && rs.code() != EWOULDBLOCK && rs.code() != kEINTR) {
                return rs;
            }
        }
        if (return_value) {
            *return_value = nullptr;
        }
        return ok_status();
    }

    bool FiberWorker::exists(fiber_id_t tid) {
        if (tid != 0) {  // tid of fiber is never 0.
            FiberEntity *m = address_meta(tid);
            if (m != nullptr) {
                return (*m->version_futex == get_version(tid));
            }
        }
        return false;
    }

    FiberStatistics FiberWorker::main_stat() const {
        FiberEntity *m = address_meta(_main_tid);
        return m ? m->stat : EMPTY_STAT;
    }

    void FiberWorker::ending_sched(FiberWorker **pg) {
        FiberWorker *g = *pg;
        fiber_id_t next_tid = 0;
        // Find next task to run, if none, switch to idle thread of the group.
#ifndef FIBER_FAIR_WSQ
        // When FIBER_FAIR_WSQ is defined, profiling shows that cpu cost of
        // WSQ::steal() in example/multi_threaded_echo_c++ changes from 1.9%
        // to 2.9%
        const bool popped = g->_rq.pop(&next_tid);
#else
        const bool popped = g->_rq.steal(&next_tid);
#endif
        if (!popped && !g->steal_task(&next_tid)) {
            // Jump to main task if there's no task to run.
            next_tid = g->_main_tid;
        }

        FiberEntity *const cur_meta = g->_cur_meta;
        FiberEntity *next_meta = address_meta(next_tid);
        if (next_meta->stack == nullptr) {
            if (next_meta->stack_type() == cur_meta->stack_type()) {
                // also works with pthread_task scheduling to pthread_task, the
                // transfered stack is just _main_stack.
                next_meta->set_stack(cur_meta->release_stack());
            } else {
                ContextualStack *stk = get_stack(next_meta->stack_type(), task_runner);
                if (stk) {
                    next_meta->set_stack(stk);
                } else {
                    next_meta->attr.stack_type = StackType::STACK_TYPE_PTHREAD;
                    next_meta->set_stack(g->_main_stack);
                }
            }
        }
        sched_to(pg, next_meta);
    }

    void FiberWorker::sched(FiberWorker **pg) {
        FiberWorker *g = *pg;
        fiber_id_t next_tid = 0;
        // Find next task to run, if none, switch to idle thread of the group.
#ifndef FIBER_FAIR_WSQ
        const bool popped = g->_rq.pop(&next_tid);
#else
        const bool popped = g->_rq.steal(&next_tid);
#endif
        if (!popped && !g->steal_task(&next_tid)) {
            // Jump to main task if there's no task to run.
            next_tid = g->_main_tid;
        }
        sched_to(pg, next_tid);
    }

    void FiberWorker::sched_to(FiberWorker **pg, FiberEntity *next_meta) {
        FiberWorker *g = *pg;
#ifndef NDEBUG
        if ((++g->_sched_recursive_guard) > 1) {
            TLOG_CRITICAL("Recursively({}) call sched_to({})", g->_sched_recursive_guard - 1, turbo::ptr(g));
        }
#endif
        // Save errno so that errno is fiber-specific.
        const int saved_errno = errno;
        void *saved_unique_user_ptr = tls_unique_user_ptr;

        FiberEntity *const cur_meta = g->_cur_meta;
        const int64_t now = turbo::get_current_time_nanos();
        const int64_t elp_ns = now - g->_last_run_ns;
        g->_last_run_ns = now;
        cur_meta->stat.cputime_ns += elp_ns;
        if (cur_meta->tid != g->main_tid()) {
            g->_cumulated_cputime_ns += elp_ns;
        }
        ++cur_meta->stat.nswitch;
        ++g->_nswitch;
        // Switch to the task
        if (TURBO_LIKELY(next_meta != cur_meta)) {
            g->_cur_meta = next_meta;
            // Switch tls_bls
            cur_meta->local_storage = tls_bls;
            tls_bls = next_meta->local_storage;

            // Logging must be done after switching the local storage, since the logging lib
            // use fiber local storage internally, or will cause memory leak.
            if (is_log_context_switch(cur_meta->attr) ||
                    is_log_context_switch(next_meta->attr)) {
                TDLOG_INFO("Switch fiber: {} -> {}", cur_meta->tid, next_meta->tid);
            }

            if (cur_meta->stack != nullptr) {
                if (next_meta->stack != cur_meta->stack) {
                    jump_stack(cur_meta->stack, next_meta->stack);
                    // probably went to another group, need to assign g again.
                    g = tls_task_group;
                }
#ifndef NDEBUG
                else {
                    // else pthread_task is switching to another pthread_task, sc
                    // can only equal when they're both _main_stack
                    TDLOG_CHECK(cur_meta->stack == g->_main_stack);
                }
#endif
            }
            // else because of ending_sched(including pthread_task->pthread_task)
        } else {
            TLOG_CRITICAL("fiber={}  sched_to itself!", g->current_fid());
        }

        while (g->_last_context_remained) {
            RemainedFn fn = g->_last_context_remained;
            g->_last_context_remained = nullptr;
            fn(g->_last_context_remained_arg);
            g = tls_task_group;
        }

        // Restore errno
        errno = saved_errno;
        tls_unique_user_ptr = saved_unique_user_ptr;

#ifndef NDEBUG
        --g->_sched_recursive_guard;
#endif
        *pg = g;
    }

    void FiberWorker::destroy_self() {
        if (_control) {
            _control->_destroy_group(this);
            _control = nullptr;
        } else {
            TDLOG_CHECK(false);
        }
    }

    void FiberWorker::ready_to_run(fiber_id_t tid, bool nosignal) {
        push_rq(tid);
        if (nosignal) {
            ++_num_nosignal;
        } else {
            const int additional_signal = _num_nosignal;
            _num_nosignal = 0;
            _nsignaled += 1 + additional_signal;
            _control->signal_task(1 + additional_signal);
        }
    }

    void FiberWorker::flush_nosignal_tasks() {
        const int val = _num_nosignal;
        if (val) {
            _num_nosignal = 0;
            _nsignaled += val;
            _control->signal_task(val);
        }
    }

    void FiberWorker::ready_to_run_remote(fiber_id_t tid, bool nosignal) {
        _remote_rq._mutex.lock();
        while (!_remote_rq.push_locked(tid)) {
            flush_nosignal_tasks_remote_locked(_remote_rq._mutex);
            TLOG_ERROR_EVERY_SEC("_remote_rq is full, capacity={}", _remote_rq.capacity());
            turbo::sleep_for(turbo::Duration::milliseconds(1));
            _remote_rq._mutex.lock();
        }
        if (nosignal) {
            ++_remote_num_nosignal;
            _remote_rq._mutex.unlock();
        } else {
            const int additional_signal = _remote_num_nosignal;
            _remote_num_nosignal = 0;
            _remote_nsignaled += 1 + additional_signal;
            _remote_rq._mutex.unlock();
            _control->signal_task(1 + additional_signal);
        }
    }

    void FiberWorker::flush_nosignal_tasks_remote_locked(std::mutex &locked_mutex) {
        const int val = _remote_num_nosignal;
        if (!val) {
            locked_mutex.unlock();
            return;
        }
        _remote_num_nosignal = 0;
        _remote_nsignaled += val;
        locked_mutex.unlock();
        _control->signal_task(val);
    }

    void FiberWorker::ready_to_run_general(fiber_id_t tid, bool nosignal) {
        if (tls_task_group == this) {
            return ready_to_run(tid, nosignal);
        }
        return ready_to_run_remote(tid, nosignal);
    }

    void FiberWorker::flush_nosignal_tasks_general() {
        if (tls_task_group == this) {
            return flush_nosignal_tasks();
        }
        return flush_nosignal_tasks_remote();
    }

    void FiberWorker::ready_to_run_in_worker(void *args_in) {
        ReadyToRunArgs *args = static_cast<ReadyToRunArgs *>(args_in);
        return tls_task_group->ready_to_run(args->tid, args->nosignal);
    }

    void FiberWorker::ready_to_run_in_worker_ignoresignal(void *args_in) {
        ReadyToRunArgs *args = static_cast<ReadyToRunArgs *>(args_in);
        return tls_task_group->push_rq(args->tid);
    }

    struct SleepArgs {
        uint64_t timeout_us;
        fiber_id_t tid;
        FiberEntity *meta;
        FiberWorker *group;
    };

    static void ready_to_run_from_timer_thread(void *arg) {
        TLOG_CHECK(tls_task_group == nullptr);
        const SleepArgs *e = static_cast<const SleepArgs *>(arg);
        e->group->control()->choose_one_group()->ready_to_run_remote(e->tid);
    }

    void FiberWorker::_add_sleep_event(void *void_args) {
        // Must copy SleepArgs. After calling TimerThread::schedule(), previous
        // thread may be stolen by a worker immediately and the on-stack SleepArgs
        // will be gone.
        SleepArgs e = *static_cast<SleepArgs *>(void_args);
        FiberWorker *g = e.group;

        TimerThread::TaskId sleep_id;
        sleep_id = get_fiber_timer_thread()->schedule(ready_to_run_from_timer_thread, void_args,
                turbo::microseconds_from_now(e.timeout_us));

        if (!sleep_id) {
            // fail to schedule timer, go back to previous thread.
            g->ready_to_run(e.tid);
            return;
        }

        // Set FiberEntity::current_sleep which is for interruption.
        const uint32_t given_ver = get_version(e.tid);
        {
            SpinLockHolder l(&e.meta->version_lock);
            if (given_ver == *e.meta->version_futex && !e.meta->interrupted) {
                e.meta->current_sleep = sleep_id;
                return;
            }
        }
        // The thread is stopped or interrupted.
        // interrupt() always sees that current_sleep == 0. It will not schedule
        // the calling thread. The race is between current thread and timer thread.
        if (get_fiber_timer_thread()->unschedule(sleep_id).ok()) {
            // added to timer, previous thread may be already woken up by timer and
            // even stopped. It's safe to schedule previous thread when unschedule()
            // returns 0 which means "the not-run-yet sleep_id is removed". If the
            // sleep_id is running(returns 1), ready_to_run_in_worker() will
            // schedule previous thread as well. If sleep_id does not exist,
            // previous thread is scheduled by timer thread before and we don't
            // have to do it again.
            g->ready_to_run(e.tid);
        }
    }

    // To be consistent with sys_usleep, set errno and return -1 on error.
    int FiberWorker::usleep(FiberWorker **pg, uint64_t timeout_us) {
        if (0 == timeout_us) {
            yield(pg);
            return 0;
        }
        FiberWorker *g = *pg;
        // We have to schedule timer after we switched to next fiber otherwise
        // the timer may wake up(jump to) current still-running context.
        SleepArgs e = {timeout_us, g->current_fid(), g->current_task(), g};
        g->set_remained(_add_sleep_event, &e);
        sched(pg);
        g = *pg;
        e.meta->current_sleep = 0;
        if (e.meta->interrupted) {
            // Race with set and may consume multiple interruptions, which are OK.
            e.meta->interrupted = false;
            // NOTE: setting errno to ESTOP is not necessary from fiber's
            // pespective, however many RPC code expects turbo::fiber_sleep_for to set
            // errno to ESTOP when the thread is stopping, and print FATAL
            // otherwise. To make smooth transitions, ESTOP is still set instead
            // of EINTR when the thread is stopping.
            errno = (e.meta->stop ? 20 : EINTR);
            return -1;
        }
        return 0;
    }

    turbo::Status FiberWorker::sleep(FiberWorker **pg, const turbo::Time & deadline) {
        const turbo::Time now = turbo::time_now();
        if (now >= deadline) {
            yield(pg);
            return turbo::ok_status();
        }
        return sleep(pg, deadline - now);
    }

    turbo::Status FiberWorker::sleep(FiberWorker **pg, const turbo::Duration & span) {
        if (turbo::Duration::zero() == span) {
            yield(pg);
            return turbo::ok_status();
        }
        FiberWorker *g = *pg;
        // We have to schedule timer after we switched to next fiber otherwise
        // the timer may wake up(jump to) current still-running context.
        SleepArgs e = {static_cast<uint64_t>(span.to_microseconds()), g->current_fid(), g->current_task(), g};
        g->set_remained(_add_sleep_event, &e);
        sched(pg);
        g = *pg;
        e.meta->current_sleep = 0;
        if (e.meta->interrupted) {
            // Race with set and may consume multiple interruptions, which are OK.
            e.meta->interrupted = false;
            // NOTE: setting errno to ESTOP is not necessary from fiber's
            // pespective, however many RPC code expects turbo::fiber_sleep_for to set
            // errno to ESTOP when the thread is stopping, and print FATAL
            // otherwise. To make smooth transitions, ESTOP is still set instead
            // of EINTR when the thread is stopping.
            errno =  e.meta->stop ? ESTOP : EINTR;
            return turbo::make_status();
        }
        return turbo::ok_status();
    }

    bool erase_from_event_because_of_interruption(EventWaiterNode *bw);

    static turbo::Status interrupt_and_consume_waiters(
            fiber_id_t tid, EventWaiterNode **pw, uint64_t *sleep_id) {
        FiberEntity *const m = FiberWorker::address_meta(tid);
        if (m == nullptr) {
            return turbo::make_status(kEINVAL);
        }
        const uint32_t given_ver = get_version(tid);
        SpinLockHolder l(&m->version_lock);
        if (given_ver == *m->version_futex) {
            *pw = m->current_waiter.exchange(nullptr, std::memory_order_acquire);
            *sleep_id = m->current_sleep;
            m->current_sleep = 0;  // only one stopper gets the sleep_id
            m->interrupted = true;
            return turbo::ok_status();
        }
        return turbo::make_status(kEINVAL);
    }

    static turbo::Status set_event_waiter(fiber_id_t tid, EventWaiterNode *w) {
        FiberEntity *const m = FiberWorker::address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            SpinLockHolder l(&m->version_lock);
            if (given_ver == *m->version_futex) {
                // Release fence makes m->interrupted visible to waitable_event_wait
                m->current_waiter.store(w, std::memory_order_release);
                return turbo::ok_status();
            }
        }
        return turbo::make_status(kEINVAL);
    }

    // The interruption is "persistent" compared to the ones caused by signals,
    // namely if a fiber is interrupted when it's not blocked, the interruption
    // is still remembered and will be checked at next blocking. This designing
    // choice simplifies the implementation and reduces notification loss caused
    // by race conditions.
    // TODO: fibers created by FIBER_ATTR_PTHREAD blocking on turbo::fiber_sleep_for()
    // can't be interrupted.
    turbo::Status FiberWorker::interrupt(fiber_id_t tid, ScheduleGroup *c) {
        // Consume current_waiter in the FiberEntity, wake it up then set it back.
        EventWaiterNode *w = nullptr;
        uint64_t sleep_id = 0;
        auto rc = interrupt_and_consume_waiters(tid, &w, &sleep_id);
        if (!rc.ok()) {
            return rc;
        }
        // a fiber cannot wait on a futex and be sleepy at the same time.
        TLOG_CHECK(!sleep_id || !w);
        if (w != nullptr) {
            erase_from_event_because_of_interruption(w);
            // If waitable_event_wait() already wakes up before we set current_waiter back,
            // the function will spin until current_waiter becomes non-nullptr.
            rc = set_event_waiter(tid, w);
            if (!rc.ok()) {
                TLOG_CRITICAL("waitable_event_wait should spin until setting back waiter");
                return rc;
            }
        } else if (sleep_id != 0) {
            if (get_fiber_timer_thread()->unschedule(sleep_id).ok()) {
                turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
                if (g) {
                    g->ready_to_run(tid);
                } else {
                    if (!c) {
                        return turbo::make_status(kEINVAL);
                    }
                    c->choose_one_group()->ready_to_run_remote(tid);
                }
            }
        }
        return turbo::ok_status();
    }

    void FiberWorker::yield(FiberWorker **pg) {
        FiberWorker *g = *pg;
        ReadyToRunArgs args = {g->current_fid(), false};
        g->set_remained(ready_to_run_in_worker, &args);
        sched(pg);
    }

    void print_task(std::ostream &os, fiber_id_t tid) {
        FiberEntity *const m = FiberWorker::address_meta(tid);
        if (m == nullptr) {
            os << "fiber=" << tid << " : never existed";
            return;
        }
        const uint32_t given_ver = get_version(tid);
        bool matched = false;
        bool stop = false;
        bool interrupted = false;
        bool about_to_quit = false;
        std::function<void *(void *)> *fn = nullptr;
        void *arg = nullptr;
        FiberAttribute attr = FIBER_ATTR_NORMAL;
        bool has_tls = false;
        int64_t cpuwide_start_ns = 0;
        FiberStatistics stat = {0, 0};
        {
            SpinLockHolder l(&m->version_lock);
            if (given_ver == *m->version_futex) {
                matched = true;
                stop = m->stop;
                interrupted = m->interrupted;
                about_to_quit = m->about_to_quit;
                fn = &m->fn;
                arg = m->arg;
                attr = m->attr;
                has_tls = m->local_storage.keytable;
                cpuwide_start_ns = m->cpuwide_start_ns;
                stat = m->stat;
            }
        }
        if (!matched) {
            os << "fiber=" << tid << " : not exist now";
        } else {
            os << "fiber=" << tid << " :\nstop=" << stop
               << "\ninterrupted=" << interrupted
               << "\nabout_to_quit=" << about_to_quit
               << "\nfn=" << (void *) fn
               << "\narg=" << (void *) arg
               << "\nattr={stack_type=" << turbo::format("{}",attr.stack_type)
               << " flags=" <<  turbo::format("{}",attr.flags)
               << " keytable_pool=" << attr.keytable_pool
               << "}\nhas_tls=" << has_tls
               << "\nuptime_ns=" << turbo::get_current_time_nanos() - cpuwide_start_ns
               << "\ncputime_ns=" << stat.cputime_ns
               << "\nnswitch=" << stat.nswitch;
        }
    }

}  // namespace turbo::fiber_internal
