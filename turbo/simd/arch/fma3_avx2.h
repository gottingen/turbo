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

#ifndef TURBO_SIMD_ARCH_FMA3_AVX2_H_
#define TURBO_SIMD_ARCH_FMA3_AVX2_H_

#include "turbo/simd/types/fma3_avx2_register.h"

// Allow inclusion of turbo/simd/arch/fma3_avx.h
#ifdef TURBO_SIMD_ARCH_FMA3_AVX_H_
#undef TURBO_SIMD_ARCH_FMA3_AVX_H_
#define TURBO_SIMD_FORCE_ARCH_FMA3_AVX_H_
#endif

// Disallow inclusion of turbo/simd/types/fma3_avx_register.h
#ifndef TURBO_SIMD_TYPES_FMA3_AVX_REGISTER_H_
#define TURBO_SIMD_TYPES_FMA3_AVX_REGISTER_H_
#define TURBO_SIMD_FORCE_TYPES_FMA3_AVX_REGISTER_H_
#endif

// Include turbo/simd/arch/fma3_avx.h but s/avx/avx2
#define avx avx2
#include "turbo/simd/arch/fma3_avx.h"
#undef avx
#undef TURBO_SIMD_ARCH_FMA3_AVX_H_

// Carefully restore guards
#ifdef TURBO_SIMD_FORCE_ARCH_FMA3_AVX_H_
#define TURBO_SIMD_ARCH_FMA3_AVX_H_
#undef TURBO_SIMD_FORCE_ARCH_FMA3_AVX_H_
#endif

#ifdef TURBO_SIMD_FORCE_TYPES_FMA3_AVX_REGISTER_H_
#undef TURBO_SIMD_TYPES_FMA3_AVX_REGISTER_H_
#undef TURBO_SIMD_FORCE_TYPES_FMA3_AVX_REGISTER_H_
#endif

#endif  // TURBO_SIMD_ARCH_FMA3_AVX2_H_
