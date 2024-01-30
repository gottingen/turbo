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
// Created by jeff on 24-1-4.
//

#include "turbo/fiber/fiber.h"
#include "turbo/platform/port.h"
#include "turbo/fiber/internal/fiber_worker.h"

namespace turbo {
    namespace fiber_internal {
        extern __thread turbo::fiber_internal::fiber_local_storage tls_bls;
        extern __thread FiberWorker *tls_task_group;
    }  // namespace fiber_internal

    [[maybe_unused]] turbo::Status fiber_start(fiber_id_t *TURBO_RESTRICT tid,
                                                      const FiberAttribute *TURBO_RESTRICT attr,
                                                      fiber_fn_t &&fn,
                                                      void *TURBO_RESTRICT args) {
        return turbo::fiber_internal::fiber_start_impl(tid, attr, std::move(fn), args);
    }

    [[maybe_unused]] turbo::Status fiber_start_background(fiber_id_t *TURBO_RESTRICT tid,
                                                          const FiberAttribute *TURBO_RESTRICT attr,
                                                          fiber_fn_t &&fn,
                                                          void *TURBO_RESTRICT args) {
        return turbo::fiber_internal::fiber_start_background_impl(tid, attr, std::move(fn), args);
    }

    turbo::Status fiber_interrupt(fiber_id_t tid) {
        return turbo::fiber_internal::fiber_interrupt_impl(tid);
    }

    turbo::Status fiber_stop(fiber_id_t tid) {
        return turbo::fiber_internal::fiber_stop_impl(tid);
    }

    bool fiber_stopped(fiber_id_t tid) {
        return turbo::fiber_internal::fiber_stopped_impl(tid);
    }


    void fiber_exit(void *retval) {
        turbo::fiber_internal::fiber_exit_impl(retval);
    }

    turbo::Status fiber_join(fiber_id_t bt, void **fiber_return) {
        return turbo::fiber_internal::fiber_join_impl(bt, fiber_return);
    }

    turbo::Status Fiber::start(fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_impl(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
        if(rs.ok()) {
            _status = FiberStatus::eRunning;
        }
        return rs;
    }

    turbo::Status Fiber::start(const FiberAttribute attr, fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_impl(&_fid, &attr, std::move(fn), args);
        if(rs.ok()) {
            _status = FiberStatus::eRunning;
        }
        return rs;
    }

    turbo::Status Fiber::start_lazy(fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_background_impl(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
        if(rs.ok()) {
            _status = FiberStatus::eRunning;
        }
        return rs;
    }

    turbo::Status Fiber::start_lazy(const FiberAttribute attr, fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_background_impl(&_fid, &attr, std::move(fn), args);
        if(rs.ok()) {
            _status = FiberStatus::eRunning;
        }
        return rs;
    }


    turbo::Status Fiber::start(LaunchPolicy policy, fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        if (policy == LaunchPolicy::eImmediately) {
            auto rs = turbo::fiber_internal::fiber_start_impl(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        } else {
            auto rs = turbo::fiber_internal::fiber_start_background_impl(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        }
    }

    turbo::Status Fiber::start(LaunchPolicy policy, const FiberAttribute attr, fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        if (policy == LaunchPolicy::eImmediately) {
            auto rs = turbo::fiber_internal::fiber_start_impl(&_fid, &attr, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        } else {
            auto rs = turbo::fiber_internal::fiber_start_background_impl(&_fid, &attr, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        }
    }

    turbo::Status Fiber::join(void **retval) {
        if (joinable()) {
            auto rs = turbo::fiber_internal::fiber_join_impl(_fid, retval);
            _status = FiberStatus::eJoined;
            return rs;
        }
        if(_status == FiberStatus::eJoined) {
            return turbo::ok_status();
        }
        return turbo::make_status(kEINVAL, "Fiber is not joinable.");
    }

    void Fiber::detach() {
        _status = FiberStatus::eDetached;
    }

    turbo::Status Fiber::stop() {
        if(running()) {
            _status = FiberStatus::eStopped;
            return turbo::fiber_internal::fiber_stop_impl(_fid);
        }
        if(_status == FiberStatus::eStopped || _status == FiberStatus::eJoined) {
            return turbo::ok_status();
        }
        return turbo::make_status(kESTOP, "Fiber is not running.");
    }

    Fiber::~Fiber() {
        if(joinable()) {
            TLOG_CRITICAL("You need to call either `join()` or `detach()` prior to destroy "
                          "a fiber. Otherwise the behavior is undefined.");
            std::terminate();
        }
    }

    void Fiber::start_span(void *parent) {
        turbo::fiber_internal::tls_bls.parent_span = parent;
    }

    void *Fiber::get_span() {
        return turbo::fiber_internal::tls_bls.parent_span;
    }

    void Fiber::end_span(void *parent) {
        if(turbo::fiber_internal::tls_bls.parent_span == parent) {
            turbo::fiber_internal::tls_bls.parent_span = nullptr;
        }
    }

    std::string Fiber::describe() const {
        memory_buffer buffer;
        turbo::fiber_internal::FiberWorker::print_fiber(buffer, _fid);
        return {buffer.data(), buffer.size()};
    }

    void Fiber::describe(std::ostream &os) const {
        turbo::fiber_internal::FiberWorker::print_fiber(os, _fid);
    }

    void Fiber::fiber_flush() {
        turbo::fiber_internal::fiber_flush_impl();
    }

    int Fiber::fiber_about_to_quit() {
        return turbo::fiber_internal::fiber_about_to_quit_impl();
    }

    bool Fiber::exists(fiber_id_t fid) {
        return turbo::fiber_internal::FiberWorker::exists(fid);
    }

    bool Fiber::is_running_on_fiber() {
        return turbo::fiber_internal::FiberWorker::is_running_on_fiber();
    }

    bool Fiber::is_running_on_pthread() {
        return turbo::fiber_internal::FiberWorker::is_running_on_pthread();
    }

    fiber_id_t Fiber::fiber_self(void) {
        return turbo::fiber_internal::fiber_self_impl();
    }

    int Fiber::equal(fiber_id_t t1, fiber_id_t t2) {
        return turbo::fiber_internal::fiber_equal_impl(t1, t2);
    }

    turbo::Status Fiber::sleep_until(const turbo::Time& deadline) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::FiberWorker::sleep(&g, deadline);
        }
        turbo::sleep_until(deadline);
        return turbo::ok_status();
    }

    turbo::Status Fiber::sleep_for(const turbo::Duration& span) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::FiberWorker::sleep(&g, span);
        }
        turbo::sleep_for(span);
        return turbo::ok_status();
    }

    turbo::Status Fiber::yield() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            turbo::fiber_internal::FiberWorker::yield(&g);
            return turbo::ok_status();
        }
        // pthread_yield is not available on MAC
        auto r = sched_yield();
        return r == 0 ? turbo::ok_status() : turbo::make_status();
    }

    void Fiber::print(std::ostream &os, fiber_id_t tid) {
        turbo::fiber_internal::FiberWorker::print_fiber(os, tid);
    }

    std::string Fiber::print(fiber_id_t tid) {
        std::ostringstream oss;
        turbo::fiber_internal::FiberWorker::print_fiber(oss, tid);
        return oss.str();
    }
}  // namespace turbo

