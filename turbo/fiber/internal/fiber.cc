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

    ScheduleGroup *get_task_control() {
        return g_task_control;
    }

    ScheduleGroup *get_or_new_task_control() {
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
        int concurrency = turbo::get_flag(FLAGS_fiber_min_concurrency) > 0 ?
                          turbo::get_flag(FLAGS_fiber_min_concurrency) :
                          turbo::get_flag(FLAGS_fiber_concurrency);
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
            return make_status(kENOMEM);
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
        return c->choose_one_group()->start_background<true>(tid, attr, std::move(fn), arg);
    }

    struct TidTraits {
        static const size_t BLOCK_SIZE = 63;
        static const size_t MAX_ENTRIES = 65536;
        static const fiber_id_t SESSION_INIT;

        static bool exists(fiber_id_t id) { return turbo::fiber_internal::FiberWorker::exists(id); }
    };

    const fiber_id_t TidTraits::SESSION_INIT = INVALID_FIBER_ID;

    typedef ListOfABAFreeId<fiber_id_t, TidTraits> TidList;

    struct TidStopper {
        void operator()(fiber_id_t id) const { TURBO_UNUSED(fiber_stop_impl(id)); }
    };

    struct TidJoiner {
        void operator()(fiber_id_t &id) const {
            TURBO_UNUSED(fiber_join_impl(id, nullptr));
            id = INVALID_FIBER_ID;
        }
    };


    turbo::Status fiber_start_impl(fiber_id_t *TURBO_RESTRICT tid,
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

    turbo::Status fiber_start_background_impl(fiber_id_t *TURBO_RESTRICT tid,
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

    void fiber_flush_impl() {
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

    turbo::Status fiber_interrupt_impl(fiber_id_t tid) {
        return turbo::fiber_internal::FiberWorker::interrupt(tid, turbo::fiber_internal::get_task_control());
    }

    turbo::Status fiber_stop_impl(fiber_id_t tid) {
        turbo::fiber_internal::FiberWorker::set_stopped(tid);
        return fiber_interrupt_impl(tid);
    }

    bool fiber_stopped_impl(fiber_id_t tid) {
        return turbo::fiber_internal::FiberWorker::is_stopped(tid);
    }

    fiber_id_t fiber_self_impl(void) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        // note: return 0 for main tasks now, which include main thread and
        // all work threads. So that we can identify main tasks from logs
        // more easily. This is probably questionable in future.
        if (g != nullptr && !g->is_current_main_task()/*note*/) {
            return g->current_fid();
        }
        return INVALID_FIBER_ID;
    }

    int fiber_equal_impl(fiber_id_t t1, fiber_id_t t2) {
        return t1 == t2;
    }

    void fiber_exit_impl(void *retval) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g != nullptr && !g->is_current_main_task()) {
            throw turbo::fiber_internal::ExitException(retval);
        } else {
            pthread_exit(retval);
        }
    }

    turbo::Status fiber_join_impl(fiber_id_t tid, void **thread_return) {
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

    int fiber_get_concurrency_impl(void) {
        return turbo::get_flag(FLAGS_fiber_concurrency);
    }

    turbo::Status fiber_set_concurrency_impl(int num) {
        if (num < turbo::fiber_config::FIBER_MIN_CONCURRENCY || num > turbo::fiber_config::FIBER_MAX_CONCURRENCY) {
            TLOG_ERROR("Invalid concurrency={}", num);
            return turbo::make_status(EINVAL);
        }
        if (turbo::get_flag(FLAGS_fiber_min_concurrency) > 0) {
            if (num < turbo::get_flag(FLAGS_fiber_min_concurrency)) {
                return turbo::make_status(EINVAL);
            }
            if (turbo::fiber_internal::never_set_fiber_concurrency) {
                turbo::fiber_internal::never_set_fiber_concurrency = false;
            }
            turbo::set_flag(&FLAGS_fiber_concurrency,  num);
            return turbo::ok_status();
        }
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_task_control();
        if (c != nullptr) {
            if (num < c->concurrency()) {
                return turbo::make_status(EPERM);
            } else if (num == c->concurrency()) {
                return turbo::ok_status();
            }
        }
        std::unique_lock l(turbo::fiber_internal::g_task_control_mutex);
        c = turbo::fiber_internal::get_task_control();
        if (c == nullptr) {
            if (turbo::fiber_internal::never_set_fiber_concurrency) {
                turbo::fiber_internal::never_set_fiber_concurrency = false;
                turbo::set_flag(&FLAGS_fiber_concurrency, num);
            } else if (num > turbo::get_flag(FLAGS_fiber_concurrency)) {
                turbo::set_flag(&FLAGS_fiber_concurrency, num);
            }
            return turbo::ok_status();
        }
        if (turbo::get_flag(FLAGS_fiber_concurrency) != c->concurrency()) {
            TLOG_ERROR("failed: fiber_concurrency={} != tc_concurrency={}",
                       turbo::get_flag(FLAGS_fiber_concurrency), c->concurrency());
            turbo::set_flag(&FLAGS_fiber_concurrency, c->concurrency());
        }
        if (num > turbo::get_flag(FLAGS_fiber_concurrency)) {
            // Create more workers if needed.
            auto n= c->add_workers(num - turbo::get_flag(FLAGS_fiber_concurrency)) + turbo::get_flag(FLAGS_fiber_concurrency);
            turbo::set_flag(&FLAGS_fiber_concurrency, n);
            return turbo::ok_status();
        }
        return (num == turbo::get_flag(FLAGS_fiber_concurrency) ? turbo::ok_status()
                                                                            : turbo::make_status(EPERM));
    }

    int fiber_about_to_quit_impl() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (g != nullptr) {
            turbo::fiber_internal::FiberEntity *current_fiber = g->current_fiber();
            if (!is_never_quit(current_fiber->attr)) {
                current_fiber->about_to_quit = true;
            }
            return 0;
        }
        return EPERM;
    }

    int fiber_set_worker_startfn(void (*start_fn)()) {
        if (start_fn == nullptr) {
            return EINVAL;
        }
        turbo::fiber_internal::g_worker_startfn = start_fn;
        return 0;
    }

    void fiber_stop_world_impl() {
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
