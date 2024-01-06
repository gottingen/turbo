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

#include <utility>
#include "turbo/log/logging.h"
#include "turbo/fiber/internal/fiber_worker.h"                // FiberWorker
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/times/timer_thread.h"
#include "turbo/fiber/internal/list_of_abafree_id.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/log/logging.h"

namespace turbo::fiber_internal {


    static bool never_set_fiber_concurrency = true;

    static_assert(sizeof(ScheduleGroup *) == sizeof(std::atomic<ScheduleGroup *>), "atomic_size_match");

    std::mutex g_task_control_mutex;
    // Referenced in rpc, needs to be extern.
    // Notice that we can't declare the variable as atomic<ScheduleGroup*> which
    // are not constructed before main().
    ScheduleGroup *g_task_control = nullptr;

    extern TURBO_THREAD_LOCAL FiberWorker *tls_task_group;

    extern void (*g_worker_startfn)();

    inline ScheduleGroup *get_task_control() {
        return g_task_control;
    }

    inline ScheduleGroup *get_or_new_task_control() {
        std::atomic<ScheduleGroup *> *p = (std::atomic<ScheduleGroup *> *) &g_task_control;
        ScheduleGroup *c = p->load(std::memory_order_consume);
        if (c != nullptr) {
            return c;
        }
        std::unique_lock l(g_task_control_mutex);
        c = p->load(std::memory_order_consume);
        if (c != nullptr) {
            return c;
        }
        c = new(std::nothrow) ScheduleGroup;
        if (nullptr == c) {
            return nullptr;
        }
        int concurrency = FiberConfig::get_instance().fiber_min_concurrency > 0 ?
                          FiberConfig::get_instance().fiber_min_concurrency :
                          FiberConfig::get_instance().fiber_concurrency;
        if (c->init(concurrency) != 0) {
            TLOG_ERROR("Fail to init g_task_control");
            delete c;
            return nullptr;
        }
        p->store(c, std::memory_order_release);
        return c;
    }

    __thread FiberWorker *tls_task_group_nosignal = nullptr;

    TURBO_FORCE_INLINE turbo::Status
    start_from_non_worker(fiber_id_t *TURBO_RESTRICT tid,
                          const FiberAttribute *TURBO_RESTRICT attr,
                          std::function<void *(void *)> &&fn,
                          void *TURBO_RESTRICT arg) {
        ScheduleGroup *c = get_or_new_task_control();
        if (nullptr == c) {
            return turbo::resource_exhausted_error("");
        }
        if (attr != nullptr && is_nosignal(*attr)) {
            // Remember the FiberWorker to insert NOSIGNAL tasks for 2 reasons:
            // 1. NOSIGNAL is often for creating many fibers in batch,
            //    inserting into the same FiberWorker maximizes the batch.
            // 2. fiber_flush() needs to know which FiberWorker to flush.
            FiberWorker *g = tls_task_group_nosignal;
            if (nullptr == g) {
                g = c->choose_one_group();
                tls_task_group_nosignal = g;
            }
            return g->start_background<true>(tid, attr, std::move(fn), arg);
        }
        return c->choose_one_group()->start_background<true>(
                tid, attr, std::move(fn), arg);
    }

    struct TidTraits {
        static const size_t BLOCK_SIZE = 63;
        static const size_t MAX_ENTRIES = 65536;
        static const fiber_id_t TOKEN_INIT;

        static bool exists(fiber_id_t id) { return turbo::fiber_internal::FiberWorker::exists(id); }
    };

    const fiber_id_t TidTraits::TOKEN_INIT = INVALID_FIBER_ID;

    typedef ListOfABAFreeId<fiber_id_t, TidTraits> TidList;

    struct TidStopper {
        void operator()(fiber_id_t id) const { TURBO_UNUSED(fiber_stop(id)); }
    };

    struct TidJoiner {
        void operator()(fiber_id_t &id) const {
            TURBO_UNUSED(fiber_join(id, nullptr));
            id = INVALID_FIBER_ID;
        }
    };


    turbo::Status fiber_start_urgent(fiber_id_t *TURBO_RESTRICT tid,
                                     const FiberAttribute *TURBO_RESTRICT attr,
                                     std::function<void *(void *)> &&fn,
                                     void *TURBO_RESTRICT arg) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g) {
            // start from worker
            return turbo::fiber_internal::FiberWorker::start_foreground(&g, tid, attr, std::move(fn), arg);
        }
        return turbo::fiber_internal::start_from_non_worker(tid, attr, std::move(fn), arg);
    }

    turbo::Status fiber_start_background(fiber_id_t *TURBO_RESTRICT tid,
                                         const FiberAttribute *TURBO_RESTRICT attr,
                                         std::function<void *(void *)> &&fn,
                                         void *TURBO_RESTRICT arg) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g) {
            // start from worker
            return g->start_background<false>(tid, attr, std::move(fn), arg);
        }
        return turbo::fiber_internal::start_from_non_worker(tid, attr, std::move(fn), arg);
    }

    void fiber_flush() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g) {
            return g->flush_nosignal_tasks();
        }
        g = turbo::fiber_internal::tls_task_group_nosignal;
        if (g) {
            // NOSIGNAL tasks were created in this non-worker.
            turbo::fiber_internal::tls_task_group_nosignal = nullptr;
            return g->flush_nosignal_tasks_remote();
        }
    }

    turbo::Status fiber_interrupt(fiber_id_t tid) {
        return turbo::fiber_internal::FiberWorker::interrupt(tid, turbo::fiber_internal::get_task_control());
    }

    turbo::Status fiber_stop(fiber_id_t tid) {
        turbo::fiber_internal::FiberWorker::set_stopped(tid);
        return fiber_interrupt(tid);
    }

    bool fiber_stopped(fiber_id_t tid) {
        return turbo::fiber_internal::FiberWorker::is_stopped(tid);
    }

    fiber_id_t fiber_self(void) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        // note: return 0 for main tasks now, which include main thread and
        // all work threads. So that we can identify main tasks from logs
        // more easily. This is probably questionable in future.
        if (g != nullptr && !g->is_current_main_task()/*note*/) {
            return g->current_fid();
        }
        return INVALID_FIBER_ID;
    }

    int fiber_equal(fiber_id_t t1, fiber_id_t t2) {
        return t1 == t2;
    }

    void fiber_exit(void *retval) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g != nullptr && !g->is_current_main_task()) {
            throw turbo::fiber_internal::ExitException(retval);
        } else {
            pthread_exit(retval);
        }
    }

    turbo::Status fiber_join(fiber_id_t tid, void **thread_return) {
        return turbo::fiber_internal::FiberWorker::join(tid, thread_return);
    }

    int fiber_attr_init(FiberAttribute *a) {
        *a = FIBER_ATTR_NORMAL;
        return 0;
    }

    int fiber_attr_destroy(FiberAttribute *) {
        return 0;
    }

    int fiber_getattr(fiber_id_t tid, FiberAttribute *attr) {
        return turbo::fiber_internal::FiberWorker::get_attr(tid, attr);
    }

    int fiber_get_concurrency(void) {
        return turbo::FiberConfig::get_instance().fiber_concurrency;
    }

    turbo::Status fiber_set_concurrency(int num) {
        if (num < turbo::FiberConfig::FIBER_MIN_CONCURRENCY || num > turbo::FiberConfig::FIBER_MAX_CONCURRENCY) {
            TLOG_ERROR("Invalid concurrency={}", num);
            return turbo::invalid_argument_error("");
        }
        if (turbo::FiberConfig::get_instance().fiber_min_concurrency > 0) {
            if (num < turbo::FiberConfig::get_instance().fiber_min_concurrency) {
                return turbo::invalid_argument_error("");
            }
            if (turbo::fiber_internal::never_set_fiber_concurrency) {
                turbo::fiber_internal::never_set_fiber_concurrency = false;
            }
            turbo::FiberConfig::get_instance().fiber_concurrency = num;
            return turbo::ok_status();
        }
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_task_control();
        if (c != nullptr) {
            if (num < c->concurrency()) {
                return turbo::resource_exhausted_error("");
            } else if (num == c->concurrency()) {
                return turbo::ok_status();
            }
        }
        std::unique_lock l(turbo::fiber_internal::g_task_control_mutex);
        c = turbo::fiber_internal::get_task_control();
        if (c == nullptr) {
            if (turbo::fiber_internal::never_set_fiber_concurrency) {
                turbo::fiber_internal::never_set_fiber_concurrency = false;
                turbo::FiberConfig::get_instance().fiber_concurrency = num;
            } else if (num > turbo::FiberConfig::get_instance().fiber_concurrency) {
                turbo::FiberConfig::get_instance().fiber_concurrency = num;
            }
            return turbo::ok_status();
        }
        if (turbo::FiberConfig::get_instance().fiber_concurrency != c->concurrency()) {
            TLOG_ERROR("failed: fiber_concurrency={} != tc_concurrency={}",
                       turbo::FiberConfig::get_instance().fiber_concurrency, c->concurrency());
            turbo::FiberConfig::get_instance().fiber_concurrency = c->concurrency();
        }
        if (num > turbo::FiberConfig::get_instance().fiber_concurrency) {
            // Create more workers if needed.
            turbo::FiberConfig::get_instance().fiber_concurrency +=
                    c->add_workers(num - turbo::FiberConfig::get_instance().fiber_concurrency);
            return turbo::ok_status();
        }
        return (num == turbo::FiberConfig::get_instance().fiber_concurrency ? turbo::ok_status()
                                                                            : turbo::resource_exhausted_error(""));
    }

    int fiber_about_to_quit() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g != nullptr) {
            turbo::fiber_internal::FiberEntity *current_task = g->current_task();
            if (!is_never_quit(current_task->attr)) {
                current_task->about_to_quit = true;
            }
            return 0;
        }
        return EPERM;
    }

    int fiber_timer_add(fiber_timer_id *id, timespec abstime,
                        void (*on_timer)(void *), void *arg) {
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_or_new_task_control();
        if (c == nullptr) {
            return ENOMEM;
        }
        turbo::TimerThread *tt = turbo::get_or_create_global_timer_thread();
        if (tt == nullptr) {
            return ENOMEM;
        }
        fiber_timer_id tmp = tt->schedule(on_timer, arg, turbo::time_from_timespec(abstime));
        if (tmp != 0) {
            *id = tmp;
            return 0;
        }
        return 20;
    }

    int fiber_timer_del(fiber_timer_id id) {
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_task_control();
        if (c != nullptr) {
            turbo::TimerThread *tt = turbo::get_global_timer_thread();
            if (tt == nullptr) {
                return EINVAL;
            }
            const auto state = tt->unschedule(id);
            if (state.ok() || turbo::is_not_found(state)) {
                return 0;
            }
        }
        return EINVAL;
    }

    int fiber_set_worker_startfn(void (*start_fn)()) {
        if (start_fn == nullptr) {
            return EINVAL;
        }
        turbo::fiber_internal::g_worker_startfn = start_fn;
        return 0;
    }

    void fiber_stop_world() {
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_task_control();
        if (c != nullptr) {
            c->stop_and_join();
        }
    }

    int fiber_list_init(fiber_list_t *list,
                        unsigned /*size*/,
                        unsigned /*conflict_size*/) {
        list->impl = new(std::nothrow) turbo::fiber_internal::TidList;
        if (nullptr == list->impl) {
            return ENOMEM;
        }
        // Set unused fields to zero as well.
        list->head = 0;
        list->size = 0;
        list->conflict_head = 0;
        list->conflict_size = 0;
        return 0;
    }

    void fiber_list_destroy(fiber_list_t *list) {
        delete static_cast<turbo::fiber_internal::TidList *>(list->impl);
        list->impl = nullptr;
    }

    int fiber_list_add(fiber_list_t *list, fiber_id_t id) {
        if (list->impl == nullptr) {
            return EINVAL;
        }
        return static_cast<turbo::fiber_internal::TidList *>(list->impl)->add(id);
    }

    int fiber_list_stop(fiber_list_t *list) {
        if (list->impl == nullptr) {
            return EINVAL;
        }
        static_cast<turbo::fiber_internal::TidList *>(list->impl)->apply(turbo::fiber_internal::TidStopper());
        return 0;
    }

    int fiber_list_join(fiber_list_t *list) {
        if (list->impl == nullptr) {
            return EINVAL;
        }
        static_cast<turbo::fiber_internal::TidList *>(list->impl)->apply(turbo::fiber_internal::TidJoiner());
        return 0;
    }

}  // namespace turbo::fiber_internal
