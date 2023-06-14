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


#ifndef TURBO_SIMD_ARCH_ISA_H_
#define TURBO_SIMD_ARCH_ISA_H_

#include "turbo/simd/config/arch.h"

#include "turbo/simd/arch/generic_fwd.h"

#if TURBO_WITH_SSE2
#include "turbo/simd/arch/sse2.h"
#endif

#if TURBO_WITH_SSE3
#include "turbo/simd/arch/sse3.h"
#endif

#if TURBO_WITH_SSSE3
#include "turbo/simd/arch/ssse3.h"
#endif

#if TURBO_WITH_SSE4_1
#include "turbo/simd/arch/sse4_1.h"
#endif

#if TURBO_WITH_SSE4_2
#include "turbo/simd/arch/sse4_2.h"
#endif

#if TURBO_WITH_FMA3_SSE
#include "turbo/simd/arch/fma3_sse.h"
#endif

#if TURBO_WITH_FMA4
#include "turbo/simd/arch/fma4.h"
#endif

#if TURBO_WITH_AVX
#include "turbo/simd/arch/avx.h"
#endif

#if TURBO_WITH_FMA3_AVX
#include "turbo/simd/arch/fma3_avx.h"
#endif

#if TURBO_WITH_AVX2
#include "turbo/simd/arch/avx2.h"
#endif

#if TURBO_WITH_FMA3_AVX2
#include "turbo/simd/arch/fma3_avx2.h"
#endif

#if TURBO_WITH_AVX512F
#include "turbo/simd/arch/avx512f.h"
#endif

#if TURBO_WITH_AVX512BW
#include "turbo/simd/arch/avx512bw.h"
#endif

#if TURBO_WITH_NEON
#include "turbo/simd/arch/neon.h"
#endif

#if TURBO_WITH_NEON64
#include "turbo/simd/arch/neon64.h"
#endif

#if TURBO_WITH_SVE
#include "turbo/simd/arch/sve.h"
#endif

// Must come last to have access to all conversion specializations.
#include "turbo/simd/arch/generic.h"

#endif  // TURBO_SIMD_ARCH_ISA_H_

