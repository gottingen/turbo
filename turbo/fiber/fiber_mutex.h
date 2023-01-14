
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_FIBER_FIBER_MUTEX_H_
#define TURBO_FIBER_FIBER_MUTEX_H_

#include "turbo/fiber/internal/mutex.h"

namespace turbo {
    // The C++ Wrapper of fiber_mutex

    // NOTE: Not aligned to cacheline as the container of fiber_mutex is practically aligned
    class fiber_mutex {
    public:
        typedef fiber_mutex_t *native_handler_type;

        fiber_mutex() {
            int ec = fiber_mutex_init(&_mutex, nullptr);
            if (ec != 0) {
                throw std::system_error(std::error_code(ec, std::system_category()), "fiber_mutex constructor failed");
            }
        }

        ~fiber_mutex() { TURBO_CHECK_EQ(0, fiber_mutex_destroy(&_mutex)); }

        native_handler_type native_handler() { return &_mutex; }

        void lock() {
            int ec = fiber_mutex_lock(&_mutex);
            if (ec != 0) {
                throw std::system_error(std::error_code(ec, std::system_category()), "fiber_mutex lock failed");
            }
        }

        void unlock() { fiber_mutex_unlock(&_mutex); }

        bool try_lock() { return !fiber_mutex_trylock(&_mutex); }
        // TODO(jeff.li): Complement interfaces for C++11
    private:
        TURBO_DISALLOW_COPY_AND_ASSIGN(fiber_mutex);

        fiber_mutex_t _mutex;
    };
}  // namespace turbo

#endif // TURBO_FIBER_FIBER_MUTEX_H_
