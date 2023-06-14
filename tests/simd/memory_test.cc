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


#include "turbo/simd/simd.h"
#ifndef TURBO_NO_SUPPORTED_ARCHITECTURE

#include <type_traits>
#include <vector>

#include "doctest/doctest.h"

#include "turbo/simd/memory/aligned_allocator.h"
#include "turbo/simd/memory/alignment.h"

struct mock_container
{
};

TEST_CASE("[alignment]")
{
    using u_vector_type = std::vector<double>;
    using a_vector_type = std::vector<double, turbo::simd::default_allocator<double>>;

    using u_vector_align = turbo::simd::container_alignment_t<u_vector_type>;
    using a_vector_align = turbo::simd::container_alignment_t<a_vector_type>;
    using mock_align = turbo::simd::container_alignment_t<mock_container>;

    CHECK_UNARY((std::is_same<u_vector_align, turbo::simd::unaligned_mode>::value));
    CHECK_UNARY((std::is_same<a_vector_align, turbo::simd::aligned_mode>::value));
    CHECK_UNARY((std::is_same<mock_align, turbo::simd::unaligned_mode>::value));
}

TEST_CASE("[is_aligned]")
{
    float f[100];
    void* unaligned_f = static_cast<void*>(&f[0]);
    constexpr std::size_t alignment = turbo::simd::default_arch::alignment();
    std::size_t aligned_f_size;
    void* aligned_f = std::align(alignment, sizeof(f), unaligned_f, aligned_f_size);
    CHECK_UNARY(turbo::simd::is_aligned(aligned_f));

    // GCC does not generate correct alignment on ARM
    // (see https://godbolt.org/z/obv1n8bWq)
#if !(TURBO_WITH_NEON && defined(__GNUC__) && !defined(__clang__))
    alignas(alignment) char aligned[8];
    CHECK_UNARY(turbo::simd::is_aligned(&aligned[0]));
    CHECK_UNARY(!turbo::simd::is_aligned(&aligned[3]));
#endif
}
#endif
