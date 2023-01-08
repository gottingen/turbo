
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_THREAD_THREAD_CACHE_H_
#define FLARE_THREAD_THREAD_CACHE_H_


#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include "flare/thread/thread_local.h"

namespace flare {

    // This class helps you optimizing read-mostly shared data access by caching
    // data locally in TLS.
    template<class T>
    class thread_cache {
    public:
        template<class... Us>
        explicit thread_cache(Us &&... args) : value_(std::forward<Us>(args)...) {}

        // `Get` tests if thread-local cached object is up-to-date, and uses
        // thread-local only if it is, avoiding touching internally shared mutex or
        // global data.
        //
        // If the cached object is out-of-date, slow path (acquiring global mutex and
        // update the cache) is taken instead.
        //
        // CAUTION: TWO CONSECUTIVE CALLS TO `get()` CAN RETURN REF. TO DIFFERENT
        // OBJECTS. BESIDES, IF THIS IS THE CASE, THE FIRST REF. IS INVALIDATED BEFORE
        // THE SECOND CALL RETURNS.
        const T &non_idempotent_get() const {
            auto p = tls_cache_.get();
            if (FLARE_UNLIKELY(p->version !=
                               version_.load(std::memory_order_relaxed))) {
                return get_slow();
            }
            return *p->object;
        }

        // Use `args...` to reinitialize value stored.
        //
        // Note that each call to `Emplace` will cause subsequent calls to `Get()` to
        // acquire internal mutex (once per thread). So don't call `Emplace` unless
        // the value is indeed changed.
        //
        // Calls to `Emplace` acquire internal mutex, it's slow.
        template<class... Us>
        void emplace(Us &&... args) {
            std::scoped_lock _(lock_);
            value_ = T(std::forward<Us>(args)...);
            // `value_` is always accessed with `lock_` held, so we don't need extra
            // fence when accessing `version_`.
            version_.fetch_add(1, std::memory_order_relaxed);
        }

        // TODO(yinbinli): Support for replacing value stored by functor's return
        // value, with internally lock held.

    private:

        const T &get_slow() const;

    private:
        struct cache_entry {
            std::uint64_t version = 0;
            std::unique_ptr<T> object;
        };

        std::atomic<std::uint64_t> version_{1};  // Incremented each time `value_`
        // is changed.
        thread_internal::thread_local_always_initialized<cache_entry> tls_cache_;

        // I do think it's possible to optimize the lock away with `Hazptr` and
        // seqlocks.
        mutable std::shared_mutex lock_;
        T value_;
    };

    // NOT inlined (to keep fast-path `get()` small.).
    template<class T>
    const T &thread_cache<T>::get_slow() const {
        std::shared_lock _(lock_);
        auto p = tls_cache_.get();
        p->version = version_.load(std::memory_order_relaxed);
        p->object = std::make_unique<T>(value_);
        return *p->object;
    }

}  // namespace flare

#endif  // FLARE_THREAD_THREAD_CACHE_H_
