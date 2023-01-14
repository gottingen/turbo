
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_FIBER_FIBER_LOCAL_H_
#define TURBO_FIBER_FIBER_LOCAL_H_

#include <memory>
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/fiber/internal/unstable.h"

namespace turbo {

    template <typename T>
    inline void local_dtor(void *data, const void *args) {
        delete static_cast<T*>(data);
    }

    template<class T>
    class fiber_local {
    public:
        // A dedicated FLS slot is allocated for this `fiber_local`.
        fiber_local() {
            fiber_key_create2(&_key, local_dtor<T>, nullptr);
            T *d = std::make_unique<T>().release();
            fiber_setspecific(_key, d);
        }

        // The FLS slot is released on destruction.
        ~fiber_local() {
            fiber_key_delete(_key);
        }

        // Accessor.
        T *operator->() const noexcept { return get(); }

        T &operator*() const noexcept { return *get(); }

        T *get() const noexcept { return static_cast<T *>(get_impl()); }

    private:

        void *get_impl() const noexcept {
            return ::fiber_getspecific(_key);
        }

    private:
        fiber_local_key _key;
    };

}  // namespace turbo

#endif  // TURBO_FIBER_FIBER_LOCAL_H_
