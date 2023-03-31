// Copyright 2023 The Turbo Authors.
//
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

#ifndef TURBO_PLATFORM_CONFIG_CONFIG_AVX512_H_
#define TURBO_PLATFORM_CONFIG_CONFIG_AVX512_H_

/*
    It's possible to override AVX512 settings with cmake DCMAKE_CXX_FLAGS.

    All preprocessor directives has form `TURBO_HAVE_AVX512{feature}`,
    where a feature is a code name for extensions.

    Please see the listing below to find which are supported.
*/

#ifndef TURBO_HAVE_AVX512F
#if defined(__AVX512F__) && __AVX512F__ == 1
    #define TURBO_HAVE_AVX512F 1
#else
    #define TURBO_HAVE_AVX512F 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512DQ
#if defined(__AVX512DQ__) && __AVX512DQ__ == 1
    #define TURBO_HAVE_AVX512DQ 1
#else
    #define TURBO_HAVE_AVX512DQ 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512IFMA
# if defined(__AVX512IFMA__) && __AVX512IFMA__ == 1
#   define TURBO_HAVE_AVX512IFMA 1
# endif
#endif

#ifndef TURBO_HAVE_AVX512CD
#if defined(__AVX512CD__) && __AVX512CD__ == 1
#define TURBO_HAVE_AVX512CD 1
#else
#define TURBO_HAVE_AVX512CD 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512BW
#if defined(__AVX512BW__) && __AVX512BW__ == 1
#define TURBO_HAVE_AVX512BW 1
#else
#define TURBO_HAVE_AVX512BW 0
# endif
#endif

#ifndef TURBO_HAVE_AVX512VL
#if defined(__AVX512VL__) && __AVX512VL__ == 1
#define TURBO_HAVE_AVX512VL 1
#else
#define TURBO_HAVE_AVX512VL 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512VBMI
# if defined(__AVX512VBMI__) && __AVX512VBMI__ == 1
#define TURBO_HAVE_AVX512VBMI 1
#else
#define TURBO_HAVE_AVX512VBMI 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512VBMI2
#if defined(__AVX512VBMI2__) && __AVX512VBMI2__ == 1
#define TURBO_HAVE_AVX512VBMI2 1
#else
#define TURBO_HAVE_AVX512VBMI2 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512VNNI
#if defined(__AVX512VNNI__) && __AVX512VNNI__ == 1
#define TURBO_HAVE_AVX512VNNI 1
#else
#define TURBO_HAVE_AVX512VNNI 0
#endif
#endif

#ifndef TURBO_HAVE_AVX512BITALG
# if defined(__AVX512BITALG__) && __AVX512BITALG__ == 1
#define TURBO_HAVE_AVX512BITALG 1
#else
#define TURBO_HAVE_AVX512BITALG 0
# endif
#endif

#ifndef TURBO_HAVE_AVX512VPOPCNTDQ
#if defined(__AVX512VPOPCNTDQ__) && __AVX512VPOPCNTDQ__ == 1
#define TURBO_HAVE_AVX512VPOPCNTDQ 1
#else
#define TURBO_HAVE_AVX512VPOPCNTDQ 0
#endif
#endif

#endif // TURBO_PLATFORM_CONFIG_CONFIG_AVX512_H_
