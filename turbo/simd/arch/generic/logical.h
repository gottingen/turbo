// Copyright 2023 The titan-search Authors.
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

#ifndef TURBO_SIMD_ARCH_GENERIC_LOGICAL_H_
#define TURBO_SIMD_ARCH_GENERIC_LOGICAL_H_

#include "turbo/simd/arch/generic/details.h"

namespace turbo::simd::kernel {

    using namespace types;

    // from  mask
    template<class A, class T>
    inline batch_bool<T, A> from_mask(batch_bool<T, A> const &, uint64_t mask, requires_arch<generic>) noexcept {
        alignas(A::alignment()) bool buffer[batch_bool<T, A>::size];
        // This is inefficient but should never be called. It's just a
        // temporary implementation until arm support is added.
        for (size_t i = 0; i < batch_bool<T, A>::size; ++i)
            buffer[i] = mask & (1ull << i);
        return batch_bool<T, A>::load_aligned(buffer);
    }

    // ge
    template<class A, class T>
    inline batch_bool<T, A> ge(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return other <= self;
    }

    // gt
    template<class A, class T>
    inline batch_bool<T, A> gt(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return other < self;
    }

    // is_even
    template<class A, class T>
    inline batch_bool<T, A> is_even(batch<T, A> const &self, requires_arch<generic>) noexcept {
        return is_flint(self * T(0.5));
    }

    // is_flint
    template<class A, class T>
    inline batch_bool<T, A> is_flint(batch<T, A> const &self, requires_arch<generic>) noexcept {
        auto frac = select(isnan(self - self), constants::nan<batch<T, A>>(), self - trunc(self));
        return frac == T(0.);
    }

    // is_odd
    template<class A, class T>
    inline batch_bool<T, A> is_odd(batch<T, A> const &self, requires_arch<generic>) noexcept {
        return is_even(self - T(1.));
    }

    // isinf
    template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    inline batch_bool<T, A> isinf(batch<T, A> const &, requires_arch<generic>) noexcept {
        return batch_bool<T, A>(false);
    }

    template<class A>
    inline batch_bool<float, A> isinf(batch<float, A> const &self, requires_arch<generic>) noexcept {
        return abs(self) == std::numeric_limits<float>::infinity();
    }

    template<class A>
    inline batch_bool<double, A> isinf(batch<double, A> const &self, requires_arch<generic>) noexcept {
        return abs(self) == std::numeric_limits<double>::infinity();
    }

    // isfinite
    template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    inline batch_bool<T, A> isfinite(batch<T, A> const &, requires_arch<generic>) noexcept {
        return batch_bool<T, A>(true);
    }

    template<class A>
    inline batch_bool<float, A> isfinite(batch<float, A> const &self, requires_arch<generic>) noexcept {
        return (self - self) == 0.f;
    }

    template<class A>
    inline batch_bool<double, A> isfinite(batch<double, A> const &self, requires_arch<generic>) noexcept {
        return (self - self) == 0.;
    }

    // isnan
    template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    inline batch_bool<T, A> isnan(batch<T, A> const &, requires_arch<generic>) noexcept {
        return batch_bool<T, A>(false);
    }

    // le
    template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    inline batch_bool<T, A> le(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return (self < other) || (self == other);
    }

    // neq
    template<class A, class T>
    inline batch_bool<T, A>
    neq(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return !(other == self);
    }

    // logical_and
    template<class A, class T>
    inline batch<T, A>
    logical_and(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return detail::apply([](T x, T y) noexcept { return x && y; },
                             self, other);
    }

    // logical_or
    template<class A, class T>
    inline batch<T, A>
    logical_or(batch<T, A> const &self, batch<T, A> const &other, requires_arch<generic>) noexcept {
        return detail::apply([](T x, T y) noexcept { return x || y; },
                             self, other);
    }

    // mask
    template<class A, class T>
    inline uint64_t mask(batch_bool<T, A> const &self, requires_arch<generic>) noexcept {
        alignas(A::alignment()) bool buffer[batch_bool<T, A>::size];
        self.store_aligned(buffer);
        // This is inefficient but should never be called. It's just a
        // temporary implementation until arm support is added.
        uint64_t res = 0;
        for (size_t i = 0; i < batch_bool<T, A>::size; ++i)
            if (buffer[i])
                res |= 1ul << i;
        return res;
    }
}  // namespace turbo::simd::kernel

#endif  // TURBO_SIMD_ARCH_GENERIC_LOGICAL_H_
