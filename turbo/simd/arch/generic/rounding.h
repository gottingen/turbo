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

#ifndef TURBO_SIMD_ARCH_GENERIC_ROUNDING_H_
#define TURBO_SIMD_ARCH_GENERIC_ROUNDING_H_

#include "turbo/simd/arch/generic/details.h"

namespace turbo::simd::kernel {

    using namespace types;

    // ceil
    template<class A, class T>
    inline batch<T, A> ceil(batch<T, A> const &self, requires_arch<generic>) noexcept {
        batch<T, A> truncated_self = trunc(self);
        return select(truncated_self < self, truncated_self + 1, truncated_self);
    }

    // floor
    template<class A, class T>
    inline batch<T, A> floor(batch<T, A> const &self, requires_arch<generic>) noexcept {
        batch<T, A> truncated_self = trunc(self);
        return select(truncated_self > self, truncated_self - 1, truncated_self);
    }

    // round
    template<class A, class T>
    inline batch<T, A> round(batch<T, A> const &self, requires_arch<generic>) noexcept {
        auto v = abs(self);
        auto c = ceil(v);
        auto cp = select(c - 0.5 > v, c - 1, c);
        return select(v > constants::maxflint<batch<T, A>>(), self, copysign(cp, self));
    }

    // trunc
    template<class A, class T, class = typename std::enable_if<std::is_integral<T>::value, void>::type>
    inline batch<T, A> trunc(batch<T, A> const &self, requires_arch<generic>) noexcept {
        return self;
    }

    template<class A>
    inline batch<float, A> trunc(batch<float, A> const &self, requires_arch<generic>) noexcept {
        return select(abs(self) < constants::maxflint<batch<float, A>>(), to_float(to_int(self)), self);
    }

    template<class A>
    inline batch<double, A> trunc(batch<double, A> const &self, requires_arch<generic>) noexcept {
        return select(abs(self) < constants::maxflint<batch<double, A>>(), to_float(to_int(self)), self);
    }


}  // namespace turbo::simd::kernel

#endif  // TURBO_SIMD_ARCH_GENERIC_ROUNDING_H_
