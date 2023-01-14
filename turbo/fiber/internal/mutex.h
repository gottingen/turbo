// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: 2015/12/14 18:17:04

#ifndef  TURBO_FIBER_INTERNAL_MUTEX_H_
#define  TURBO_FIBER_INTERNAL_MUTEX_H_

#include "turbo/fiber/internal/types.h"
#include "turbo/base/scoped_lock.h"
#include "turbo/metrics/utils/lock_timer.h"

__BEGIN_DECLS
extern int fiber_mutex_init(fiber_mutex_t *__restrict mutex,
                            const fiber_mutexattr_t *__restrict mutex_attr);
extern int fiber_mutex_destroy(fiber_mutex_t *mutex);
extern int fiber_mutex_trylock(fiber_mutex_t *mutex);
extern int fiber_mutex_lock(fiber_mutex_t *mutex);
extern int fiber_mutex_timedlock(fiber_mutex_t *__restrict mutex,
                                 const struct timespec *__restrict abstime);
extern int fiber_mutex_unlock(fiber_mutex_t *mutex);
__END_DECLS

namespace turbo::fiber_internal {

    namespace internal {

        class FastPthreadMutex {
        public:
            FastPthreadMutex() : _futex(0) {}

            ~FastPthreadMutex() {}

            void lock();

            void unlock();

            bool try_lock();

        private:
            TURBO_DISALLOW_COPY_AND_ASSIGN(FastPthreadMutex);

            int lock_contended();

            unsigned _futex;
        };

    }

}  // namespace turbo::fiber_internal

// Specialize std::lock_guard and std::unique_lock for fiber_mutex_t

namespace std {

    template<>
    class lock_guard<fiber_mutex_t> {
    public:
        explicit lock_guard(fiber_mutex_t &mutex) : _pmutex(&mutex) {
#if !defined(NDEBUG)
            const int rc = fiber_mutex_lock(_pmutex);
            if (rc) {
                TURBO_LOG(FATAL) << "Fail to lock fiber_mutex_t=" << _pmutex << ", " << turbo_error(rc);
                _pmutex = nullptr;
            }
#else
            fiber_mutex_lock(_pmutex);
#endif  // NDEBUG
        }

        ~lock_guard() {
#ifndef NDEBUG
            if (_pmutex) {
                fiber_mutex_unlock(_pmutex);
            }
#else
            fiber_mutex_unlock(_pmutex);
#endif
        }

    private:
        TURBO_DISALLOW_COPY_AND_ASSIGN(lock_guard);

        fiber_mutex_t *_pmutex;
    };

    template<>
    class unique_lock<fiber_mutex_t> {
        TURBO_DISALLOW_COPY_AND_ASSIGN(unique_lock);

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
                : _mutex(&mutex), _owns_lock(fiber_mutex_trylock(&mutex) == 0) {}

        unique_lock(mutex_type &mutex, adopt_lock_t)
                : _mutex(&mutex), _owns_lock(true) {}

        ~unique_lock() {
            if (_owns_lock) {
                unlock();
            }
        }

        void lock() {
            if (!_mutex) {
                TURBO_CHECK(false) << "Invalid operation";
                return;
            }
            if (_owns_lock) {
                TURBO_CHECK(false) << "Detected deadlock issue";
                return;
            }
            fiber_mutex_lock(_mutex);
            _owns_lock = true;
        }

        bool try_lock() {
            if (!_mutex) {
                TURBO_CHECK(false) << "Invalid operation";
                return false;
            }
            if (_owns_lock) {
                TURBO_CHECK(false) << "Detected deadlock issue";
                return false;
            }
            _owns_lock = !fiber_mutex_trylock(_mutex);
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                TURBO_CHECK(false) << "Invalid operation";
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

namespace turbo {

    template<>
    struct MutexConstructor<fiber_mutex_t> {
        bool operator()(fiber_mutex_t *mutex) const {
            return fiber_mutex_init(mutex, nullptr) == 0;
        }
    };

    template<>
    struct MutexDestructor<fiber_mutex_t> {
        bool operator()(fiber_mutex_t *mutex) const {
            return fiber_mutex_destroy(mutex) == 0;
        }
    };

}  // namespace turbo

#endif  // TURBO_FIBER_INTERNAL_MUTEX_H_
