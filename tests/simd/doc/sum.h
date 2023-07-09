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

#ifndef _SUM_H_
#define _SUM_H_

#include "turbo/simd/simd.h"

// functor with a call method that depends on `Arch`
struct sum {
    // It's critical not to use an in-class definition here.
    // In-class and inline definition bypass extern template mechanism.
    template<class Arch, class T>
    T operator()(Arch, T const *data, unsigned size);
};

template<class Arch, class T>
T sum::operator()(Arch, T const *data, unsigned size) {
    using batch = turbo::simd::batch<T, Arch>;
    batch acc(static_cast<T>(0));
    const unsigned n = size / batch::size * batch::size;
    for (unsigned i = 0; i != n; i += batch::size)
        acc += batch::load_unaligned(data + i);
    T star_acc = turbo::simd::reduce_add(acc);
    for (unsigned i = n; i < size; ++i)
        star_acc += data[i];
    return star_acc;
}

// Inform the compiler that sse2 and avx2 implementation are to be found in another compilation unit.
extern template float sum::operator()<turbo::simd::avx2, float>(turbo::simd::avx2, float const *, unsigned);

extern template float sum::operator()<turbo::simd::avx, float>(turbo::simd::avx, float const *, unsigned);

extern template float sum::operator()<turbo::simd::sse2, float>(turbo::simd::sse2, float const *, unsigned);

#endif
