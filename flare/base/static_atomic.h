
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_STATIC_ATOMIC_H_
#define FLARE_BASE_STATIC_ATOMIC_H_

#include <atomic>

#define FLARE_STATIC_ATOMIC_INIT(val) { (val) }

namespace flare {

    template<typename T>
    struct static_atomic {
        T val;

        // NOTE: the std::memory_order parameters must be present.
        T load(std::memory_order o) { return ref().load(o); }

        void store(T v, std::memory_order o) { return ref().store(v, o); }

        T exchange(T v, std::memory_order o) { return ref().exchange(v, o); }

        bool compare_exchange_weak(T &e, T d, std::memory_order o) { return ref().compare_exchange_weak(e, d, o); }

        bool compare_exchange_weak(T &e, T d, std::memory_order so, std::memory_order fo) {
            return ref().compare_exchange_weak(e, d, so, fo);
        }

        bool compare_exchange_strong(T &e, T d, std::memory_order o) { return ref().compare_exchange_strong(e, d, o); }

        bool compare_exchange_strong(T &e, T d, std::memory_order so, std::memory_order fo) {
            return ref().compare_exchange_strong(e, d, so, fo);
        }

        T fetch_add(T v, std::memory_order o) { return ref().fetch_add(v, o); }

        T fetch_sub(T v, std::memory_order o) { return ref().fetch_sub(v, o); }

        T fetch_and(T v, std::memory_order o) { return ref().fetch_and(v, o); }

        T fetch_or(T v, std::memory_order o) { return ref().fetch_or(v, o); }

        T fetch_xor(T v, std::memory_order o) { return ref().fetch_xor(v, o); }

        static_atomic &operator=(T v) {
            store(v, std::memory_order_seq_cst);
            return *this;
        }

    private:
        static_atomic(const static_atomic &) = delete;

        static_assert(sizeof(T) == sizeof(std::atomic<T>), "size must match");

        std::atomic<T> &ref() {
            // Suppress strict-alias warnings.
            std::atomic<T> *p = reinterpret_cast<std::atomic<T> *>(&val);
            return *p;
        }
    };
}  // namespace flare
#endif  // FLARE_BASE_STATIC_ATOMIC_H_
