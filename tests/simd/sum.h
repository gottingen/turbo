// Copyright 2023 The titan-search Authors.
// Copyright (c) Jeff.li
// Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and Martin Renou
// Copyright (c) QuantStack
// Copyright (c) Serge Guelton
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

#ifndef SUM_H_
#define SUM_H_
#include "turbo/simd/simd.h"
#ifndef TURBO_NO_SUPPORTED_ARCHITECTURE

struct sum
{
    // NOTE: no inline definition here otherwise extern template instantiation
    // doesn't prevent implicit instantiation.
    template <class Arch, class T>
    T operator()(Arch, T const* data, unsigned size);
};

template <class Arch, class T>
T sum::operator()(Arch, T const* data, unsigned size)
{
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

#if TURBO_WITH_AVX
extern template float sum::operator()(turbo::simd::avx, float const*, unsigned);
#endif

#endif

#endif  // SUM_H_
