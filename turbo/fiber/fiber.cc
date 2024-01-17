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

    turbo::Status Fiber::start(fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_urgent(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
        if(rs.ok()) {
            _status = FiberStatus::eRunning;
        }
        return rs;
    }

    turbo::Status Fiber::start(const FiberAttribute attr, fiber_fn_t &&fn, void *args) {
        if(!startable()) {
            return turbo::make_status(kEEXIST, "Fiber already started.");
        }
        auto rs = turbo::fiber_internal::fiber_start_urgent(&_fid, &attr, std::move(fn), args);
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
            auto rs = turbo::fiber_internal::fiber_start_urgent(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        } else {
            auto rs = turbo::fiber_internal::fiber_start_background(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
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
            auto rs = turbo::fiber_internal::fiber_start_urgent(&_fid, &attr, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        } else {
            auto rs = turbo::fiber_internal::fiber_start_background(&_fid, &attr, std::move(fn), args);
            if(rs.ok()) {
                _status = FiberStatus::eRunning;
            }
            return rs;
        }
    }

    turbo::Status Fiber::join(void **retval) {
        if (joinable()) {
            auto rs = turbo::fiber_internal::fiber_join(_fid, retval);
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
            return turbo::fiber_internal::fiber_stop(_fid);
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

    fiber_id_t fiber_self() {
        return turbo::fiber_internal::fiber_self();
    }

    void Fiber::fiber_flush() {
        turbo::fiber_internal::fiber_flush();
    }

    int Fiber::fiber_about_to_quit() {
        return turbo::fiber_internal::fiber_about_to_quit();
    }

    bool Fiber::exists(fiber_id_t fid) {
        return turbo::fiber_internal::FiberWorker::exists(fid);
    }
}  // namespace turbo

