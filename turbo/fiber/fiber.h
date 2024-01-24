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
#include "turbo/fiber/fiber_local.h"
#include "turbo/fiber/fiber_cond.h"
#include "turbo/fiber/fiber_mutex.h"
#include "turbo/fiber/fiber_session.h"
#include "turbo/fiber/timer.h"
#include "turbo/fiber/wait_event.h"
#include "turbo/fiber/runtime.h"
#include "turbo/status/status.h"
#include "turbo/format/format.h"

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
    using fiber_internal::FiberSessionImpl;
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

    using fiber_internal::FIBER_ATTR_NORMAL_WITH_SPAN;

    ///////////////////////////// c api ////////////////////////////////

    [[maybe_unused]] turbo::Status fiber_start(fiber_id_t *TURBO_RESTRICT tid,
                                                      const FiberAttribute *TURBO_RESTRICT attr,
                                                      fiber_fn_t &&fn,
                                                      void *TURBO_RESTRICT args);

    [[maybe_unused]] turbo::Status fiber_start_background(fiber_id_t *TURBO_RESTRICT tid,
                                                          const FiberAttribute *TURBO_RESTRICT attr,
                                                          fiber_fn_t &&fn,
                                                          void *TURBO_RESTRICT args);

    turbo::Status fiber_interrupt(fiber_id_t tid);

    turbo::Status fiber_stop(fiber_id_t tid);

    bool fiber_stopped(fiber_id_t tid);

    void fiber_exit(void *retval) __attribute__((__noreturn__));

    turbo::Status fiber_join(fiber_id_t bt, void **fiber_return);

    /**
     * @ingroup turbo_fiber
     * @brief fiber launch policy,
     *        start fiber immediately or lazy
     */
    enum class LaunchPolicy {
        eImmediately,
        eLazy
    };

    enum class FiberStatus {
        eInvalid,
        eRunning,
        eStopped,
        eDetached,
        eJoined
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

        constexpr Fiber(Fiber &&other) noexcept;

        constexpr Fiber &operator=(Fiber &&other) noexcept;

        turbo::Status start(fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start(const FiberAttribute attr, fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start_lazy(fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start_lazy(const FiberAttribute attr, fiber_fn_t &&fn, void *args = nullptr);


        turbo::Status start(LaunchPolicy policy, fiber_fn_t &&fn, void *args = nullptr);

        turbo::Status start(LaunchPolicy policy, const FiberAttribute attr, fiber_fn_t &&fn, void *args = nullptr);


        fiber_id_t self() const {
            return _fid;
        }

        // Wait for the fiber to exit.
        turbo::Status join(void **retval = nullptr);

        void detach();

        turbo::Status stop();

        constexpr bool running() const {
            return _fid != INVALID_FIBER_ID && _status == FiberStatus::eRunning;
        }

        constexpr bool joinable() const {
            return _fid != INVALID_FIBER_ID && _status > FiberStatus::eInvalid && _status < FiberStatus::eDetached;
        }

        constexpr bool startable() const {
            return _fid == INVALID_FIBER_ID && _status == FiberStatus::eInvalid;
        }

        std::string describe() const;

        void describe(std::ostream &os) const;

        static void fiber_flush();

        // Mark the calling fiber as "about to quit". When the fiber is scheduled,
        // worker pthreads are not notified.
        static int fiber_about_to_quit();

        static bool exists(fiber_id_t fid);

        static void start_span(void *parent);

        static void *get_span();

        static void end_span(void *parent);

        static bool is_running_on_fiber();

        static bool is_running_on_pthread();

        static fiber_id_t fiber_self(void);

        static int equal(fiber_id_t t1, fiber_id_t t2);

        static turbo::Status sleep_until(const turbo::Time &deadline);

        static turbo::Status sleep_for(const turbo::Duration &span);

        static turbo::Status sleep(uint64_t sec);

        static turbo::Status usleep(uint64_t usec);

        static turbo::Status msleep(uint64_t msec);

        static turbo::Status nsleep(uint64_t nsec);

        static  turbo::Status yield();

        static void print(std::ostream &os, fiber_id_t tid);

        static std::string print(fiber_id_t tid);
    private:
        // nolint
        TURBO_NON_COPYABLE(Fiber);
        FiberStatus _status{FiberStatus::eInvalid};
        fiber_id_t _fid{INVALID_FIBER_ID};
    };

    /**
     * @ingroup turbo_fiber
     * @brief get current fiber id
     * @return current fiber id
     */
    fiber_id_t get_fiber_id();


    /// inlined functions
    inline constexpr Fiber::Fiber(Fiber &&other) noexcept {
        _fid = other._fid;
        _status = other._status;
        other._fid = INVALID_FIBER_ID;
        other._status = FiberStatus::eInvalid;
    }

    inline constexpr Fiber &Fiber::operator=(Fiber &&other) noexcept {
        if (this != &other) {
            _fid = other._fid;
            _status = other._status;
            other._fid = INVALID_FIBER_ID;
            other._status = FiberStatus::eInvalid;
        }
        return *this;
    }

    /// inlined functions
    inline turbo::Status Fiber::usleep(uint64_t usec) {
        return sleep_for(turbo::Duration::microseconds(usec));
    }

    inline turbo::Status Fiber::msleep(uint64_t msec) {
        return sleep_for(turbo::Duration::milliseconds(msec));
    }

    inline turbo::Status Fiber::nsleep(uint64_t nsec) {
        return sleep_for(turbo::Duration::nanoseconds(nsec));
    }

    inline turbo::Status Fiber::sleep(uint64_t sec) {
        return sleep_for(turbo::Duration::seconds(sec));
    }

    template<>
    struct formatter<Fiber> {
        bool details = false;
        constexpr auto parse(basic_format_parse_context<char> &ctx)
        -> decltype(ctx.begin()) {
            auto begin = ctx.begin(), end = ctx.end();
            if (begin != end && (*begin == 'd' || *begin == 'D')) {
                ++begin;
                details = true;
            }
            return begin;
        }

        template<typename FormatContext>
        auto format(const Fiber &f, FormatContext &ctx) {
            if(details) {
                return format_to(ctx.out(), "{}", f.describe());
            } else {
                return format_to(ctx.out(), "{}", f.self());
            }
        }
    };
}  // namespace turbo
#endif // TURBO_FIBER_FIBER_H_
