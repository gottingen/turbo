//
// Copyright 2020 The Turbo Authors.
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
//
// -----------------------------------------------------------------------------
// File: optimization.h
// -----------------------------------------------------------------------------
//
// This header file defines portable macros for performance optimization.

#ifndef TURBO_BASE_OPTIMIZATION_H_
#define TURBO_BASE_OPTIMIZATION_H_

#include <assert.h>

#include "turbo/platform/config/config.h"

// TURBO_BLOCK_TAIL_CALL_OPTIMIZATION
//
// Instructs the compiler to avoid optimizing tail-call recursion. This macro is
// useful when you wish to preserve the existing function order within a stack
// trace for logging, debugging, or profiling purposes.
//
// Example:
//
//   int f() {
//     int result = g();
//     TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
//     return result;
//   }
#if defined(__pnacl__)
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
// Clang will not tail call given inline volatile assembly.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
// GCC will not tail call given inline volatile assembly.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
#include <intrin.h>
// The __nop() intrinsic blocks the optimisation.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

// TURBO_CACHELINE_SIZE
//
// Explicitly defines the size of the L1 cache for purposes of alignment.
// Setting the cacheline size allows you to specify that certain objects be
// aligned on a cacheline boundary with `TURBO_CACHELINE_ALIGNED` declarations.
// (See below.)
//
// NOTE: this macro should be replaced with the following C++17 features, when
// those are generally available:
//
//   * `std::hardware_constructive_interference_size`
//   * `std::hardware_destructive_interference_size`
//
// See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html
// for more information.
#if defined(__GNUC__)
// Cache line alignment
#if defined(__i386__) || defined(__x86_64__)
#define TURBO_CACHELINE_SIZE 64
#elif defined(__powerpc64__)
#define TURBO_CACHELINE_SIZE 128
#elif defined(__aarch64__)
// We would need to read special register ctr_el0 to find out L1 dcache size.
// This value is a good estimate based on a real aarch64 machine.
#define TURBO_CACHELINE_SIZE 64
#elif defined(__arm__)
// Cache line sizes for ARM: These values are not strictly correct since
// cache line sizes depend on implementations, not architectures.  There
// are even implementations with cache line sizes configurable at boot
// time.
#if defined(__ARM_ARCH_5T__)
#define TURBO_CACHELINE_SIZE 32
#elif defined(__ARM_ARCH_7A__)
#define TURBO_CACHELINE_SIZE 64
#endif
#endif
#endif

#ifndef TURBO_CACHELINE_SIZE
// A reasonable default guess.  Note that overestimates tend to waste more
// space, while underestimates tend to waste more time.
#define TURBO_CACHELINE_SIZE 64
#endif

// TURBO_CACHELINE_ALIGNED
//
// Indicates that the declared object be cache aligned using
// `TURBO_CACHELINE_SIZE` (see above). Cacheline aligning objects allows you to
// load a set of related objects in the L1 cache for performance improvements.
// Cacheline aligning objects properly allows constructive memory sharing and
// prevents destructive (or "false") memory sharing.
//
// NOTE: callers should replace uses of this macro with `alignas()` using
// `std::hardware_constructive_interference_size` and/or
// `std::hardware_destructive_interference_size` when C++17 becomes available to
// them.
//
// See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html
// for more information.
//
// On some compilers, `TURBO_CACHELINE_ALIGNED` expands to an `__attribute__`
// or `__declspec` attribute. For compilers where this is not known to work,
// the macro expands to nothing.
//
// No further guarantees are made here. The result of applying the macro
// to variables and types is always implementation-defined.
//
// WARNING: It is easy to use this attribute incorrectly, even to the point
// of causing bugs that are difficult to diagnose, crash, etc. It does not
// of itself guarantee that objects are aligned to a cache line.
//
// NOTE: Some compilers are picky about the locations of annotations such as
// this attribute, so prefer to put it at the beginning of your declaration.
// For example,
//
//   TURBO_CACHELINE_ALIGNED static Foo* foo = ...
//
//   class TURBO_CACHELINE_ALIGNED Bar { ...
//
// Recommendations:
//
// 1) Consult compiler documentation; this comment is not kept in sync as
//    toolchains evolve.
// 2) Verify your use has the intended effect. This often requires inspecting
//    the generated machine code.
// 3) Prefer applying this attribute to individual variables. Avoid
//    applying it to types. This tends to localize the effect.
#if defined(__clang__) || defined(__GNUC__)
#define TURBO_CACHELINE_ALIGNED __attribute__((aligned(TURBO_CACHELINE_SIZE)))
#elif defined(_MSC_VER)
#define TURBO_CACHELINE_ALIGNED __declspec(align(TURBO_CACHELINE_SIZE))
#else
#define TURBO_CACHELINE_ALIGNED
#endif

// TURBO_PREDICT_TRUE, TURBO_PREDICT_FALSE
//
// Enables the compiler to prioritize compilation using static analysis for
// likely paths within a boolean branch.
//
// Example:
//
//   if (TURBO_PREDICT_TRUE(expression)) {
//     return result;                        // Faster if more likely
//   } else {
//     return 0;
//   }
//
// Compilers can use the information that a certain branch is not likely to be
// taken (for instance, a CHECK failure) to optimize for the common case in
// the absence of better information (ie. compiling gcc with `-fprofile-arcs`).
//
// Recommendation: Modern CPUs dynamically predict branch execution paths,
// typically with accuracy greater than 97%. As a result, annotating every
// branch in a codebase is likely counterproductive; however, annotating
// specific branches that are both hot and consistently mispredicted is likely
// to yield performance improvements.
#if TURBO_HAVE_BUILTIN(__builtin_expect) || \
    (defined(__GNUC__) && !defined(__clang__))
#define TURBO_PREDICT_FALSE(x) (__builtin_expect(false || (x), false))
#define TURBO_PREDICT_TRUE(x) (__builtin_expect(false || (x), true))
#else
#define TURBO_PREDICT_FALSE(x) (x)
#define TURBO_PREDICT_TRUE(x) (x)
#endif

// `TURBO_INTERNAL_IMMEDIATE_ABORT_IMPL()` aborts the program in the fastest
// possible way, with no attempt at logging. One use is to implement hardening
// aborts with TURBO_OPTION_HARDENED.  Since this is an internal symbol, it
// should not be used directly outside of Turbo.
#if TURBO_HAVE_BUILTIN(__builtin_trap) || \
    (defined(__GNUC__) && !defined(__clang__))
#define TURBO_INTERNAL_IMMEDIATE_ABORT_IMPL() __builtin_trap()
#else
#define TURBO_INTERNAL_IMMEDIATE_ABORT_IMPL() abort()
#endif

// `TURBO_INTERNAL_UNREACHABLE_IMPL()` is the platform specific directive to
// indicate that a statement is unreachable, and to allow the compiler to
// optimize accordingly. Clients should use `TURBO_UNREACHABLE()`, which is
// defined below.
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#define TURBO_INTERNAL_UNREACHABLE_IMPL() std::unreachable()
#elif defined(__GNUC__) || TURBO_HAVE_BUILTIN(__builtin_unreachable)
#define TURBO_INTERNAL_UNREACHABLE_IMPL() __builtin_unreachable()
#elif TURBO_HAVE_BUILTIN(__builtin_assume)
#define TURBO_INTERNAL_UNREACHABLE_IMPL() __builtin_assume(false)
#elif defined(_MSC_VER)
#define TURBO_INTERNAL_UNREACHABLE_IMPL() __assume(false)
#else
#define TURBO_INTERNAL_UNREACHABLE_IMPL()
#endif

// `TURBO_UNREACHABLE()` is an unreachable statement.  A program which reaches
// one has undefined behavior, and the compiler may optimize accordingly.
#if TURBO_OPTION_HARDENED == 1 && defined(NDEBUG)
// Abort in hardened mode to avoid dangerous undefined behavior.
#define TURBO_UNREACHABLE()                \
  do {                                    \
    TURBO_INTERNAL_IMMEDIATE_ABORT_IMPL(); \
    TURBO_INTERNAL_UNREACHABLE_IMPL();     \
  } while (false)
#else
// The assert only fires in debug mode to aid in debugging.
// When NDEBUG is defined, reaching TURBO_UNREACHABLE() is undefined behavior.
#define TURBO_UNREACHABLE()                       \
  do {                                           \
    /* NOLINTNEXTLINE: misc-static-assert */     \
    assert(false && "TURBO_UNREACHABLE reached"); \
    TURBO_INTERNAL_UNREACHABLE_IMPL();            \
  } while (false)
#endif

// TURBO_INTERNAL_UNIQUE_SMALL_NAME(cond)
// This macro forces small unique name on a static file level symbols like
// static local variables or static functions. This is intended to be used in
// macro definitions to optimize the cost of generated code. Do NOT use it on
// symbols exported from translation unit since it may cause a link time
// conflict.
//
// Example:
//
// #define MY_MACRO(txt)
// namespace {
//  char VeryVeryLongVarName[] TURBO_INTERNAL_UNIQUE_SMALL_NAME() = txt;
//  const char* VeryVeryLongFuncName() TURBO_INTERNAL_UNIQUE_SMALL_NAME();
//  const char* VeryVeryLongFuncName() { return txt; }
// }
//

#if defined(__GNUC__)
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME2(x) #x
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME1(x) TURBO_INTERNAL_UNIQUE_SMALL_NAME2(x)
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME() \
  asm(TURBO_INTERNAL_UNIQUE_SMALL_NAME1(.turbo.__COUNTER__))
#else
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME()
#endif

#endif  // TURBO_BASE_OPTIMIZATION_H_
