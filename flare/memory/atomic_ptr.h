
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_MEMORY_ATOMIC_PTR_H_
#define FLARE_MEMORY_ATOMIC_PTR_H_


#include "flare/base/copyable_atomic.h"
#include "flare/memory/ref_ptr.h"

namespace flare {

    // Smart pointers that are safe to be read by multiple threads simultaneously.
    //
    // Note that it's still UNSAFE to assign to them from multiple threads. To be
    // precise, it's UNSAFE to assign to this pointer in ANY FASHION at all.
    // Preventing accessing it concurrently in unsafe fashion is done by the user of
    // these "atomic" pointer types.
    //
    // They're not intended for general use. The only reason they're here is for
    // `ThreadLocal` to implement `for_each` in a thread-safe manner.

    // `ref_ptr`, with assignment and read being implemented by `CopyableAtomic`.
    template<class T>
    class atomic_ref_ptr {
        using Traits = ref_traits<T>;

    public:
        atomic_ref_ptr() = default;

        // Used only when relocating TLS array. Performance doesn't matter.
        atomic_ref_ptr(atomic_ref_ptr &&from) noexcept
                : ptr_(from.ptr_.load(std::memory_order_acquire)) {
            from.ptr_.store(nullptr, std::memory_order_relaxed);
        }

        ~atomic_ref_ptr() { clear(); }

        void clear() {
            if (auto ptr = ptr_.load(std::memory_order_acquire)) {
                Traits::Dereference(ptr);
            }
            ptr_.store(nullptr, std::memory_order_relaxed);
        }

        void set(ref_ptr<T> from) noexcept {
            clear();
            ptr_.store(from.Leak(), std::memory_order_release);
        }

        T *get() const noexcept { return ptr_.load(std::memory_order_acquire); }

        T *leak() noexcept {
            auto ptr = ptr_.load(std::memory_order_acquire);
            ptr_.store(nullptr, std::memory_order_relaxed);
            return ptr;
        }

        atomic_ref_ptr &operator=(const atomic_ref_ptr &) = default;

    private:
        copyable_atomic<T *> ptr_{};
    };

    // Non-shared ownership, helps implementing `ThreadLocal<T>`.
    template<class T>
    class atomic_scoped_ptr {
    public:
        atomic_scoped_ptr() = default;

        // Used only when relocating TLS array.
        atomic_scoped_ptr(atomic_scoped_ptr &&from) noexcept
                : ptr_(from.ptr_.load(std::memory_order_acquire)) {
            from.ptr_.store(nullptr, std::memory_order_relaxed);
        }

        ~atomic_scoped_ptr() { clear(); }

        void clear() {
            if (auto ptr = ptr_.load(std::memory_order_acquire)) {
                delete ptr;
            }
            ptr_.store(nullptr, std::memory_order_relaxed);
        }

        void set(std::unique_ptr<T> ptr) {
            clear();
            ptr_.store(ptr.release(), std::memory_order_release);
        }

        T *get() const noexcept { return ptr_.load(std::memory_order_acquire); }

        T *leak() noexcept {
            auto ptr = ptr_.load(std::memory_order_acquire);
            ptr_.store(nullptr, std::memory_order_relaxed);
            return ptr;
        }

        atomic_scoped_ptr &operator=(const atomic_scoped_ptr &) = delete;

    private:
        copyable_atomic<T *> ptr_{};
    };

}  // namespace flare

#endif  // FLARE_MEMORY_ATOMIC_PTR_H_
