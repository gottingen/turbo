
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_COPYABLE_ATOMIC_H_
#define TURBO_BASE_COPYABLE_ATOMIC_H_


#include <atomic>

namespace turbo {

    // Make `std::atomic<T>` copyable.
    template<class T>
    class copyable_atomic : public std::atomic<T> {
    public:
        copyable_atomic() = default;

        /* implicit */ copyable_atomic(T value) : std::atomic<T>(std::move(value)) {}

        constexpr copyable_atomic(const copyable_atomic &from)
                : std::atomic<T>(from.load()) {}

        constexpr copyable_atomic &operator=(const copyable_atomic &from) {
            store(from.load());
            return *this;
        }
    };

}  // namespace turbo

#endif  // TURBO_BASE_COPYABLE_ATOMIC_H_
