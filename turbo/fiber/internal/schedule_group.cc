// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Tue Jul 10 17:40:58 CST 2012

#include "turbo/base/scoped_lock.h"             // TURBO_SCOPED_LOCK
#include "turbo/base/errno.h"                    // turbo_error
#include "turbo/log/logging.h"
#include "turbo/hash/murmurhash3.h"
#include "turbo/fiber/internal/sys_futex.h"            // futex_wake_private
#include "turbo/fiber/internal/interrupt_pthread.h"
#include "turbo/fiber/internal/processor.h"            // cpu_relax
#include "turbo/fiber/internal/fiber_worker.h"           // fiber_worker
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/timer_thread.h"         // global_timer_thread
#include <gflags/gflags.h>
#include "turbo/fiber/internal/log.h"

DEFINE_int32(task_group_delete_delay, 1,
             "delay deletion of fiber_worker for so many seconds");
DEFINE_int32(task_group_runqueue_capacity, 4096,
             "capacity of runqueue in each fiber_worker");
DEFINE_int32(task_group_yield_before_idle, 0,
             "fiber_worker yields so many times before idle");

namespace turbo::fiber_internal {

    DECLARE_int32(fiber_concurrency);
    DECLARE_int32(fiber_min_concurrency);

    extern pthread_mutex_t g_task_control_mutex;
    extern TURBO_THREAD_LOCAL fiber_worker *tls_task_group;

    void (*g_worker_startfn)() = nullptr;

    // May be called in other modules to run startfn in non-worker pthreads.
    void run_worker_startfn() {
        if (g_worker_startfn) {
            g_worker_startfn();
        }
    }

    void *schedule_group::worker_thread(void *arg) {
        run_worker_startfn();

        schedule_group *c = static_cast<schedule_group *>(arg);
        fiber_worker *g = c->create_group();
        fiber_statistics stat;
        if (nullptr == g) {
            TURBO_LOG(ERROR) << "Fail to create fiber_worker in pthread=" << pthread_self();
            return nullptr;
        }
        BT_VLOG << "Created worker=" << pthread_self()
                << " fiber=" << g->main_tid();

        tls_task_group = g;
        c->_nworkers << 1;
        g->run_main_task();

        stat = g->main_stat();
        BT_VLOG << "Destroying worker=" << pthread_self() << " fiber="
                << g->main_tid() << " idle=" << stat.cputime_ns / 1000000.0
                << "ms uptime=" << g->current_uptime_ns() / 1000000.0 << "ms";
        tls_task_group = nullptr;
        g->destroy_self();
        c->_nworkers << -1;
        return nullptr;
    }

    fiber_worker *schedule_group::create_group() {
        fiber_worker *g = new(std::nothrow) fiber_worker(this);
        if (nullptr == g) {
            TURBO_LOG(FATAL) << "Fail to new fiber_worker";
            return nullptr;
        }
        if (g->init(FLAGS_task_group_runqueue_capacity) != 0) {
            TURBO_LOG(ERROR) << "Fail to init fiber_worker";
            delete g;
            return nullptr;
        }
        if (_add_group(g) != 0) {
            delete g;
            return nullptr;
        }
        return g;
    }

    static void print_rq_sizes_in_the_tc(std::ostream &os, void *arg) {
        schedule_group *tc = (schedule_group *) arg;
        tc->print_rq_sizes(os);
    }

    static double get_cumulated_worker_time_from_this(void *arg) {
        return static_cast<schedule_group *>(arg)->get_cumulated_worker_time();
    }

    static int64_t get_cumulated_switch_count_from_this(void *arg) {
        return static_cast<schedule_group *>(arg)->get_cumulated_switch_count();
    }

    static int64_t get_cumulated_signal_count_from_this(void *arg) {
        return static_cast<schedule_group *>(arg)->get_cumulated_signal_count();
    }

    schedule_group::schedule_group()
    // NOTE: all fileds must be initialized before the vars.
            : _ngroup(0), _groups((fiber_worker **) calloc(FIBER_MAX_CONCURRENCY, sizeof(fiber_worker *))),
              _stop(false), _concurrency(0), _nworkers("fiber_worker_count"), _pending_time(nullptr)
            // Delay exposure of following two vars because they rely on TC which
            // is not initialized yet.
            , _cumulated_worker_time(get_cumulated_worker_time_from_this, this),
              _worker_usage_second(&_cumulated_worker_time, 1),
              _cumulated_switch_count(get_cumulated_switch_count_from_this, this),
              _switch_per_second(&_cumulated_switch_count),
              _cumulated_signal_count(get_cumulated_signal_count_from_this, this),
              _signal_per_second(&_cumulated_signal_count), _status(print_rq_sizes_in_the_tc, this),
              _nfibers("fiber_count") {
        // calloc shall set memory to zero
        TURBO_CHECK(_groups) << "Fail to create array of groups";
    }

    int schedule_group::init(int concurrency) {
        if (_concurrency != 0) {
            TURBO_LOG(ERROR) << "Already initialized";
            return -1;
        }
        if (concurrency <= 0) {
            TURBO_LOG(ERROR) << "Invalid concurrency=" << concurrency;
            return -1;
        }
        _concurrency = concurrency;

        // Make sure TimerThread is ready.
        if (get_or_create_global_timer_thread() == nullptr) {
            TURBO_LOG(ERROR) << "Fail to get global_timer_thread";
            return -1;
        }

        _workers.resize(_concurrency);
        for (int i = 0; i < _concurrency; ++i) {
            const int rc = pthread_create(&_workers[i], nullptr, worker_thread, this);
            if (rc) {
                TURBO_LOG(ERROR) << "Fail to create _workers[" << i << "], " << turbo_error(rc);
                return -1;
            }
        }
        _worker_usage_second.expose("fiber_worker_usage", "");
        _switch_per_second.expose("fiber_switch_second", "");
        _signal_per_second.expose("fiber_signal_second", "");
        _status.expose("fiber_group_status", "");

        // Wait for at least one group is added so that choose_one_group()
        // never returns nullptr.
        // TODO: Handle the case that worker quits before add_group
        while (_ngroup == 0) {
            usleep(100);  // TODO: Elaborate
        }
        return 0;
    }

    int schedule_group::add_workers(int num) {
        if (num <= 0) {
            return 0;
        }
        try {
            _workers.resize(_concurrency + num);
        } catch (...) {
            return 0;
        }
        const int old_concurency = _concurrency.load(std::memory_order_relaxed);
        for (int i = 0; i < num; ++i) {
            // Worker will add itself to _idle_workers, so we have to add
            // _concurrency before create a worker.
            _concurrency.fetch_add(1);
            const int rc = pthread_create(
                    &_workers[i + old_concurency], nullptr, worker_thread, this);
            if (rc) {
                TURBO_LOG(WARNING) << "Fail to create _workers[" << i + old_concurency
                             << "], " << turbo_error(rc);
                _concurrency.fetch_sub(1, std::memory_order_release);
                break;
            }
        }
        // Cannot fail
        _workers.resize(_concurrency.load(std::memory_order_relaxed));
        return _concurrency.load(std::memory_order_relaxed) - old_concurency;
    }

    fiber_worker *schedule_group::choose_one_group() {
        const size_t ngroup = _ngroup.load(std::memory_order_acquire);
        if (ngroup != 0) {
            return _groups[turbo::base::fast_rand_less_than(ngroup)];
        }
        TURBO_CHECK(false) << "Impossible: ngroup is 0";
        return nullptr;
    }

    extern int stop_and_join_epoll_threads();

    void schedule_group::stop_and_join() {
        // Close epoll threads so that worker threads are not waiting on epoll(
        // which cannot be woken up by signal_task below)
        TURBO_CHECK_EQ(0, stop_and_join_epoll_threads());

        // Stop workers
        {
            TURBO_SCOPED_LOCK(_modify_group_mutex);
            _stop = true;
            _ngroup.exchange(0, std::memory_order_relaxed);
        }
        for (int i = 0; i < PARKING_LOT_NUM; ++i) {
            _pl[i].stop();
        }
        // Interrupt blocking operations.
        for (size_t i = 0; i < _workers.size(); ++i) {
            interrupt_pthread(_workers[i]);
        }
        // Join workers
        for (size_t i = 0; i < _workers.size(); ++i) {
            pthread_join(_workers[i], nullptr);
        }
    }

    schedule_group::~schedule_group() {
        // NOTE: g_task_control is not destructed now because the situation
        //       is extremely racy.
        delete _pending_time.exchange(nullptr, std::memory_order_relaxed);
        _worker_usage_second.hide();
        _switch_per_second.hide();
        _signal_per_second.hide();
        _status.hide();

        stop_and_join();

        free(_groups);
        _groups = nullptr;
    }

    int schedule_group::_add_group(fiber_worker *g) {
        if (__builtin_expect(nullptr == g, 0)) {
            return -1;
        }
        std::unique_lock<std::mutex> mu(_modify_group_mutex);
        if (_stop) {
            return -1;
        }
        size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        if (ngroup < (size_t) FIBER_MAX_CONCURRENCY) {
            _groups[ngroup] = g;
            _ngroup.store(ngroup + 1, std::memory_order_release);
        }
        mu.unlock();
        // See the comments in _destroy_group
        // TODO: Not needed anymore since non-worker pthread cannot have fiber_worker
        signal_task(65536);
        return 0;
    }

    void schedule_group::delete_task_group(void *arg) {
        delete (fiber_worker *) arg;
    }

    int schedule_group::_destroy_group(fiber_worker *g) {
        if (nullptr == g) {
            TURBO_LOG(ERROR) << "Param[g] is nullptr";
            return -1;
        }
        if (g->_control != this) {
            TURBO_LOG(ERROR) << "fiber_worker=" << g
                       << " does not belong to this schedule_group=" << this;
            return -1;
        }
        bool erased = false;
        {
            TURBO_SCOPED_LOCK(_modify_group_mutex);
            const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
            for (size_t i = 0; i < ngroup; ++i) {
                if (_groups[i] == g) {
                    // No need for atomic_thread_fence because lock did it.
                    _groups[i] = _groups[ngroup - 1];
                    // Change _ngroup and keep _groups unchanged at last so that:
                    //  - If steal_task sees the newest _ngroup, it would not touch
                    //    _groups[ngroup -1]
                    //  - If steal_task sees old _ngroup and is still iterating on
                    //    _groups, it would not miss _groups[ngroup - 1] which was
                    //    swapped to _groups[i]. Although adding new group would
                    //    overwrite it, since we do signal_task in _add_group(),
                    //    we think the pending tasks of _groups[ngroup - 1] would
                    //    not miss.
                    _ngroup.store(ngroup - 1, std::memory_order_release);
                    //_groups[ngroup - 1] = nullptr;
                    erased = true;
                    break;
                }
            }
        }

        // Can't delete g immediately because for performance consideration,
        // we don't lock _modify_group_mutex in steal_task which may
        // access the removed group concurrently. We use simple strategy here:
        // Schedule a function which deletes the fiber_worker after
        // FLAGS_task_group_delete_delay seconds
        if (erased) {
            get_global_timer_thread()->schedule(
                    delete_task_group, g,
                    turbo::time_point::future_unix_seconds(FLAGS_task_group_delete_delay).to_timespec());
        }
        return 0;
    }

    bool schedule_group::steal_task(fiber_id_t *tid, size_t *seed, size_t offset) {
        // 1: Acquiring fence is paired with releasing fence in _add_group to
        // avoid accessing uninitialized slot of _groups.
        const size_t ngroup = _ngroup.load(std::memory_order_acquire/*1*/);
        if (0 == ngroup) {
            return false;
        }

        // NOTE: Don't return inside `for' iteration since we need to update |seed|
        bool stolen = false;
        size_t s = *seed;
        for (size_t i = 0; i < ngroup; ++i, s += offset) {
            fiber_worker *g = _groups[s % ngroup];
            // g is possibly nullptr because of concurrent _destroy_group
            if (g) {
                if (g->_rq.steal(tid)) {
                    stolen = true;
                    break;
                }
                if (g->_remote_rq.pop(tid)) {
                    stolen = true;
                    break;
                }
            }
        }
        *seed = s;
        return stolen;
    }

    void schedule_group::signal_task(int num_task) {
        if (num_task <= 0) {
            return;
        }
        // TODO(gejun): Current algorithm does not guarantee enough threads will
        // be created to match caller's requests. But in another side, there's also
        // many useless signalings according to current impl. Capping the concurrency
        // is a good balance between performance and timeliness of scheduling.
        if (num_task > 2) {
            num_task = 2;
        }
        int start_index = turbo::hash::fmix64(pthread_numeric_id()) % PARKING_LOT_NUM;
        num_task -= _pl[start_index].signal(1);
        if (num_task > 0) {
            for (int i = 1; i < PARKING_LOT_NUM && num_task > 0; ++i) {
                if (++start_index >= PARKING_LOT_NUM) {
                    start_index = 0;
                }
                num_task -= _pl[start_index].signal(1);
            }
        }
        if (num_task > 0 &&
            FLAGS_fiber_min_concurrency > 0 &&    // test min_concurrency for performance
            _concurrency.load(std::memory_order_relaxed) < FLAGS_fiber_concurrency) {
            // TODO: Reduce this lock
            TURBO_SCOPED_LOCK(g_task_control_mutex);
            if (_concurrency.load(std::memory_order_acquire) < FLAGS_fiber_concurrency) {
                add_workers(1);
            }
        }
    }

    void schedule_group::print_rq_sizes(std::ostream &os) {
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        DEFINE_SMALL_ARRAY(int, nums, ngroup, 128);
        {
            TURBO_SCOPED_LOCK(_modify_group_mutex);
            // ngroup > _ngroup: nums[_ngroup ... ngroup-1] = 0
            // ngroup < _ngroup: just ignore _groups[_ngroup ... ngroup-1]
            for (size_t i = 0; i < ngroup; ++i) {
                nums[i] = (_groups[i] ? _groups[i]->_rq.volatile_size() : 0);
            }
        }
        for (size_t i = 0; i < ngroup; ++i) {
            os << nums[i] << ' ';
        }
    }

    double schedule_group::get_cumulated_worker_time() {
        int64_t cputime_ns = 0;
        TURBO_SCOPED_LOCK(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            if (_groups[i]) {
                cputime_ns += _groups[i]->_cumulated_cputime_ns;
            }
        }
        return cputime_ns / 1000000000.0;
    }

    int64_t schedule_group::get_cumulated_switch_count() {
        int64_t c = 0;
        TURBO_SCOPED_LOCK(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            if (_groups[i]) {
                c += _groups[i]->_nswitch;
            }
        }
        return c;
    }

    int64_t schedule_group::get_cumulated_signal_count() {
        int64_t c = 0;
        TURBO_SCOPED_LOCK(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            fiber_worker *g = _groups[i];
            if (g) {
                c += g->_nsignaled + g->_remote_nsignaled;
            }
        }
        return c;
    }

    turbo::LatencyRecorder *schedule_group::create_exposed_pending_time() {
        bool is_creator = false;
        _pending_time_mutex.lock();
        turbo::LatencyRecorder *pt = _pending_time.load(std::memory_order_consume);
        if (!pt) {
            pt = new turbo::LatencyRecorder;
            _pending_time.store(pt, std::memory_order_release);
            is_creator = true;
        }
        _pending_time_mutex.unlock();
        if (is_creator) {
            pt->expose("fiber_creation");
        }
        return pt;
    }

}  // namespace turbo::fiber_internal
