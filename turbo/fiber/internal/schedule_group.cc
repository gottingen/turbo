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


#include "turbo/log/logging.h"
#include "turbo/hash/hash.h"
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/system/threading.h"
#include "turbo/base/processor.h"            // cpu_relax
#include "turbo/fiber/internal/fiber_worker.h"           // FiberWorker
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/timer.h"         // global_timer_thread
#include "turbo/log/logging.h"
#include "turbo/status/error.h"
#include "turbo/random/random.h"
#include "turbo/system/sysinfo.h"
#include <mutex>


namespace turbo::fiber_internal {

    extern std::mutex g_task_control_mutex;
    extern TURBO_THREAD_LOCAL FiberWorker *tls_task_group;

    void (*g_worker_startfn)() = nullptr;

    // May be called in other modules to run startfn in non-worker pthreads.
    void run_worker_startfn() {
        if (g_worker_startfn) {
            g_worker_startfn();
        }
    }

    void *ScheduleGroup::worker_thread(void *arg) {
        run_worker_startfn();

        auto *c = static_cast<ScheduleGroup *>(arg);
        auto *g = c->create_group();
        FiberStatistics stat;
        if (nullptr == g) {
            TLOG_ERROR("Fail to create FiberWorker in pthread={}", pthread_self());
            return nullptr;
        }
        TDLOG_INFO("Created worker={} fiber={}", pthread_self(), g->main_tid());


        tls_task_group = g;
        g->run_main_task();

        stat = g->main_stat();
        TDLOG_INFO("Destroying worker={} fiber={} idle={}ms uptime={}ms", pthread_self(), g->main_tid(),
                   stat.cputime_ns / 1000000.0, g->current_uptime_ns() / 1000000.0);
        tls_task_group = nullptr;
        g->destroy_self();
        return nullptr;
    }

    FiberWorker *ScheduleGroup::create_group() {
        auto g = new(std::nothrow) FiberWorker(this);
        if (nullptr == g) {
            TLOG_CRITICAL("Fail to new FiberWorker");
            return nullptr;
        }
        if (g->init(turbo::get_flag(FLAGS_task_group_runqueue_capacity)) != 0) {
            TLOG_CRITICAL("Fail to init FiberWorker");
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
        ScheduleGroup *tc = (ScheduleGroup *) arg;
        tc->print_rq_sizes(os);
    }

    static double get_cumulated_worker_time_from_this(void *arg) {
        return static_cast<ScheduleGroup *>(arg)->get_cumulated_worker_time();
    }

    static int64_t get_cumulated_switch_count_from_this(void *arg) {
        return static_cast<ScheduleGroup *>(arg)->get_cumulated_switch_count();
    }

    static int64_t get_cumulated_signal_count_from_this(void *arg) {
        return static_cast<ScheduleGroup *>(arg)->get_cumulated_signal_count();
    }

    ScheduleGroup::ScheduleGroup()
    // NOTE: all fileds must be initialized before the vars.
            : _ngroup(0),
              _groups((FiberWorker **) calloc(turbo::fiber_config::FIBER_MAX_CONCURRENCY, sizeof(FiberWorker *))),
              _stop(false), _concurrency(0) {
        // calloc shall set memory to zero
        TLOG_CHECK(_groups, "Fail to create array of groups");
    }

    int ScheduleGroup::init(int concurrency) {
        if (_concurrency != 0) {
            TLOG_ERROR("Already initialized");
            return -1;
        }
        if (concurrency <= 0) {
            TLOG_ERROR("Invalid concurrency={}", concurrency);
            return -1;
        }
        _concurrency = concurrency;

        // Make sure TimerThread is ready.
        if (!init_fiber_timer_thread().ok()) {
            TLOG_ERROR("Fail to get global_timer_thread");
            return -1;
        }

        _workers.resize(_concurrency);
        for (int i = 0; i < _concurrency; ++i) {
            const int rc = pthread_create(&_workers[i], nullptr, worker_thread, this);
            if (rc) {
                TLOG_ERROR("Fail to create _workers[{}], {}", i, terror(rc));
                return -1;
            }
        }

        // Wait for at least one group is added so that choose_one_group()
        // never returns nullptr.
        // TODO: Handle the case that worker quits before add_group
        while (_ngroup == 0) {
            usleep(100);  // TODO: Elaborate
        }
        return 0;
    }

    int ScheduleGroup::add_workers(int num) {
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
                TLOG_WARN("Fail to create _workers[{}], {}", i + old_concurency, terror(errno));
                _concurrency.fetch_sub(1, std::memory_order_release);
                break;
            }
        }
        // Cannot fail
        _workers.resize(_concurrency.load(std::memory_order_relaxed));
        return _concurrency.load(std::memory_order_relaxed) - old_concurency;
    }

    FiberWorker *ScheduleGroup::choose_one_group() {
        const size_t ngroup = _ngroup.load(std::memory_order_acquire);
        if (ngroup != 0) {
            return _groups[turbo::fast_uniform<size_t>(0ul, ngroup)];
        }
        TLOG_CHECK(false, "Impossible: ngroup is 0");
        return nullptr;
    }

    extern int stop_and_join_epoll_threads();

    void ScheduleGroup::stop_and_join() {
        // Close epoll threads so that worker threads are not waiting on epoll(
        // which cannot be woken up by signal_task below)
        TLOG_CHECK_EQ(0, stop_and_join_epoll_threads());

        // Stop workers
        {
            std::unique_lock l(_modify_group_mutex);
            _stop = true;
            _ngroup.exchange(0, std::memory_order_relaxed);
        }
        for (int i = 0; i < PARKING_LOT_NUM; ++i) {
            _pl[i].stop();
        }
        // Interrupt blocking operations.
        for (size_t i = 0; i < _workers.size(); ++i) {
            turbo::PlatformThread::kill_thread(_workers[i]);
        }
        // Join workers
        for (size_t i = 0; i < _workers.size(); ++i) {
            pthread_join(_workers[i], nullptr);
        }
    }

    ScheduleGroup::~ScheduleGroup() {
        stop_and_join();

        free(_groups);
        _groups = nullptr;
    }

    int ScheduleGroup::_add_group(FiberWorker *g) {
        if (TURBO_UNLIKELY(nullptr == g)) {
            return -1;
        }
        std::unique_lock<std::mutex> mu(_modify_group_mutex);
        if (_stop) {
            return -1;
        }
        size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        if (ngroup < (size_t) turbo::fiber_config::FIBER_MAX_CONCURRENCY) {
            _groups[ngroup] = g;
            _ngroup.store(ngroup + 1, std::memory_order_release);
        }
        mu.unlock();
        // See the comments in _destroy_group
        // TODO: Not needed anymore since non-worker pthread cannot have FiberWorker
        signal_task(65536);
        return 0;
    }

    void ScheduleGroup::delete_task_group(void *arg) {
        delete (FiberWorker *) arg;
    }

    int ScheduleGroup::_destroy_group(FiberWorker *g) {
        if (nullptr == g) {
            TLOG_ERROR("Param[g] is nullptr");
            return -1;
        }
        if (g->_control != this) {
            TLOG_ERROR("FiberWorker={} does not belong to this ScheduleGroup={}", turbo::ptr(g), turbo::ptr(this));
            return -1;
        }
        bool erased = false;
        {
            std::unique_lock l(_modify_group_mutex);
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
        // Schedule a function which deletes the FiberWorker after
        if (erased) {
            auto rs = get_fiber_timer_thread()->schedule(delete_task_group, g,
                                                         turbo::seconds_from_now(
                                                                 turbo::get_flag(FLAGS_task_group_delete_delay)));
            TURBO_UNUSED(rs);
        }
        return 0;
    }

    bool ScheduleGroup::steal_task(fiber_id_t *tid, size_t *seed, size_t offset) {
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
            FiberWorker *g = _groups[s % ngroup];
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

    void ScheduleGroup::signal_task(int num_task) {
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
        int start_index = turbo::hash_mixer8(thread_numeric_id()) % PARKING_LOT_NUM;
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
            turbo::get_flag(FLAGS_fiber_min_concurrency) > 0 &&    // test min_concurrency for performance
            _concurrency.load(std::memory_order_relaxed) < turbo::get_flag(FLAGS_fiber_concurrency)) {
            // TODO: Reduce this lock
            std::unique_lock l(g_task_control_mutex);
            if (_concurrency.load(std::memory_order_acquire) < turbo::get_flag(FLAGS_fiber_concurrency)) {
                add_workers(1);
            }
        }
    }

    void ScheduleGroup::print_rq_sizes(std::ostream &os) {
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        std::vector<size_t> nums;
        {
            std::unique_lock l(_modify_group_mutex);
            // ngroup > _ngroup: nums[_ngroup ... ngroup-1] = 0
            // ngroup < _ngroup: just ignore _groups[_ngroup ... ngroup-1]
            for (size_t i = 0; i < ngroup; ++i) {
                nums.emplace_back(_groups[i] ? _groups[i]->_rq.volatile_size() : 0);
            }
        }
        for (size_t i = 0; i < ngroup; ++i) {
            os << nums[i] << ' ';
        }
    }

    double ScheduleGroup::get_cumulated_worker_time() {
        int64_t cputime_ns = 0;
        std::unique_lock l(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            if (_groups[i]) {
                cputime_ns += _groups[i]->_cumulated_cputime_ns;
            }
        }
        return cputime_ns / 1000000000.0;
    }

    int64_t ScheduleGroup::get_cumulated_switch_count() {
        int64_t c = 0;
        std::unique_lock l(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            if (_groups[i]) {
                c += _groups[i]->_nswitch;
            }
        }
        return c;
    }

    int64_t ScheduleGroup::get_cumulated_signal_count() {
        int64_t c = 0;
        std::unique_lock l(_modify_group_mutex);
        const size_t ngroup = _ngroup.load(std::memory_order_relaxed);
        for (size_t i = 0; i < ngroup; ++i) {
            FiberWorker *g = _groups[i];
            if (g) {
                c += g->_nsignaled + g->_remote_nsignaled;
            }
        }
        return c;
    }

}  // namespace turbo::fiber_internal
