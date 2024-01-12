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


#ifndef  TURBO_FIBER_INTERNAL_MUTEX_H_
#define  TURBO_FIBER_INTERNAL_MUTEX_H_

#include "turbo/fiber/internal/types.h"
#include "turbo/status/status.h"

namespace turbo::fiber_internal {

    struct fiber_mutex_t {
        unsigned *event;
    };

    struct fiber_mutexattr_t {
    };

    [[maybe_unused]] turbo::Status fiber_mutex_init(fiber_mutex_t *__restrict mutex,
                                                    const fiber_mutexattr_t *__restrict mutex_attr);

    [[maybe_unused]] void fiber_mutex_destroy(fiber_mutex_t *mutex);

    [[maybe_unused]] turbo::Status fiber_mutex_trylock(fiber_mutex_t *mutex);

    [[maybe_unused]] turbo::Status fiber_mutex_lock(fiber_mutex_t *mutex);

    [[maybe_unused]] turbo::Status fiber_mutex_timedlock(fiber_mutex_t *__restrict mutex,
                                                         const struct timespec *__restrict abstime);

    [[maybe_unused]] void fiber_mutex_unlock(fiber_mutex_t *mutex);

    [[maybe_unused]] turbo::Status fiber_mutex_lock_contended(fiber_mutex_t *m);

}  // namespace turbo::fiber_internal

namespace std {
    using namespace turbo::fiber_internal;

    template<>
    class unique_lock<fiber_mutex_t> { TURBO_NON_COPYABLE(unique_lock);

    public:
        typedef fiber_mutex_t mutex_type;

        unique_lock() : _mutex(nullptr), _owns_lock(false) {}

        explicit unique_lock(mutex_type &mutex)
                : _mutex(&mutex), _owns_lock(false) {
            lock();
        }

        unique_lock(mutex_type &mutex, defer_lock_t)
                : _mutex(&mutex), _owns_lock(false) {}

        unique_lock(mutex_type &mutex, try_to_lock_t)
                : _mutex(&mutex), _owns_lock(fiber_mutex_trylock(&mutex).ok()) {}

        unique_lock(mutex_type &mutex, adopt_lock_t)
                : _mutex(&mutex), _owns_lock(true) {}

        ~unique_lock() {
            if (_owns_lock) {
                unlock();
            }
        }

        void lock() {
            if (!_mutex) {
                TDLOG_CHECK(false, "Invalid operation");
                return;
            }
            if (_owns_lock) {
                TDLOG_CHECK(false, "Detected deadlock issue");
                return;
            }
            _owns_lock = fiber_mutex_lock(_mutex).ok();
        }

        bool try_lock() {
            if (!_mutex) {
                TDLOG_CHECK(false, "Invalid operation");
                return false;
            }
            if (_owns_lock) {
                TLOG_CHECK(false, "Detected deadlock issue");
                return false;
            }
            _owns_lock = fiber_mutex_trylock(_mutex).ok();
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                TDLOG_CHECK("Invalid operation");
                return;
            }
            if (_mutex) {
                fiber_mutex_unlock(_mutex);
                _owns_lock = false;
            }
        }

        void swap(unique_lock &rhs) {
            std::swap(_mutex, rhs._mutex);
            std::swap(_owns_lock, rhs._owns_lock);
        }

        mutex_type *release() {
            mutex_type *saved_mutex = _mutex;
            _mutex = nullptr;
            _owns_lock = false;
            return saved_mutex;
        }

        mutex_type *mutex() { return _mutex; }

        bool owns_lock() const { return _owns_lock; }

        operator bool() const { return owns_lock(); }

    private:
        mutex_type *_mutex;
        bool _owns_lock;
    };

}  // namespace std


#endif  // TURBO_FIBER_INTERNAL_MUTEX_H_
