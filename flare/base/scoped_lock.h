
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_SCOPED_LOCK_H_
#define FLARE_BASE_SCOPED_LOCK_H_

#include <mutex>                           // std::lock_guard
#include "flare/log/logging.h"
#include "flare/base/errno.h"
#include "flare/base/profile.h"
#include "flare/base/compat.h"

namespace flare::base {
    namespace detail {
        template<typename T>
        std::lock_guard<typename std::remove_reference<T>::type> get_lock_guard();
    }  // namespace detail
}  // namespace flare::base

#define FLARE_SCOPED_LOCK(ref_of_lock)                                  \
    decltype(::flare::base::detail::get_lock_guard<decltype(ref_of_lock)>()) \
    FLARE_CONCAT(scoped_locker_dummy_at_line_, __LINE__)(ref_of_lock)

namespace std {

#if defined(FLARE_PLATFORM_POSIX)

    template<> class lock_guard<pthread_mutex_t> {
    public:
        explicit lock_guard(pthread_mutex_t & mutex) : _pmutex(&mutex) {
#if !defined(NDEBUG)
            const int rc = pthread_mutex_lock(_pmutex);
            if (rc) {
                FLARE_LOG(FATAL) << "Fail to lock pthread_mutex_t=" << _pmutex << ", " << flare_error(rc);
                _pmutex = nullptr;
            }
#else
            pthread_mutex_lock(_pmutex);
#endif  // NDEBUG
        }

        ~lock_guard() {
#ifndef NDEBUG
            if (_pmutex) {
                pthread_mutex_unlock(_pmutex);
            }
#else
            pthread_mutex_unlock(_pmutex);
#endif
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(lock_guard);
        pthread_mutex_t* _pmutex;
    };

    template<> class lock_guard<pthread_spinlock_t> {
    public:
        explicit lock_guard(pthread_spinlock_t & spin) : _pspin(&spin) {
#if !defined(NDEBUG)
            const int rc = pthread_spin_lock(_pspin);
            if (rc) {
                FLARE_LOG(FATAL) << "Fail to lock pthread_spinlock_t=" << _pspin << ", " << flare_error(rc);
                _pspin = nullptr;
            }
#else
            pthread_spin_lock(_pspin);
#endif  // NDEBUG
        }

        ~lock_guard() {
#ifndef NDEBUG
            if (_pspin) {
                pthread_spin_unlock(_pspin);
            }
#else
            pthread_spin_unlock(_pspin);
#endif
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(lock_guard);
        pthread_spinlock_t* _pspin;
    };

    template<> class unique_lock<pthread_mutex_t> {
        FLARE_DISALLOW_COPY_AND_ASSIGN(unique_lock);
    public:
        typedef pthread_mutex_t         mutex_type;
        unique_lock() : _mutex(nullptr), _owns_lock(false) {}
        explicit unique_lock(mutex_type& mutex)
            : _mutex(&mutex), _owns_lock(true) {
            pthread_mutex_lock(_mutex);
        }
        unique_lock(mutex_type& mutex, defer_lock_t)
            : _mutex(&mutex), _owns_lock(false)
        {}
        unique_lock(mutex_type& mutex, try_to_lock_t)
            : _mutex(&mutex), _owns_lock(pthread_mutex_trylock(&mutex) == 0)
        {}
        unique_lock(mutex_type& mutex, adopt_lock_t)
            : _mutex(&mutex), _owns_lock(true)
        {}

        ~unique_lock() {
            if (_owns_lock) {
                pthread_mutex_unlock(_mutex);
            }
        }

        void lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return;
            }
#if !defined(NDEBUG)
            const int rc = pthread_mutex_lock(_mutex);
            if (rc) {
                FLARE_LOG(FATAL) << "Fail to lock pthread_mutex=" << _mutex << ", " << flare_error(rc);
                return;
            }
            _owns_lock = true;
#else
            _owns_lock = true;
            pthread_mutex_lock(_mutex);
#endif  // NDEBUG
        }

        bool try_lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return false;
            }
            _owns_lock = !pthread_mutex_trylock(_mutex);
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                FLARE_CHECK(false) << "Invalid operation";
                return;
            }
            pthread_mutex_unlock(_mutex);
            _owns_lock = false;
        }

        void swap(unique_lock& rhs) {
            std::swap(_mutex, rhs._mutex);
            std::swap(_owns_lock, rhs._owns_lock);
        }

        mutex_type* release() {
            mutex_type* saved_mutex = _mutex;
            _mutex = nullptr;
            _owns_lock = false;
            return saved_mutex;
        }

        mutex_type* mutex() { return _mutex; }
        bool owns_lock() const { return _owns_lock; }
        operator bool() const { return owns_lock(); }

    private:
        mutex_type*                     _mutex;
        bool                            _owns_lock;
    };

    template<> class unique_lock<pthread_spinlock_t> {
        FLARE_DISALLOW_COPY_AND_ASSIGN(unique_lock);
    public:
        typedef pthread_spinlock_t  mutex_type;
        unique_lock() : _mutex(nullptr), _owns_lock(false) {}
        explicit unique_lock(mutex_type& mutex)
            : _mutex(&mutex), _owns_lock(true) {
            pthread_spin_lock(_mutex);
        }

        ~unique_lock() {
            if (_owns_lock) {
                pthread_spin_unlock(_mutex);
            }
        }
        unique_lock(mutex_type& mutex, defer_lock_t)
            : _mutex(&mutex), _owns_lock(false)
        {}
        unique_lock(mutex_type& mutex, try_to_lock_t)
            : _mutex(&mutex), _owns_lock(pthread_spin_trylock(&mutex) == 0)
        {}
        unique_lock(mutex_type& mutex, adopt_lock_t)
            : _mutex(&mutex), _owns_lock(true)
        {}

        void lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return;
            }
#if !defined(NDEBUG)
            const int rc = pthread_spin_lock(_mutex);
            if (rc) {
                FLARE_LOG(FATAL) << "Fail to lock pthread_spinlock=" << _mutex << ", " << flare_error(rc);
                return;
            }
            _owns_lock = true;
#else
            _owns_lock = true;
            pthread_spin_lock(_mutex);
#endif  // NDEBUG
        }

        bool try_lock() {
            if (_owns_lock) {
                FLARE_CHECK(false) << "Detected deadlock issue";
                return false;
            }
            _owns_lock = !pthread_spin_trylock(_mutex);
            return _owns_lock;
        }

        void unlock() {
            if (!_owns_lock) {
                FLARE_CHECK(false) << "Invalid operation";
                return;
            }
            pthread_spin_unlock(_mutex);
            _owns_lock = false;
        }

        void swap(unique_lock& rhs) {
            std::swap(_mutex, rhs._mutex);
            std::swap(_owns_lock, rhs._owns_lock);
        }

        mutex_type* release() {
            mutex_type* saved_mutex = _mutex;
            _mutex = nullptr;
            _owns_lock = false;
            return saved_mutex;
        }

        mutex_type* mutex() { return _mutex; }
        bool owns_lock() const { return _owns_lock; }
        operator bool() const { return owns_lock(); }

    private:
        mutex_type*                     _mutex;
        bool                            _owns_lock;
    };

#endif  // defined(FLARE_PLATFORM_POSIX)

}  // namespace std

namespace flare::base {

// Lock both lck1 and lck2 without the dead lock issue
    template<typename Mutex1, typename Mutex2>
    void double_lock(std::unique_lock<Mutex1> &lck1, std::unique_lock<Mutex2> &lck2) {
        FLARE_DCHECK(!lck1.owns_lock());
        FLARE_DCHECK(!lck2.owns_lock());
        volatile void *const ptr1 = lck1.mutex();
        volatile void *const ptr2 = lck2.mutex();
        FLARE_DCHECK_NE(ptr1, ptr2);
        if (ptr1 < ptr2) {
            lck1.lock();
            lck2.lock();
        } else {
            lck2.lock();
            lck1.lock();
        }
    }

};

#endif  // FLARE_BASE_SCOPED_LOCK_H_
