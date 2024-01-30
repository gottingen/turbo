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
// Created by jeff on 24-1-16.
//

#ifndef TURBO_FIBER_WAIT_EVENT_H_
#define TURBO_FIBER_WAIT_EVENT_H_

#include <atomic>
#include "turbo/platform/port.h"
#include "turbo/meta/type_traits.h"
#include "turbo/fiber/internal/waitable_event.h"

namespace turbo {

    namespace fiber_internal {
        template<typename T>
        struct is_atomic_type : public std::false_type {
        };

        template<typename T>
        struct is_atomic_type<std::atomic<T>> : public std::true_type {
        };

        template<typename T, bool>
        struct atomic_traits {
            using value_type = T;
        };

        template<typename T>
        struct atomic_traits<T,true> {
            using value_type = typename T::value_type;
        };

    }  // namespace fiber_internal
    template<typename T>
    class WaitEvent {
    public:

        static constexpr bool is_atomic = fiber_internal::is_atomic_type<T>::value;
        using value_type = typename fiber_internal::atomic_traits<T, is_atomic>::value_type;
        static constexpr bool is_valid_type = std::is_same_v<value_type, int> ||
                                              std::is_same_v<value_type, unsigned int> ||
                                              std::is_same_v<value_type, uint32_t> ||
                                              std::is_same_v<value_type, int32_t>;

        static_assert(sizeof(T) == sizeof(int), "sizeof T must_equal int");
        static_assert(is_valid_type, "T must be int or unsigned int or uint32_t or int32_t");

        static constexpr Duration kMinTimeout = Duration::microseconds(2);

        WaitEvent() = default;

        ~WaitEvent();

        WaitEvent(const WaitEvent &) = delete;

        WaitEvent &operator=(const WaitEvent &) = delete;

        WaitEvent(WaitEvent &&rhs) noexcept
                :event_(rhs.event_) {
            rhs.waiters_.store(0, std::memory_order_relaxed);
            rhs.event_ = nullptr;
        }

        WaitEvent &operator=(WaitEvent &&rhs) noexcept {
            if (this != &rhs) {
                rhs.waiters_.store(0, std::memory_order_relaxed);
                event_ = rhs.event_;
                rhs.event_ = nullptr;
            }
            return *this;
        }

        turbo::Status initialize(value_type event = value_type{0});

        turbo::Status wait(value_type expected = value_type{0});

        turbo::Status wait_for(turbo::Duration d, value_type expected = 0);

        turbo::Status wait_until(turbo::Time deadline, value_type expected = 0);

        int notify_one();

        int notify_all();

        int notify_all_fiber();

        void destroy();

        constexpr bool is_valid() const;

        constexpr void store(value_type event);

        constexpr value_type load() const;

        constexpr T *event();

        constexpr operator bool() const { return is_valid(); }

        constexpr WaitEvent &operator=(value_type event);

    private:
        T *event_{nullptr};
    };

    /// inline functions
    template<typename T>
    inline turbo::Status WaitEvent<T>::initialize(value_type event) {
        TURBO_ASSERT(event_ == nullptr);
        event_ = turbo::fiber_internal::waitable_event_create_checked<T>();
        if (event_ == nullptr) {
            return turbo::make_status(kENOMEM);
        }
        if constexpr (is_atomic) {
            event_->store(event, std::memory_order_relaxed);
        } else {
            *event_ = event;
        }
        return turbo::ok_status();
    }

    template<typename T>
    inline turbo::Status WaitEvent<T>::wait(value_type expected) {
        TURBO_ASSERT(event_ != nullptr);
        auto r = turbo::fiber_internal::waitable_event_wait(event_, static_cast<int>(expected), Time::infinite_future());
        return r;
    }

    template<typename T>
    inline turbo::Status WaitEvent<T>::wait_for(turbo::Duration d, value_type expected) {
        TURBO_ASSERT(event_ != nullptr);
        auto deadline = (turbo::Time::time_now() + d);
        auto r = turbo::fiber_internal::waitable_event_wait(event_, static_cast<int>(expected), deadline);
        return r;
    }

    template<typename T>
    inline turbo::Status WaitEvent<T>::wait_until(turbo::Time deadline, value_type expected) {
        TURBO_ASSERT(event_ != nullptr);
        auto r = turbo::fiber_internal::waitable_event_wait(event_, static_cast<int>(expected), deadline);
        return r;
    }

    template<typename T>
    inline int WaitEvent<T>::notify_one() {
        TURBO_ASSERT(event_ != nullptr);
        return turbo::fiber_internal::waitable_event_wake(event_);

    }

    template<typename T>
    inline int WaitEvent<T>::notify_all() {
        TURBO_ASSERT(event_ != nullptr);
        return turbo::fiber_internal::waitable_event_wake_all(event_);
    }

    template<typename T>
    inline int WaitEvent<T>::notify_all_fiber() {
        TURBO_ASSERT(event_ != nullptr);
        return turbo::fiber_internal::waitable_event_wake_except(event_, 0);

    }

    template<typename T>
    void WaitEvent<T>::destroy() {
        if (event_ != nullptr) {
            turbo::fiber_internal::waitable_event_destroy(event_);
            event_ = nullptr;
        }
    }

    template<typename T>
    inline constexpr bool WaitEvent<T>::is_valid() const {
        return event_ != nullptr;
    }

    template<typename T>
    inline constexpr void WaitEvent<T>::store(value_type event) {
        TURBO_ASSERT(event_ != nullptr);
        if constexpr (is_atomic) {
            event_->store(event, std::memory_order_relaxed);
        } else {
            *event_ = event;
        }
    }

    template<typename T>
    inline constexpr typename WaitEvent<T>::value_type WaitEvent<T>::load() const {
        TURBO_ASSERT(event_ != nullptr);
        if constexpr (is_atomic) {
            return event_->load(std::memory_order_relaxed);
        } else {
            return *event_;
        }
    }

    template<typename T>
    inline constexpr T *WaitEvent<T>::event() {
        return event_;
    }

    template<typename T>
    constexpr WaitEvent<T> &WaitEvent<T>::operator=(value_type event) {
        store(event);
        return *this;
    }

    template<typename T>
    WaitEvent<T>::~WaitEvent() {
        destroy();
    }

}  // namespace turbo
#endif  // TURBO_FIBER_WAIT_EVENT_H_
