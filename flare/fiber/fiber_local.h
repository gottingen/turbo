
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_FIBER_FIBER_LOCAL_H_
#define FLARE_FIBER_FIBER_LOCAL_H_

#include <memory>
#include "flare/fiber/internal/types.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/unstable.h"

namespace flare {

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

}  // namespace flare

#endif  // FLARE_FIBER_FIBER_LOCAL_H_
