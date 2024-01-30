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

#ifndef TURBO_FIBER_INTERNAL_KEY_H_
#define TURBO_FIBER_INTERNAL_KEY_H_

#include "turbo/platform/port.h"
#include <cstdint>
#include <ostream>
#include <functional>
#include "turbo/format/format.h"

namespace turbo::fiber_internal {

    typedef std::function<void*(const void*)> key_pool_ctor_t;

    // Key of thread-local data, created by fiber_key_create.
    typedef struct {
        uint32_t index;    // index in KeyTable
        uint32_t version;  // ABA avoidance
    } fiber_local_key;

    static const fiber_local_key INVALID_FIBER_KEY = {0, 0};


    // Overload operators for fiber_local_key
    inline bool operator==(fiber_local_key key1, fiber_local_key key2) {
        return key1.index == key2.index && key1.version == key2.version;
    }

    inline bool operator!=(fiber_local_key key1, fiber_local_key key2) { return !(key1 == key2); }

    inline bool operator<(fiber_local_key key1, fiber_local_key key2) {
        return key1.index != key2.index ? (key1.index < key2.index) :
               (key1.version < key2.version);
    }

    inline bool operator>(fiber_local_key key1, fiber_local_key key2) { return key2 < key1; }

    inline bool operator<=(fiber_local_key key1, fiber_local_key key2) { return !(key2 < key1); }

    inline bool operator>=(fiber_local_key key1, fiber_local_key key2) { return !(key1 < key2); }

    inline std::ostream &operator<<(std::ostream &os, fiber_local_key key) {
        return os << "fiber_local_key{index=" << key.index << " version="
                  << key.version << '}';
    }


    // ---------------------------------------------------------------------
    // Functions for handling thread-specific data.
    // Notice that they can be used in pthread: get pthread-specific data in
    // pthreads and get fiber-specific data in fibers.
    // ---------------------------------------------------------------------

    // Create a key value identifying a slot in a thread-specific data area.
    // Each thread maintains a distinct thread-specific data area.
    // `destructor', if non-nullptr, is called with the value associated to that key
    // when the key is destroyed. `destructor' is not called if the value
    // associated is nullptr when the key is destroyed.
    // Returns 0 on success, error code otherwise.
    int fiber_key_create(fiber_local_key *key,
                                void (*destructor)(void *data));

    // Delete a key previously returned by fiber_key_create().
    // It is the responsibility of the application to free the data related to
    // the deleted key in any running thread. No destructor is invoked by
    // this function. Any destructor that may have been associated with key
    // will no longer be called upon thread exit.
    // Returns 0 on success, error code otherwise.
    int fiber_key_delete(fiber_local_key key);

    // Store `data' in the thread-specific slot identified by `key'.
    // fiber_setspecific() is callable from within destructor. If the application
    // does so, destructors will be repeatedly called for at most
    // PTHREAD_DESTRUCTOR_ITERATIONS times to clear the slots.
    // NOTE: If the thread is not created by turbo server and lifetime is
    // very short(doing a little thing and exit), avoid using fiber-local. The
    // reason is that fiber-local always allocate keytable on first call to
    // fiber_setspecific, the overhead is negligible in long-lived threads,
    // but noticeable in shortly-lived threads. Threads in turbo server
    // are special since they reuse keytables from a fiber_keytable_pool_t
    // in the server.
    // Returns 0 on success, error code otherwise.
    // If the key is invalid or deleted, return EINVAL.
    int fiber_setspecific(fiber_local_key key, void *data);

    // Return current value of the thread-specific slot identified by `key'.
    // If fiber_setspecific() had not been called in the thread, return nullptr.
    // If the key is invalid or deleted, return nullptr.
    void *fiber_getspecific(fiber_local_key key);

    // Create a fiber_local_key with an additional arg to destructor.
    // Generally the dtor_arg is for passing the creator of data so that we can
    // return the data back to the creator in destructor. Without this arg, we
    // have to do an extra heap allocation to contain data and its creator.
    int fiber_key_create2(fiber_local_key *key,
                                 void (*destructor)(void *data, const void *dtor_arg),
                                 const void *dtor_arg);


    // CAUTION: functions marked with [RPC INTERNAL] are NOT supposed to be called
    // by RPC users.
    
    // [RPC INTERNAL]
    // Create a pool to cache KeyTables so that frequently created/destroyed
    // fibers reuse these tables, namely when a fiber needs a KeyTable,
    // it fetches one from the pool instead of creating on heap. When a fiber
    // exits, it puts the table back to pool instead of deleting it.
    // Returns 0 on success, error code otherwise.
    int fiber_keytable_pool_init(fiber_keytable_pool_t *);

    // [RPC INTERNAL]
    // Destroy the pool. All KeyTables inside are destroyed.
    // Returns 0 on success, error code otherwise.
    int fiber_keytable_pool_destroy(fiber_keytable_pool_t *);
    
    // [RPC INTERNAL]
    // Put statistics of `pool' into `stat'.
    int fiber_keytable_pool_getstat(fiber_keytable_pool_t *pool,
                                           fiber_keytable_pool_stat_t *stat);

    // [RPC INTERNAL]
    // Reserve at most `nfree' keytables with `key' pointing to data created by
    // ctor(args).
    void fiber_keytable_pool_reserve(
            fiber_keytable_pool_t *pool, size_t nfree,
            fiber_local_key key, const key_pool_ctor_t &ctor, const void *args);

    void fiber_assign_data(void *data);

    void *fiber_get_assigned_data();

}  // namespace turbo::fiber_internal

namespace turbo {

    template <>
    struct formatter<fiber_internal::fiber_local_key> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext &ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const fiber_internal::fiber_local_key &key, FormatContext &ctx) {
            return format_to(ctx.out(), "fiber_local_key{{index={}, version={}}}",
                             key.index, key.version);
        }
    };
}  // namespace turbo
#endif  // TURBO_FIBER_INTERNAL_KEY_H_
