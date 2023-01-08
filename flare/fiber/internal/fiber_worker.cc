
#include <sys/types.h>
#include <stddef.h>                         // size_t
#include <inttypes.h>
#include <gflags/gflags.h>
#include "flare/base/compat.h"                   // FLARE_PLATFORM_OSX
#include "flare/base/scoped_lock.h"              // FLARE_SCOPED_LOCK
#include "flare/base/fast_rand.h"
#include <memory>
#include "flare/hash/murmurhash3.h" // fmix64
#include "flare/fiber/internal/errno.h"                  // ESTOP
#include "flare/fiber/internal/waitable_event.h"                  // butex_*
#include "flare/fiber/internal/sys_futex.h"              // futex_wake_private
#include "flare/fiber/internal/processor.h"              // cpu_relax
#include "flare/fiber/internal/schedule_group.h"
#include "flare/fiber/internal/fiber_worker.h"
#include "flare/fiber/internal/timer_thread.h"
#include "flare/fiber/internal/errno.h"

namespace flare::fiber_internal {

    static const fiber_attribute FIBER_ATTR_TASKGROUP = {
            FIBER_STACKTYPE_UNKNOWN, 0, nullptr};

    static bool pass_bool(const char *, bool) { return true; }

    DEFINE_bool(show_fiber_creation_in_vars, false, "When this flags is on, The time "
                                                    "from fiber creation to first run will be recorded and shown "
                                                    "in /vars");
    const bool FLARE_ALLOW_UNUSED dummy_show_fiber_creation_in_vars =
            ::google::RegisterFlagValidator(&FLAGS_show_fiber_creation_in_vars,
                                               pass_bool);

    DEFINE_bool(show_per_worker_usage_in_vars, false,
                "Show per-worker usage in /vars/fiber_per_worker_usage_<tid>");
    const bool FLARE_ALLOW_UNUSED dummy_show_per_worker_usage_in_vars =
            ::google::RegisterFlagValidator(&FLAGS_show_per_worker_usage_in_vars,
                                               pass_bool);

    __thread fiber_worker *tls_task_group = nullptr;
    // Sync with fiber_entity::local_storage when a fiber is created or destroyed.
    // During running, the two fields may be inconsistent, use tls_bls as the
    // groundtruth.
    thread_local fiber_local_storage tls_bls = FIBER_LOCAL_STORAGE_INITIALIZER;

    // defined in fiber/key.cpp
    extern void return_keytable(fiber_keytable_pool_t *, KeyTable *);

    // [Hacky] This is a special TLS set by fiber-rpc privately... to save
    // overhead of creation keytable, may be removed later.
    FLARE_THREAD_LOCAL void *tls_unique_user_ptr = nullptr;

    const fiber_statistics EMPTY_STAT = {0, 0};

    const size_t OFFSET_TABLE[] = {
#include "flare/fiber/internal/offset_inl.list"
    };

    int fiber_worker::get_attr(fiber_id_t tid, fiber_attribute *out) {
        fiber_entity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            FLARE_SCOPED_LOCK(m->version_lock);
            if (given_ver == *m->version_butex) {
                *out = m->attr;
                return 0;
            }
        }
        errno = EINVAL;
        return -1;
    }

    void fiber_worker::set_stopped(fiber_id_t tid) {
        fiber_entity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            FLARE_SCOPED_LOCK(m->version_lock);
            if (given_ver == *m->version_butex) {
                m->stop = true;
            }
        }
    }

    bool fiber_worker::is_stopped(fiber_id_t tid) {
        fiber_entity *const m = address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            FLARE_SCOPED_LOCK(m->version_lock);
            if (given_ver == *m->version_butex) {
                return m->stop;
            }
        }
        // If the tid does not exist or version does not match, it's intuitive
        // to treat the thread as "stopped".
        return true;
    }

    bool fiber_worker::wait_task(fiber_id_t *tid) {
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
        return static_cast<fiber_worker *>(arg)->cumulated_cputime_ns() / 1000000000.0;
    }

    void fiber_worker::run_main_task() {
        flare::status_gauge<double> cumulated_cputime(
                get_cumulated_cputime_from_this, this);
        std::unique_ptr<flare::per_second<flare::status_gauge<double> > > usage_variable;

        fiber_worker *dummy = this;
        fiber_id_t tid;
        while (wait_task(&tid)) {
            fiber_worker::sched_to(&dummy, tid);
            FLARE_DCHECK_EQ(this, dummy);
            FLARE_DCHECK_EQ(_cur_meta->stack, _main_stack);
            if (_cur_meta->tid != _main_tid) {
                fiber_worker::task_runner(1/*skip remained*/);
            }
            if (FLAGS_show_per_worker_usage_in_vars && !usage_variable) {
                char name[32];
#if defined(FLARE_PLATFORM_OSX)
                snprintf(name, sizeof(name), "fiber_worker_usage_%" PRIu64,
                         pthread_numeric_id());
#else
                snprintf(name, sizeof(name), "fiber_worker_usage_%ld",
                         (long)syscall(SYS_gettid));
#endif
                usage_variable.reset(new flare::per_second<flare::status_gauge<double> >
                                             (name, &cumulated_cputime, 1));
            }
        }
        // Don't forget to add elapse of last wait_task.
        current_task()->stat.cputime_ns += flare::get_current_time_nanos() - _last_run_ns;
    }

    fiber_worker::fiber_worker(schedule_group *c)
            :
#ifndef NDEBUG
            _sched_recursive_guard(0),
#endif
            _cur_meta(nullptr), _control(c), _num_nosignal(0), _nsignaled(0),
            _last_run_ns(flare::get_current_time_nanos()),
            _cumulated_cputime_ns(0), _nswitch(0), _last_context_remained(nullptr), _last_context_remained_arg(nullptr),
            _pl(nullptr), _main_stack(nullptr), _main_tid(0), _remote_num_nosignal(0), _remote_nsignaled(0) {
        _steal_seed = flare::base::fast_rand();
        _steal_offset = OFFSET_TABLE[_steal_seed % FLARE_ARRAY_SIZE(OFFSET_TABLE)];
        _pl = &c->_pl[flare::hash::fmix64(pthread_numeric_id()) % schedule_group::PARKING_LOT_NUM];
        FLARE_CHECK(c);
    }

    fiber_worker::~fiber_worker() {
        if (_main_tid) {
            fiber_entity *m = address_meta(_main_tid);
            FLARE_CHECK(_main_stack == m->stack);
            return_stack(m->release_stack());
            return_resource(get_slot(_main_tid));
            _main_tid = 0;
        }
    }

    int fiber_worker::init(size_t runqueue_capacity) {
        if (_rq.init(runqueue_capacity) != 0) {
            FLARE_LOG(FATAL) << "Fail to init _rq";
            return -1;
        }
        if (_remote_rq.init(runqueue_capacity / 2) != 0) {
            FLARE_LOG(FATAL) << "Fail to init _remote_rq";
            return -1;
        }
        fiber_contextual_stack *stk = get_stack(STACK_TYPE_MAIN, nullptr);
        if (nullptr == stk) {
            FLARE_LOG(FATAL) << "Fail to get main stack container";
            return -1;
        }
        flare::ResourceId<fiber_entity> slot;
        fiber_entity *m = flare::get_resource<fiber_entity>(&slot);
        if (nullptr == m) {
            FLARE_LOG(FATAL) << "Fail to get fiber_entity";
            return -1;
        }
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = nullptr;
        m->arg = nullptr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = flare::get_current_time_nanos();
        m->stat = EMPTY_STAT;
        m->attr = FIBER_ATTR_TASKGROUP;
        m->tid = make_tid(*m->version_butex, slot);
        m->set_stack(stk);

        _cur_meta = m;
        _main_tid = m->tid;
        _main_stack = stk;
        _last_run_ns = flare::get_current_time_nanos();
        return 0;
    }

    void fiber_worker::task_runner(intptr_t skip_remained) {
        // NOTE: tls_task_group is volatile since tasks are moved around
        //       different groups.
        fiber_worker *g = tls_task_group;

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
            fiber_entity *const m = g->_cur_meta;

            if (FLAGS_show_fiber_creation_in_vars) {
                // NOTE: the thread triggering exposure of pending time may spend
                // considerable time because a single flare::LatencyRecorder
                // contains many variable.
                g->_control->exposed_pending_time() <<
                                                    (flare::get_current_time_nanos() - m->cpuwide_start_ns) / 1000L;
            }

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
            if (m->attr.flags & FIBER_LOG_START_AND_FINISH) {
                FLARE_LOG(INFO) << "Finished fiber " << m->tid << ", cputime="
                          << m->stat.cputime_ns / 1000000.0 << "ms";
            }

            // Clean tls variables, must be done before changing version_butex
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
            // The spinlock is for visibility of fiber_worker::get_attr.
            {
                FLARE_SCOPED_LOCK(m->version_lock);
                if (0 == ++*m->version_butex) {
                    ++*m->version_butex;
                }
            }
            waitable_event_wake_except(m->version_butex, 0);

            g->_control->_nfibers << -1;
            g->set_remained(fiber_worker::_release_last_context, m);
            ending_sched(&g);

        } while (g->_cur_meta->tid != g->_main_tid);

        // Was called from a pthread and we don't have FIBER_STACKTYPE_PTHREAD
        // tasks to run, quit for more tasks.
    }

    void fiber_worker::_release_last_context(void *arg) {
        fiber_entity *m = static_cast<fiber_entity *>(arg);
        if (m->stack_type() != STACK_TYPE_PTHREAD) {
            return_stack(m->release_stack()/*may be nullptr*/);
        } else {
            // it's _main_stack, don't return.
            m->set_stack(nullptr);
        }
        return_resource(get_slot(m->tid));
    }

    int fiber_worker::start_foreground(fiber_worker **pg,
                                       fiber_id_t *__restrict th,
                                       const fiber_attribute *__restrict attr,
                                       std::function<void *(void *)> &&fn,
                                       void *__restrict arg) {
        if (__builtin_expect(!fn, 0)) {
            return EINVAL;
        }
        const int64_t start_ns = flare::get_current_time_nanos();
        const fiber_attribute using_attr = (attr ? *attr : FIBER_ATTR_NORMAL);
        flare::ResourceId<fiber_entity> slot;
        fiber_entity *m = flare::get_resource(&slot);
        if (__builtin_expect(!m, 0)) {
            return ENOMEM;
        }
        FLARE_CHECK(m->current_waiter.load(std::memory_order_relaxed) == nullptr);
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = std::move(fn);
        m->arg = arg;
        FLARE_CHECK(m->stack == nullptr);
        m->attr = using_attr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = start_ns;
        m->stat = EMPTY_STAT;
        m->tid = make_tid(*m->version_butex, slot);
        *th = m->tid;
        if (using_attr.flags & FIBER_LOG_START_AND_FINISH) {
            FLARE_LOG(INFO) << "Started fiber " << m->tid;
        }

        fiber_worker *g = *pg;
        g->_control->_nfibers << 1;
        if (g->is_current_pthread_task()) {
            // never create foreground task in pthread.
            g->ready_to_run(m->tid, (using_attr.flags & FIBER_NOSIGNAL));
        } else {
            // NOSIGNAL affects current task, not the new task.
            RemainedFn fn = nullptr;
            if (g->current_task()->about_to_quit) {
                fn = ready_to_run_in_worker_ignoresignal;
            } else {
                fn = ready_to_run_in_worker;
            }
            ReadyToRunArgs args = {
                    g->current_fid(),
                    (bool) (using_attr.flags & FIBER_NOSIGNAL)
            };
            g->set_remained(fn, &args);
            fiber_worker::sched_to(pg, m->tid);
        }
        return 0;
    }

    template<bool REMOTE>
    int fiber_worker::start_background(fiber_id_t *__restrict th,
                                       const fiber_attribute *__restrict attr,
                                       std::function<void *(void *)> &&fn,
                                       void *__restrict arg) {
        if (__builtin_expect(!fn, 0)) {
            return EINVAL;
        }
        const int64_t start_ns = flare::get_current_time_nanos();
        const fiber_attribute using_attr = (attr ? *attr : FIBER_ATTR_NORMAL);
        flare::ResourceId<fiber_entity> slot;
        fiber_entity *m = flare::get_resource(&slot);
        if (__builtin_expect(!m, 0)) {
            return ENOMEM;
        }
        FLARE_CHECK(m->current_waiter.load(std::memory_order_relaxed) == nullptr);
        m->stop = false;
        m->interrupted = false;
        m->about_to_quit = false;
        m->fn = std::move(fn);
        m->arg = arg;
        FLARE_CHECK(m->stack == nullptr);
        m->attr = using_attr;
        m->local_storage = LOCAL_STORAGE_INIT;
        m->cpuwide_start_ns = start_ns;
        m->stat = EMPTY_STAT;
        m->tid = make_tid(*m->version_butex, slot);
        *th = m->tid;
        if (using_attr.flags & FIBER_LOG_START_AND_FINISH) {
            FLARE_LOG(INFO) << "Started fiber " << m->tid;
        }
        _control->_nfibers << 1;
        if (REMOTE) {
            ready_to_run_remote(m->tid, (using_attr.flags & FIBER_NOSIGNAL));
        } else {
            ready_to_run(m->tid, (using_attr.flags & FIBER_NOSIGNAL));
        }
        return 0;
    }

// Explicit instantiations.
    template int
    fiber_worker::start_background<true>(fiber_id_t *__restrict th,
                                         const fiber_attribute *__restrict attr,
                                         std::function<void *(void *)> &&fn,
                                         void *__restrict arg);

    template int
    fiber_worker::start_background<false>(fiber_id_t *__restrict th,
                                          const fiber_attribute *__restrict attr,
                                          std::function<void *(void *)> &&fn,
                                          void *__restrict arg);

    int fiber_worker::join(fiber_id_t tid, void **return_value) {
        if (__builtin_expect(!tid, 0)) {  // tid of fiber is never 0.
            return EINVAL;
        }
        fiber_entity *m = address_meta(tid);
        if (__builtin_expect(!m, 0)) {
            // The fiber is not created yet, this join is definitely wrong.
            return EINVAL;
        }
        fiber_worker *g = tls_task_group;
        if (g != nullptr && g->current_fid() == tid) {
            // joining self causes indefinite waiting.
            return EINVAL;
        }
        const uint32_t expected_version = get_version(tid);
        while (*m->version_butex == expected_version) {
            if (waitable_event_wait(m->version_butex, expected_version, nullptr) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR) {
                return errno;
            }
        }
        if (return_value) {
            *return_value = nullptr;
        }
        return 0;
    }

    bool fiber_worker::exists(fiber_id_t tid) {
        if (tid != 0) {  // tid of fiber is never 0.
            fiber_entity *m = address_meta(tid);
            if (m != nullptr) {
                return (*m->version_butex == get_version(tid));
            }
        }
        return false;
    }

    fiber_statistics fiber_worker::main_stat() const {
        fiber_entity *m = address_meta(_main_tid);
        return m ? m->stat : EMPTY_STAT;
    }

    void fiber_worker::ending_sched(fiber_worker **pg) {
        fiber_worker *g = *pg;
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

        fiber_entity *const cur_meta = g->_cur_meta;
        fiber_entity *next_meta = address_meta(next_tid);
        if (next_meta->stack == nullptr) {
            if (next_meta->stack_type() == cur_meta->stack_type()) {
                // also works with pthread_task scheduling to pthread_task, the
                // transfered stack is just _main_stack.
                next_meta->set_stack(cur_meta->release_stack());
            } else {
                fiber_contextual_stack *stk = get_stack(next_meta->stack_type(), task_runner);
                if (stk) {
                    next_meta->set_stack(stk);
                } else {
                    // stack_type is FIBER_STACKTYPE_PTHREAD or out of memory,
                    // In latter case, attr is forced to be FIBER_STACKTYPE_PTHREAD.
                    // This basically means that if we can't allocate stack, run
                    // the task in pthread directly.
                    next_meta->attr.stack_type = FIBER_STACKTYPE_PTHREAD;
                    next_meta->set_stack(g->_main_stack);
                }
            }
        }
        sched_to(pg, next_meta);
    }

    void fiber_worker::sched(fiber_worker **pg) {
        fiber_worker *g = *pg;
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

    void fiber_worker::sched_to(fiber_worker **pg, fiber_entity *next_meta) {
        fiber_worker *g = *pg;
#ifndef NDEBUG
        if ((++g->_sched_recursive_guard) > 1) {
            FLARE_LOG(FATAL) << "Recursively(" << g->_sched_recursive_guard - 1
                       << ") call sched_to(" << g << ")";
        }
#endif
        // Save errno so that errno is fiber-specific.
        const int saved_errno = errno;
        void *saved_unique_user_ptr = tls_unique_user_ptr;

        fiber_entity *const cur_meta = g->_cur_meta;
        const int64_t now = flare::get_current_time_nanos();
        const int64_t elp_ns = now - g->_last_run_ns;
        g->_last_run_ns = now;
        cur_meta->stat.cputime_ns += elp_ns;
        if (cur_meta->tid != g->main_tid()) {
            g->_cumulated_cputime_ns += elp_ns;
        }
        ++cur_meta->stat.nswitch;
        ++g->_nswitch;
        // Switch to the task
        if (__builtin_expect(next_meta != cur_meta, 1)) {
            g->_cur_meta = next_meta;
            // Switch tls_bls
            cur_meta->local_storage = tls_bls;
            tls_bls = next_meta->local_storage;

            // Logging must be done after switching the local storage, since the logging lib
            // use fiber local storage internally, or will cause memory leak.
            if ((cur_meta->attr.flags & FIBER_LOG_CONTEXT_SWITCH) ||
                (next_meta->attr.flags & FIBER_LOG_CONTEXT_SWITCH)) {
                FLARE_LOG(INFO) << "Switch fiber: " << cur_meta->tid << " -> "
                          << next_meta->tid;
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
                    FLARE_CHECK(cur_meta->stack == g->_main_stack);
                }
#endif
            }
            // else because of ending_sched(including pthread_task->pthread_task)
        } else {
            FLARE_LOG(FATAL) << "fiber=" << g->current_fid() << " sched_to itself!";
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

    void fiber_worker::destroy_self() {
        if (_control) {
            _control->_destroy_group(this);
            _control = nullptr;
        } else {
            FLARE_CHECK(false);
        }
    }

    void fiber_worker::ready_to_run(fiber_id_t tid, bool nosignal) {
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

    void fiber_worker::flush_nosignal_tasks() {
        const int val = _num_nosignal;
        if (val) {
            _num_nosignal = 0;
            _nsignaled += val;
            _control->signal_task(val);
        }
    }

    void fiber_worker::ready_to_run_remote(fiber_id_t tid, bool nosignal) {
        _remote_rq._mutex.lock();
        while (!_remote_rq.push_locked(tid)) {
            flush_nosignal_tasks_remote_locked(_remote_rq._mutex);
            FLARE_LOG_EVERY_SECOND(ERROR) << "_remote_rq is full, capacity="
                                    << _remote_rq.capacity();
            ::usleep(1000);
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

    void fiber_worker::flush_nosignal_tasks_remote_locked(std::mutex &locked_mutex) {
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

    void fiber_worker::ready_to_run_general(fiber_id_t tid, bool nosignal) {
        if (tls_task_group == this) {
            return ready_to_run(tid, nosignal);
        }
        return ready_to_run_remote(tid, nosignal);
    }

    void fiber_worker::flush_nosignal_tasks_general() {
        if (tls_task_group == this) {
            return flush_nosignal_tasks();
        }
        return flush_nosignal_tasks_remote();
    }

    void fiber_worker::ready_to_run_in_worker(void *args_in) {
        ReadyToRunArgs *args = static_cast<ReadyToRunArgs *>(args_in);
        return tls_task_group->ready_to_run(args->tid, args->nosignal);
    }

    void fiber_worker::ready_to_run_in_worker_ignoresignal(void *args_in) {
        ReadyToRunArgs *args = static_cast<ReadyToRunArgs *>(args_in);
        return tls_task_group->push_rq(args->tid);
    }

    struct SleepArgs {
        uint64_t timeout_us;
        fiber_id_t tid;
        fiber_entity *meta;
        fiber_worker *group;
    };

    static void ready_to_run_from_timer_thread(void *arg) {
        FLARE_CHECK(tls_task_group == nullptr);
        const SleepArgs *e = static_cast<const SleepArgs *>(arg);
        e->group->control()->choose_one_group()->ready_to_run_remote(e->tid);
    }

    void fiber_worker::_add_sleep_event(void *void_args) {
        // Must copy SleepArgs. After calling TimerThread::schedule(), previous
        // thread may be stolen by a worker immediately and the on-stack SleepArgs
        // will be gone.
        SleepArgs e = *static_cast<SleepArgs *>(void_args);
        fiber_worker *g = e.group;

        TimerThread::TaskId sleep_id;
        sleep_id = get_global_timer_thread()->schedule(
                ready_to_run_from_timer_thread, void_args,
                flare::time_point::future_unix_micros(e.timeout_us).to_timespec());

        if (!sleep_id) {
            // fail to schedule timer, go back to previous thread.
            g->ready_to_run(e.tid);
            return;
        }

        // Set fiber_entity::current_sleep which is for interruption.
        const uint32_t given_ver = get_version(e.tid);
        {
            FLARE_SCOPED_LOCK(e.meta->version_lock);
            if (given_ver == *e.meta->version_butex && !e.meta->interrupted) {
                e.meta->current_sleep = sleep_id;
                return;
            }
        }
        // The thread is stopped or interrupted.
        // interrupt() always sees that current_sleep == 0. It will not schedule
        // the calling thread. The race is between current thread and timer thread.
        if (get_global_timer_thread()->unschedule(sleep_id) == 0) {
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
    int fiber_worker::usleep(fiber_worker **pg, uint64_t timeout_us) {
        if (0 == timeout_us) {
            yield(pg);
            return 0;
        }
        fiber_worker *g = *pg;
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
            // pespective, however many RPC code expects flare::fiber_sleep_for to set
            // errno to ESTOP when the thread is stopping, and print FATAL
            // otherwise. To make smooth transitions, ESTOP is still set instead
            // of EINTR when the thread is stopping.
            errno = (e.meta->stop ? ESTOP : EINTR);
            return -1;
        }
        return 0;
    }

// Defined in butex.cpp
    bool erase_from_event_because_of_interruption(fiber_mutex_waiter *bw);

    static int interrupt_and_consume_waiters(
            fiber_id_t tid, fiber_mutex_waiter **pw, uint64_t *sleep_id) {
        fiber_entity *const m = fiber_worker::address_meta(tid);
        if (m == nullptr) {
            return EINVAL;
        }
        const uint32_t given_ver = get_version(tid);
        FLARE_SCOPED_LOCK(m->version_lock);
        if (given_ver == *m->version_butex) {
            *pw = m->current_waiter.exchange(nullptr, std::memory_order_acquire);
            *sleep_id = m->current_sleep;
            m->current_sleep = 0;  // only one stopper gets the sleep_id
            m->interrupted = true;
            return 0;
        }
        return EINVAL;
    }

    static int set_event_waiter(fiber_id_t tid, fiber_mutex_waiter *w) {
        fiber_entity *const m = fiber_worker::address_meta(tid);
        if (m != nullptr) {
            const uint32_t given_ver = get_version(tid);
            FLARE_SCOPED_LOCK(m->version_lock);
            if (given_ver == *m->version_butex) {
                // Release fence makes m->interrupted visible to waitable_event_wait
                m->current_waiter.store(w, std::memory_order_release);
                return 0;
            }
        }
        return EINVAL;
    }

// The interruption is "persistent" compared to the ones caused by signals,
// namely if a fiber is interrupted when it's not blocked, the interruption
// is still remembered and will be checked at next blocking. This designing
// choice simplifies the implementation and reduces notification loss caused
// by race conditions.
// TODO: fibers created by FIBER_ATTR_PTHREAD blocking on flare::fiber_sleep_for()
// can't be interrupted.
    int fiber_worker::interrupt(fiber_id_t tid, schedule_group *c) {
        // Consume current_waiter in the fiber_entity, wake it up then set it back.
        fiber_mutex_waiter *w = nullptr;
        uint64_t sleep_id = 0;
        int rc = interrupt_and_consume_waiters(tid, &w, &sleep_id);
        if (rc) {
            return rc;
        }
        // a fiber cannot wait on a butex and be sleepy at the same time.
        FLARE_CHECK(!sleep_id || !w);
        if (w != nullptr) {
            erase_from_event_because_of_interruption(w);
            // If waitable_event_wait() already wakes up before we set current_waiter back,
            // the function will spin until current_waiter becomes non-nullptr.
            rc = set_event_waiter(tid, w);
            if (rc) {
                FLARE_LOG(FATAL) << "waitable_event_wait should spin until setting back waiter";
                return rc;
            }
        } else if (sleep_id != 0) {
            if (get_global_timer_thread()->unschedule(sleep_id) == 0) {
                flare::fiber_internal::fiber_worker *g = flare::fiber_internal::tls_task_group;
                if (g) {
                    g->ready_to_run(tid);
                } else {
                    if (!c) {
                        return EINVAL;
                    }
                    c->choose_one_group()->ready_to_run_remote(tid);
                }
            }
        }
        return 0;
    }

    void fiber_worker::yield(fiber_worker **pg) {
        fiber_worker *g = *pg;
        ReadyToRunArgs args = {g->current_fid(), false};
        g->set_remained(ready_to_run_in_worker, &args);
        sched(pg);
    }

    void print_task(std::ostream &os, fiber_id_t tid) {
        fiber_entity *const m = fiber_worker::address_meta(tid);
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
        fiber_attribute attr = FIBER_ATTR_NORMAL;
        bool has_tls = false;
        int64_t cpuwide_start_ns = 0;
        fiber_statistics stat = {0, 0};
        {
            FLARE_SCOPED_LOCK(m->version_lock);
            if (given_ver == *m->version_butex) {
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
               << "\nattr={stack_type=" << attr.stack_type
               << " flags=" << attr.flags
               << " keytable_pool=" << attr.keytable_pool
               << "}\nhas_tls=" << has_tls
               << "\nuptime_ns=" << flare::get_current_time_nanos() - cpuwide_start_ns
               << "\ncputime_ns=" << stat.cputime_ns
               << "\nnswitch=" << stat.nswitch;
        }
    }

}  // namespace flare::fiber_internal
