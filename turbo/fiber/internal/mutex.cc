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

// Date: Sun Aug  3 12:46:15 CST 2014

#include <pthread.h>
#include <execinfo.h>
#include "turbo/files/filesystem.h"
#include <dlfcn.h>                               // dlsym
#include <fcntl.h>                               // O_RDONLY
#include <atomic>
#include <memory>
#include "turbo/hash/hash.h"
#include "turbo/log/logging.h"
#include "turbo/memory/object_pool.h"
#include "turbo/fiber/internal/waitable_event.h"                       // waitable_event_*
#include "turbo/base/processor.h"                   // cpu_relax, barrier
#include "turbo/fiber/internal/mutex.h"                       // fiber_mutex_t

extern "C" {
extern void *_dl_sym(void *handle, const char *symbol, void *caller);
}

namespace turbo::fiber_internal {
    // Replace pthread_mutex_lock and pthread_mutex_unlock:
    // First call to sys_pthread_mutex_lock sets sys_pthread_mutex_lock to the
    // real function so that next calls go to the real function directly. This
    // technique avoids calling pthread_once each time.
    typedef int (*MutexOp)(pthread_mutex_t *);

    int first_sys_pthread_mutex_lock(pthread_mutex_t *mutex);

    int first_sys_pthread_mutex_unlock(pthread_mutex_t *mutex);

    static MutexOp sys_pthread_mutex_lock = first_sys_pthread_mutex_lock;
    static MutexOp sys_pthread_mutex_unlock = first_sys_pthread_mutex_unlock;
    static pthread_once_t init_sys_mutex_lock_once = PTHREAD_ONCE_INIT;

    static void init_sys_mutex_lock() {
#if defined(TURBO_PLATFORM_LINUX)
        // TODO: may need dlvsym when GLIBC has multiple versions of a same symbol.
        // http://blog.fesnel.com/blog/2009/08/25/preloading-with-multiple-symbol-versions
        sys_pthread_mutex_lock = (MutexOp) _dl_sym(RTLD_NEXT, "pthread_mutex_lock", (void *) init_sys_mutex_lock);
        sys_pthread_mutex_unlock = (MutexOp) _dl_sym(RTLD_NEXT, "pthread_mutex_unlock", (void *) init_sys_mutex_lock);
#elif defined(TURBO_PLATFORM_OSX)
        // TODO: look workaround for dlsym on mac
        sys_pthread_mutex_lock = (MutexOp) dlsym(RTLD_NEXT, "pthread_mutex_lock");
        sys_pthread_mutex_unlock = (MutexOp) dlsym(RTLD_NEXT, "pthread_mutex_unlock");
#endif
    }

    // Make sure pthread functions are ready before main().
    const int TURBO_MAYBE_UNUSED
            dummy = pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);

    int first_sys_pthread_mutex_lock(pthread_mutex_t *mutex) {
        pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);
        return sys_pthread_mutex_lock(mutex);
    }

    int first_sys_pthread_mutex_unlock(pthread_mutex_t *mutex) {
        pthread_once(&init_sys_mutex_lock_once, init_sys_mutex_lock);
        return sys_pthread_mutex_unlock(mutex);
    }

    TURBO_FORCE_INLINE int pthread_mutex_lock_impl(pthread_mutex_t *mutex) {
        return sys_pthread_mutex_lock(mutex);
    }

    TURBO_FORCE_INLINE int pthread_mutex_unlock_impl(pthread_mutex_t *mutex) {
        return sys_pthread_mutex_unlock(mutex);
    }

    // Implement fiber_mutex_t related functions
    struct MutexInternal {
        std::atomic<unsigned char> locked;
        std::atomic<unsigned char> contended;
        unsigned short padding;
    };

    const MutexInternal MUTEX_CONTENDED_RAW = {{1}, {1}, 0};
    const MutexInternal MUTEX_LOCKED_RAW = {{1}, {0}, 0};
// Define as macros rather than constants which can't be put in read-only
// section and affected by initialization-order fiasco.
#define FIBER_MUTEX_CONTENDED (*(const unsigned*)&turbo::fiber_internal::MUTEX_CONTENDED_RAW)
#define FIBER_MUTEX_LOCKED (*(const unsigned*)&turbo::fiber_internal::MUTEX_LOCKED_RAW)

    static_assert(sizeof(unsigned) == sizeof(MutexInternal),
                  "sizeof_mutex_internal_must_equal_unsigned");

    inline turbo::Status mutex_lock_contended(fiber_mutex_t *m) {
        std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
        while (whole->exchange(FIBER_MUTEX_CONTENDED) & FIBER_MUTEX_LOCKED) {
            auto rs = turbo::fiber_internal::waitable_event_wait(whole, FIBER_MUTEX_CONTENDED, nullptr);
            if (!rs.ok() && !is_unavailable(rs)) {
                // a mutex lock should ignore interrruptions in general since
                // user code is unlikely to check the return value.
                return rs;
            }
        }
        return turbo::ok_status();
    }

    inline turbo::Status mutex_timedlock_contended(
            fiber_mutex_t *m, const struct timespec *__restrict abstime) {
        std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
        while (whole->exchange(FIBER_MUTEX_CONTENDED) & FIBER_MUTEX_LOCKED) {
            auto rs = turbo::fiber_internal::waitable_event_wait(whole, FIBER_MUTEX_CONTENDED, abstime);
            if (!rs.ok() && !is_unavailable(rs)) {
                // a mutex lock should ignore interrruptions in general since
                // user code is unlikely to check the return value.
                return rs;
            }
        }
        return turbo::ok_status();
    }

} // namespace turbo::fiber_internal

namespace turbo::fiber_internal {

    turbo::Status fiber_mutex_init(fiber_mutex_t *__restrict m,
                         const fiber_mutexattr_t *__restrict) {
        m->event = turbo::fiber_internal::waitable_event_create_checked<unsigned>();
        if (!m->event) {
            return turbo::resource_exhausted_error("");
        }
        *m->event = 0;
        return turbo::ok_status();
    }

    void fiber_mutex_destroy(fiber_mutex_t *m) {
        turbo::fiber_internal::waitable_event_destroy(m->event);
    }

    turbo::Status fiber_mutex_trylock(fiber_mutex_t *m) {
        turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
        if (!split->locked.exchange(1, std::memory_order_acquire)) {
            return turbo::ok_status();
        }
        return turbo::resource_busy_error("");
    }

    turbo::Status fiber_mutex_lock_contended(fiber_mutex_t *m) {
        return turbo::fiber_internal::mutex_lock_contended(m);
    }

    turbo::Status fiber_mutex_lock(fiber_mutex_t *m) {
        turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
        if (!split->locked.exchange(1, std::memory_order_acquire)) {
            return turbo::ok_status();
        }

        return turbo::fiber_internal::mutex_lock_contended(m);
    }

    turbo::Status fiber_mutex_timedlock(fiber_mutex_t *__restrict m,
                                        const struct timespec *__restrict abstime) {
        turbo::fiber_internal::MutexInternal *split = (turbo::fiber_internal::MutexInternal *) m->event;
        if (!split->locked.exchange(1, std::memory_order_acquire)) {
            return turbo::ok_status();
        }
        return turbo::fiber_internal::mutex_timedlock_contended(m, abstime);

    }

    void fiber_mutex_unlock(fiber_mutex_t *m) {
        std::atomic<unsigned> *whole = (std::atomic<unsigned> *) m->event;
        const unsigned prev = whole->exchange(0, std::memory_order_release);
        // CAUTION: the mutex may be destroyed, check comments before waitable_event_create
        if (prev == FIBER_MUTEX_LOCKED) {
            return;
        }
        // Wakeup one waiter
        turbo::fiber_internal::waitable_event_wake(whole);
        return;
    }

    int pthread_mutex_lock(pthread_mutex_t *__mutex) {
        return turbo::fiber_internal::pthread_mutex_lock_impl(__mutex);
    }

    int pthread_mutex_unlock(pthread_mutex_t *__mutex) {
        return turbo::fiber_internal::pthread_mutex_unlock_impl(__mutex);
    }

}  // namespace turbo::fiber_internal
