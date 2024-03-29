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

#ifndef TURBO_SIMD_TYPES_SSE2_REGISTER_H_
#define TURBO_SIMD_TYPES_SSE2_REGISTER_H_

#include "turbo/simd/types/generic_arch.h"
#include "turbo/simd/types/register.h"

#if TURBO_WITH_SSE2

#include <emmintrin.h>
#include <xmmintrin.h>

#endif

namespace turbo::simd {
    /**
     * @ingroup architectures
     *
     * SSE2 instructions
     */
    struct sse2 : generic {
        static constexpr bool supported() noexcept { return TURBO_WITH_SSE2; }

        static constexpr bool available() noexcept { return true; }

        static constexpr bool requires_alignment() noexcept { return true; }

        static constexpr unsigned version() noexcept { return generic::version(1, 2, 0); }

        static constexpr std::size_t alignment() noexcept { return 16; }

        static constexpr char const *name() noexcept { return "sse2"; }
    };

#if TURBO_WITH_SSE2
    namespace types {
        TURBO_SIMD_DECLARE_SIMD_REGISTER(bool, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(signed char, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(unsigned char, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(char, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(unsigned short, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(short, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(unsigned int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(unsigned long int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(long int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(unsigned long long int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(long long int, sse2, __m128i);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(float, sse2, __m128);

        TURBO_SIMD_DECLARE_SIMD_REGISTER(double, sse2, __m128d);
    }
#endif
}

#endif  // TURBO_SIMD_TYPES_SSE2_REGISTER_H_
