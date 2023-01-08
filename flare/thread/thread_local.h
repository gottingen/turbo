
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_THREAD_THREAD_LOCAL_H_
#define FLARE_THREAD_THREAD_LOCAL_H_


#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <functional>
#include "flare/log/logging.h"
#include "flare/thread/internal/always_initialized.h"
#include "flare/memory/atomic_ptr.h"

namespace flare {

    // Performance note:
    //
    // In some versions of Google's gperftools (tcmalloc), allocating memory from
    // different threads often results in adjacent addresses (within a cacheline
    // boundary). This allocation scheme CAN EASILY LEAD TO FALSE-SHARING AND HURT
    // PERFORMANCE. As we often use `thread_local_store<T>` for perf. optimization, this (I
    // would say it's a bug) totally defeat the reason why we want a "thread-local"
    // copy in the first place.
    //
    // Due to technical reasons (we don't have control on how instances of `T` are
    // allocated), we can't workaround this quirk for you automatically, you need to
    // annotate your `T` with `alignas` yourself.
    //
    // TODO(yinbinli): It the default constructor of `thread_local_store<T>` is used, we
    // indeed can do alignment for the user.

    // Support thread-local storage, with extra capability to traverse all instances
    // among threads.
    //
    // IT'S EXPLICITLY NOT SUPPORTED TO CONSTRUCT / DESTROY OTHER THREAD-LOCAL
    // VARIABLES IN CONSTRUCTOR / DESTRUCTOR OF THIS CLASS.
    template<class T>
    class thread_local_store {
    public:
        constexpr thread_local_store()
                : thread_local_store([]() { return std::make_unique<T>(); }) {}

        template<
                class F,
                std::enable_if_t<std::is_invocable_r_v<std::unique_ptr<T>, F>> * = nullptr>
        explicit thread_local_store(F &&creator) : creator_(std::forward<F>(creator)) {}

        T *get() const {
            // NOT locked, I'm not sure if this is right. (However, it nonetheless
            // should work without problem and nobody else should make it non-null.)
            auto &&ptr = raw_tls_->get();
            return FLARE_LIKELY(!!ptr) ? ptr : get_slow();
        }

        T *operator->() const { return get(); }

        T &operator*() const { return *get(); }

        T *leak() noexcept {
            std::scoped_lock _(init_lock_);
            return raw_tls_->leak();
        }

        void reset(std::unique_ptr<T> ptr = nullptr) noexcept {
            std::scoped_lock _(init_lock_);
            raw_tls_->set(std::move(ptr));
        }

        // `for_each` calls `f` with pointer (i.e. `T*`) to each thread-local
        // instances.
        //
        // CAUTION: Called with internal lock held. You may not touch TLS in `f`.
        template<class F>
        void for_each(F &&f) const {
            std::scoped_lock _(init_lock_);
            raw_tls_.for_each([&](auto *p) {
                if (auto ptr = p->get()) {
                    f(ptr);
                }
            });
        }

        // Noncopyable, nonmovable.
        thread_local_store(const thread_local_store &) = delete;

        thread_local_store &operator=(const thread_local_store &) = delete;

    private:
        [[gnu::noinline]] T *get_slow() const {
            std::scoped_lock _(init_lock_);
            raw_tls_->set(creator_());
            return raw_tls_->get();
        }

        mutable thread_internal::thread_local_always_initialized<flare::atomic_scoped_ptr<T>> raw_tls_;
        mutable std::mutex init_lock_;
        std::function<std::unique_ptr<T>()> creator_;
    };

}  // namespace flare
#endif  // FLARE_THREAD_THREAD_LOCAL_H_
