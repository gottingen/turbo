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

#ifndef TURBO_FIBER_FIBER_H_
#define TURBO_FIBER_FIBER_H_

#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/base/status.h"

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief fiber function type
     */
    using fiber_internal::fiber_fn_t;
    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FiberAttribute;
    using fiber_internal::StackType;
    using fiber_internal::AttributeFlag;
    /**
     * @ingroup turbo_fiber
     * @brief fiber id type
     */
    using fiber_internal::fiber_id_t;

    /**
     * @ingroup turbo_fiber
     * @brief constant invalid fiber id
     */
    using fiber_internal::INVALID_FIBER_ID;

    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FIBER_ATTR_PTHREAD;

    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FIBER_ATTR_SMALL;

    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FIBER_ATTR_NORMAL;

    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FIBER_ATTR_LARGE;

    /**
     * @ingroup turbo_fiber
     * @brief fiber attribute
     */
    using fiber_internal::FIBER_ATTR_DEBUG;

    /**
     * @ingroup turbo_fiber
     * @brief fiber launch policy,
     *        start fiber immediately or lazy
     */
    enum class LaunchPolicy {
        eImmediately,
        eLazy
    };

    /**
     * @ingroup turbo_fiber
     * @brief fiber class
     */
    class Fiber {
    public:

    public:
        // Create an empty (invalid) fiber.
        Fiber() = default;

        ~Fiber();

        Fiber(Fiber &&other) noexcept;

        Fiber &operator=(Fiber &&other) noexcept;

        turbo::Status start(fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start(LaunchPolicy policy, fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start(LaunchPolicy policy, const FiberAttribute attr, fiber_fn_t &&fn, void *args = nullptr);


        fiber_id_t self() const {
            return _fid;
        }

        // Wait for the fiber to exit.
        void join();

        void detach();

        bool stopped() const;

        void stop();

       bool valid() const {
            return _fid != INVALID_FIBER_ID;
        }

        static void fiber_flush();

        // Mark the calling fiber as "about to quit". When the fiber is scheduled,
        // worker pthreads are not notified.
        static int fiber_about_to_quit();

    private:
        // nolint
        TURBO_NON_COPYABLE(Fiber);

        fiber_id_t _fid{INVALID_FIBER_ID};
        bool _detached{false};
    };

    /// inlined functions
    inline Fiber::Fiber(Fiber &&other) noexcept {
        _fid = other._fid;
        _detached = other._detached;
        other._fid = INVALID_FIBER_ID;
        other._detached = false;
    }

    inline Fiber &Fiber::operator=(Fiber &&other) noexcept {
        if (this != &other) {
            _fid = other._fid;
            _detached = other._detached;
            other._fid = INVALID_FIBER_ID;
            other._detached = false;
        }
        return *this;
    }
}  // namespace turbo
#endif // TURBO_FIBER_FIBER_H_
