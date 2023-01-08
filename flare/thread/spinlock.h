
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_THREAD_SPINLOCK_H_
#define FLARE_THREAD_SPINLOCK_H_

#include <atomic>
#include <mutex>
#include "flare/base/profile.h"
#include "flare/log/logging.h"


namespace flare {

    // TODO:tsan
    // TODO: using pthread_spinlock_t?
    class spinlock {
    public:
        void lock() noexcept {
            // Here we try to grab the lock first before failing back to TTAS.
            //
            // If the lock is not contend, this fast-path should be quicker.
            // If the lock is contended and we have to fail back to slow TTAS, this
            // single try shouldn't add too much overhead.
            //
            // What's more, by keeping this method small, chances are higher that this
            // method get inlined by the compiler.
            if (FLARE_LIKELY(try_lock())) {
                return;
            }

            // Slow path otherwise.
            lock_slow();
        }

        bool try_lock() noexcept {
            return !locked_.exchange(true, std::memory_order_acquire);
        }

        void unlock() noexcept { locked_.store(false, std::memory_order_release); }

    private:
        void lock_slow() noexcept;

    private:
        std::atomic<bool> locked_{false};
    };
}  // namespace flare::base

namespace std {

    template<>
    class lock_guard<flare::spinlock> {
    public:
        explicit lock_guard(flare::spinlock &mutex) : _pmutex(&mutex) {
            _pmutex->lock();
        }

        ~lock_guard() {
            _pmutex->unlock();
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(lock_guard);

        flare::spinlock *_pmutex;
    };

    template<>
    class unique_lock<flare::spinlock> {
        FLARE_DISALLOW_COPY_AND_ASSIGN(unique_lock);

    public:
        typedef flare::spinlock mutex_type;

        unique_lock() : _mutex(nullptr), _owns_lock(false) {}

        explicit unique_lock(mutex_type &mutex)
                : _mutex(&mutex), _owns_lock(true) {
            _mutex->lock();
        }

        ~unique_lock() {
            if (_owns_lock) {
                _mutex->unlock();
            }
        }

        unique_lock(mutex_type &mutex, defer_lock_t)
                : _mutex(&mutex), _owns_lock(false) {}

        unique_lock(mutex_type &mutex, try_to_lock_t)
                : _mutex(&mutex), _owns_lock(mutex.try_lock()) {}

        unique_lock(mutex_type &mutex, adopt_lock_t)
                : _mutex(&mutex), _owns_lock(true) {}

        void lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return;
            }
            _owns_lock = true;
            _mutex->lock();
        }

        bool try_lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return false;
            }
            _owns_lock = _mutex->try_lock();
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                FLARE_CHECK(false) << "Invalid operation";
                return;
            }
            _mutex->unlock();
            _owns_lock = false;
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

#endif  // FLARE_BASE_THREAD_SPINLOCK_H_
