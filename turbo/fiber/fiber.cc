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

namespace turbo {

    turbo::Status Fiber::start(fiber_fn_t &&fn, void *args) {
        return turbo::fiber_internal::fiber_start_urgent(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
    }

    turbo::Status Fiber::start(LaunchPolicy policy, fiber_fn_t &&fn, void *args) {
        if (policy == LaunchPolicy::eImmediately) {
            return turbo::fiber_internal::fiber_start_urgent(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
        } else {
            return turbo::fiber_internal::fiber_start_background(&_fid, &FIBER_ATTR_NORMAL, std::move(fn), args);
        }
    }

    turbo::Status Fiber::start(LaunchPolicy policy, const FiberAttribute attr, fiber_fn_t &&fn, void *args) {
        if (policy == LaunchPolicy::eImmediately) {
            return turbo::fiber_internal::fiber_start_urgent(&_fid, &attr, std::move(fn), args);
        } else {
            return turbo::fiber_internal::fiber_start_background(&_fid, &attr, std::move(fn), args);
        }
    }

    void Fiber::join() {
        if (!_detached && valid()) {
            auto rs = turbo::fiber_internal::fiber_join(_fid, nullptr);
            TURBO_UNUSED(rs);
        }
        _fid = INVALID_FIBER_ID;
    }

    void Fiber::detach() {
        _detached = true;
        _fid = INVALID_FIBER_ID;
    }

    bool Fiber::stopped() const {
        if (_detached && _fid != INVALID_FIBER_ID) {
            return turbo::fiber_internal::fiber_stopped(_fid) == 0;
        }
        return true;
    }

    void Fiber::stop() {
        if(valid()) {
            TURBO_UNUSED(turbo::fiber_internal::fiber_stop(_fid));
        }
    }

    Fiber::~Fiber() {
        if (!_detached && _fid != INVALID_FIBER_ID) {
            stop();
            join();
        }
    }

    void Fiber::fiber_flush() {
        turbo::fiber_internal::fiber_flush();
    }

    int Fiber::fiber_about_to_quit() {
        return turbo::fiber_internal::fiber_about_to_quit();
    }
}  // namespace turbo

