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
// Created by jeff on 24-1-5.
//

#ifndef TURBO_FIBER_FIBER_LOCAL_H_
#define TURBO_FIBER_FIBER_LOCAL_H_

#include <memory>
#include "turbo/fiber/internal/key.h"

namespace turbo {

    using turbo::fiber_internal::fiber_keytable_pool_t;
    using turbo::fiber_internal::fiber_keytable_pool_stat_t;
    using turbo::fiber_internal::fiber_local_key;
    using turbo::fiber_internal::key_pool_ctor_t;

    inline int fiber_keytable_pool_init(fiber_keytable_pool_t *pool) {
        return turbo::fiber_internal::fiber_keytable_pool_init(pool);
    }

    inline int fiber_keytable_pool_destroy(fiber_keytable_pool_t *pool) {
        return turbo::fiber_internal::fiber_keytable_pool_destroy(pool);
    }

    inline int fiber_keytable_pool_getstat(fiber_keytable_pool_t *pool,
                                           fiber_keytable_pool_stat_t *stat) {
        return turbo::fiber_internal::fiber_keytable_pool_getstat(pool, stat);
    }

    inline void fiber_keytable_pool_reserve(
            fiber_keytable_pool_t *pool, size_t nfree,
            fiber_local_key key, const key_pool_ctor_t &ctor, const void *args) {
        turbo::fiber_internal::fiber_keytable_pool_reserve(pool, nfree, key, ctor, args);

    }

    inline void fiber_assign_data(void *data) {
        turbo::fiber_internal::fiber_assign_data(data);
    }

    inline void *fiber_get_assigned_data() {
        return turbo::fiber_internal::fiber_get_assigned_data();
    }

}

namespace turbo {

    template<typename T>
    inline void local_dtor(void *data, const void *args) {
        delete static_cast<T *>(data);
    }

    /**
     * @ingroup turbo_fiber
     * @brief fiber local storage
     * @tparam T
     */
    template<class T>
    class FiberLocal {
    public:
        // A dedicated FLS slot is allocated for this `fiber_local`.
        FiberLocal() {
            fiber_key_create2(&_key, local_dtor<T>, nullptr);
            T *d = std::make_unique<T>().release();
            fiber_setspecific(_key, d);
        }

        // The FLS slot is released on destruction.
        ~FiberLocal() {
            fiber_key_delete(_key);
        }

        // Accessor.
        T *operator->() const noexcept { return get(); }

        T &operator*() const noexcept { return *get(); }

        T *get() const noexcept { return static_cast<T *>(get_impl()); }

    private:

        void *get_impl() const noexcept {
            return turbo::fiber_internal::fiber_getspecific(_key);
        }

    private:
        fiber_local_key _key;
    };

}  // namespace turbo

#endif  // TURBO_FIBER_FIBER_LOCAL_H_
