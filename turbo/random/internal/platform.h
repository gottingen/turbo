// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef TURBO_RANDOM_INTERNAL_PLATFORM_H_
#define TURBO_RANDOM_INTERNAL_PLATFORM_H_

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

// -----------------------------------------------------------------------------
// Platform Feature Checks
// -----------------------------------------------------------------------------

// Currently supported operating systems and associated preprocessor
// symbols:
//
//   Linux and Linux-derived           __linux__
//   Android                           __ANDROID__ (implies __linux__)
//   Linux (non-Android)               __linux__ && !__ANDROID__
//   Darwin (macOS and iOS)            __APPLE__
//   Akaros (http://akaros.org)        __ros__
//   Windows                           _WIN32
//   NaCL                              __native_client__
//   AsmJS                             __asmjs__
//   WebAssembly                       __wasm__
//   Fuchsia                           __Fuchsia__
//
// Note that since Android defines both __ANDROID__ and __linux__, one
// may probe for either Linux or Android by simply testing for __linux__.
//
// NOTE: For __APPLE__ platforms, we use #include <TargetConditionals.h>
// to distinguish os variants.
//
// http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// -----------------------------------------------------------------------------
// Architecture Checks
// -----------------------------------------------------------------------------

// These preprocessor directives are trying to determine CPU architecture,
// including necessary headers to support hardware AES.
//
// TURBO_ARCH_{X86/PPC/ARM} macros determine the platform.
#if defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64) || \
    defined(_M_X64)
#define TURBO_ARCH_X86_64
#elif defined(__i386) || defined(_M_IX86)
#define TURBO_ARCH_X86_32
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define TURBO_ARCH_AARCH64
#elif defined(__arm__) || defined(__ARMEL__) || defined(_M_ARM)
#define TURBO_ARCH_ARM
#elif defined(__powerpc64__) || defined(__PPC64__) || defined(__powerpc__) || \
    defined(__ppc__) || defined(__PPC__)
#define TURBO_ARCH_PPC
#else
// Unsupported architecture.
//  * https://sourceforge.net/p/predef/wiki/Architectures/
//  * https://msdn.microsoft.com/en-us/library/b0084kay.aspx
//  * for gcc, clang: "echo | gcc -E -dM -"
#endif

// -----------------------------------------------------------------------------
// Attribute Checks
// -----------------------------------------------------------------------------

// TURBO_RANDOM_INTERNAL_RESTRICT annotates whether pointers may be considered
// to be unaliased.
#if defined(__clang__) || defined(__GNUC__)
#define TURBO_RANDOM_INTERNAL_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define TURBO_RANDOM_INTERNAL_RESTRICT __restrict
#else
#define TURBO_RANDOM_INTERNAL_RESTRICT
#endif

// TURBO_HAVE_ACCELERATED_AES indicates whether the currently active compiler
// flags (e.g. -maes) allow using hardware accelerated AES instructions, which
// implies us assuming that the target platform supports them.
#define TURBO_HAVE_ACCELERATED_AES 0

#if defined(TURBO_ARCH_X86_64)

#if defined(__AES__) || defined(__AVX__)
#undef TURBO_HAVE_ACCELERATED_AES
#define TURBO_HAVE_ACCELERATED_AES 1
#endif

#elif defined(TURBO_ARCH_PPC)

// Rely on VSX and CRYPTO extensions for vcipher on PowerPC.
#if (defined(__VEC__) || defined(__ALTIVEC__)) && defined(__VSX__) && \
    defined(__CRYPTO__)
#undef TURBO_HAVE_ACCELERATED_AES
#define TURBO_HAVE_ACCELERATED_AES 1
#endif

#elif defined(TURBO_ARCH_ARM) || defined(TURBO_ARCH_AARCH64)

// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0053c/IHI0053C_acle_2_0.pdf
// Rely on NEON+CRYPTO extensions for ARM.
#if defined(__ARM_NEON) && defined(__ARM_FEATURE_CRYPTO)
#undef TURBO_HAVE_ACCELERATED_AES
#define TURBO_HAVE_ACCELERATED_AES 1
#endif

#endif

// NaCl does not allow AES.
#if defined(__native_client__)
#undef TURBO_HAVE_ACCELERATED_AES
#define TURBO_HAVE_ACCELERATED_AES 0
#endif

// TURBO_RANDOM_INTERNAL_AES_DISPATCH indicates whether the currently active
// platform has, or should use run-time dispatch for selecting the
// accelerated Randen implementation.
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 0

#if defined(TURBO_ARCH_X86_64)
// Dispatch is available on x86_64
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 1
#elif defined(__linux__) && defined(TURBO_ARCH_PPC)
// Or when running linux PPC
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 1
#elif defined(__linux__) && defined(TURBO_ARCH_AARCH64)
// Or when running linux AArch64
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 1
#elif defined(__linux__) && defined(TURBO_ARCH_ARM) && (__ARM_ARCH >= 8)
// Or when running linux ARM v8 or higher.
// (This captures a lot of Android configurations.)
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 1
#endif

// NaCl does not allow dispatch.
#if defined(__native_client__)
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 0
#endif

// iOS does not support dispatch, even on x86, since applications
// should be bundled as fat binaries, with a different build tailored for
// each specific supported platform/architecture.
#if (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || \
    (defined(TARGET_OS_IPHONE_SIMULATOR) && TARGET_OS_IPHONE_SIMULATOR)
#undef TURBO_RANDOM_INTERNAL_AES_DISPATCH
#define TURBO_RANDOM_INTERNAL_AES_DISPATCH 0
#endif

#endif  // TURBO_RANDOM_INTERNAL_PLATFORM_H_
