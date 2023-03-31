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

#ifndef SIMDUTF_COMMON_DEFS_H
#define SIMDUTF_COMMON_DEFS_H

#include <cassert>
#include "turbo/platform/port.h"
#include "turbo/unicode/simdutf/portability.h"


// Align to N-byte boundary
#define SIMDUTF_ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define SIMDUTF_ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define SIMDUTF_ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#if defined(SIMDUTF_REGULAR_VISUAL_STUDIO)


  #define SIMDUTF_PUSH_DISABLE_WARNINGS __pragma(warning( push ))
  #define SIMDUTF_PUSH_DISABLE_ALL_WARNINGS __pragma(warning( push, 0 ))
  #define SIMDUTF_DISABLE_VS_WARNING(WARNING_NUMBER) __pragma(warning( disable : WARNING_NUMBER ))
  // Get rid of Intellisense-only warnings (Code Analysis)
  // Though __has_include is C++17, it is supported in Visual Studio 2017 or better (_MSC_VER>=1910).
  #ifdef __has_include
  #if __has_include(<CppCoreCheck\Warnings.h>)
  #include <CppCoreCheck\Warnings.h>
  #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS SIMDUTF_DISABLE_VS_WARNING(ALL_CPPCORECHECK_WARNINGS)
  #endif
  #endif

  #ifndef SIMDUTF_DISABLE_UNDESIRED_WARNINGS
  #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS
  #endif

  #define SIMDUTF_DISABLE_DEPRECATED_WARNING SIMDUTF_DISABLE_VS_WARNING(4996)
  #define SIMDUTF_DISABLE_STRICT_OVERFLOW_WARNING
  #define SIMDUTF_POP_DISABLE_WARNINGS __pragma(warning( pop ))

#else // SIMDUTF_REGULAR_VISUAL_STUDIO

  #define SIMDUTF_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
  // gcc doesn't seem to disable all warnings with all and extra, add warnings here as necessary
  #define SIMDUTF_PUSH_DISABLE_ALL_WARNINGS SIMDUTF_PUSH_DISABLE_WARNINGS \
    SIMDUTF_DISABLE_GCC_WARNING(-Weffc++) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wall) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wconversion) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wextra) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wattributes) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wimplicit-fallthrough) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wnon-virtual-dtor) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wreturn-type) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wshadow) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-parameter) \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-variable)
  #define SIMDUTF_PRAGMA(P) _Pragma(#P)
  #define SIMDUTF_DISABLE_GCC_WARNING(WARNING) SIMDUTF_PRAGMA(GCC diagnostic ignored #WARNING)
  #if defined(SIMDUTF_CLANG_VISUAL_STUDIO)
  #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS SIMDUTF_DISABLE_GCC_WARNING(-Wmicrosoft-include)
  #else
  #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS
  #endif
  #define SIMDUTF_DISABLE_DEPRECATED_WARNING SIMDUTF_DISABLE_GCC_WARNING(-Wdeprecated-declarations)
  #define SIMDUTF_DISABLE_STRICT_OVERFLOW_WARNING SIMDUTF_DISABLE_GCC_WARNING(-Wstrict-overflow)
  #define SIMDUTF_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")



#endif // MSC_VER


/// If EXPR is an error, returns it.
#define SIMDUTF_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }


#endif // SIMDUTF_COMMON_DEFS_H
