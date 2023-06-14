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


#ifndef TURBO_SIMD_TYPES_SSE4_1_REGISTER_H_
#define TURBO_SIMD_TYPES_SSE4_1_REGISTER_H_

#include "turbo/simd/types/ssse3_register.h"

#if TURBO_WITH_SSE4_1
#include <smmintrin.h>
#endif

namespace turbo::simd
{
    /**
     * @ingroup architectures
     *
     * SSE4.1 instructions
     */
    struct sse4_1 : ssse3
    {
        static constexpr bool supported() noexcept { return TURBO_WITH_SSE4_1; }
        static constexpr bool available() noexcept { return true; }
        static constexpr unsigned version() noexcept { return generic::version(1, 4, 1); }
        static constexpr char const* name() noexcept { return "sse4.1"; }
    };

#if TURBO_WITH_SSE4_1
    namespace types
    {
        TURBO_SIMD_DECLARE_SIMD_REGISTER_ALIAS(sse4_1, ssse3);
    }
#endif
}

#endif  // TURBO_SIMD_TYPES_SSE4_1_REGISTER_H_

