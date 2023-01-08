
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_BASE_PROFILE_ATTRIBUTE_H_
#define FLARE_BASE_PROFILE_ATTRIBUTE_H_

#include "flare/base/profile/compiler.h"

// Annotate a function indicating it should not be inlined.
// Use like:
//   NOINLINE void DoStuff() { ... }
#ifndef FLARE_NO_INLINE
#if defined(FLARE_COMPILER_GNUC)
#define FLARE_NO_INLINE __attribute__((noinline))
#elif defined(FLARE_COMPILER_MSVC)
#define FLARE_NO_INLINE __declspec(noinline)
#else
#define FLARE_NO_INLINE
#endif
#endif  // FLARE_NO_INLINE

#ifndef FLARE_FORCE_INLINE
#if defined(FLARE_COMPILER_MSVC)
#define FLARE_FORCE_INLINE    __forceinline
#else
#define FLARE_FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif  // FLARE_FORCE_INLINE

#ifndef FLARE_ALLOW_UNUSED
#if defined(FLARE_COMPILER_GNUC)
#define FLARE_ALLOW_UNUSED __attribute__((unused))
#else
#define FLARE_ALLOW_UNUSED
#endif
#endif  // FLARE_ALLOW_UNUSED

#ifndef FLARE_HAVE_ATTRIBUTE
#ifdef __has_attribute
#define FLARE_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define FLARE_HAVE_ATTRIBUTE(x) 0
#endif
#endif  // FLARE_HAVE_ATTRIBUTE

#ifndef FLARE_MUST_USE_RESULT
#if FLARE_HAVE_ATTRIBUTE(nodiscard)
#define FLARE_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && FLARE_HAVE_ATTRIBUTE(warn_unused_result)
#define FLARE_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define FLARE_MUST_USE_RESULT
#endif
#endif  // FLARE_MUST_USE_RESULT

#define FLARE_ARRAY_SIZE(array) \
  (sizeof(::flare::base::base_internal::ArraySizeHelper(array)))

namespace flare::base::base_internal {
    template<typename T, size_t N>
    auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace flare::base::base_internal

// FLARE_DEPRECATED void dont_call_me_anymore(int arg);
// ...
// warning: 'void dont_call_me_anymore(int)' is deprecated
#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
# define FLARE_DEPRECATED __attribute__((deprecated))
#elif defined(FLARE_COMPILER_MSVC)
# define FLARE_DEPRECATED __declspec(deprecated)
#else
# define FLARE_DEPRECATED
#endif

// Mark function as weak. This is GCC only feature.
#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
# define FLARE_WEAK __attribute__((weak))
#else
# define FLARE_WEAK
#endif

#if (defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)) && __cplusplus >= 201103
#define FLARE_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define FLARE_WARN_UNUSED_RESULT
#endif

#ifdef _MSC_VER
# define FLARE_CACHELINE_ALIGNMENT __declspec(align(FLARE_CACHE_LINE_SIZE))
#endif /* _MSC_VER */

#ifdef __GNUC__
# define FLARE_CACHELINE_ALIGNMENT __attribute__((aligned(FLARE_CACHE_LINE_SIZE)))
#endif /* __GNUC__ */

#ifndef FLARE_CACHELINE_ALIGNMENT
# define FLARE_CACHELINE_ALIGNMENT /*FLARE_CACHELINE_ALIGNMENT*/
#endif


#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
#  if defined(__cplusplus)
#    define FLARE_LIKELY(expr) (__builtin_expect(!!(expr), true))
#    define FLARE_UNLIKELY(expr) (__builtin_expect(!!(expr), false))
#  else
#    define FLARE_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#    define FLARE_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#  endif
#else
#  define FLARE_LIKELY(expr) (expr)
#  define FLARE_UNLIKELY(expr) (expr)
#endif


// Specify memory alignment for structs, classes, etc.
// Use like:
//   class FLARE_ALIGN_AS(16) MyClass { ... }
//   FLARE_ALIGN_AS(16) int array[4];
#if defined(FLARE_COMPILER_MSVC)
#define FLARE_ALIGN_AS(byte_alignment) __declspec(align(byte_alignment))
#elif defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
#define FLARE_ALIGN_AS(byte_alignment) __attribute__((aligned(byte_alignment)))
#endif

// Return the byte alignment of the given type (available at compile time).  Use
// sizeof(type) prior to checking __alignof to workaround Visual C++ bug:
// http://goo.gl/isH0C
// Use like:
//   FLARE_ALIGN_OF(int32_t)  // this would be 4
#if defined(FLARE_COMPILER_MSVC)
#define FLARE_ALIGN_OF(type) (sizeof(type) - sizeof(type) + __alignof(type))
#elif defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
#define FLARE_ALIGN_OF(type) __alignof__(type)
#endif


// FLARE_NO_SANITIZE_MEMORY
//
// Tells the  MemorySanitizer to relax the handling of a given function. All
// "Use of uninitialized value" warnings from such functions will be suppressed,
// and all values loaded from memory will be considered fully initialized.
// This attribute is similar to the ADDRESS_SANITIZER attribute above, but deals
// with initialized-ness rather than addressability issues.
// NOTE: MemorySanitizer(msan) is supported by Clang but not GCC.
#if defined(__clang__)
#define FLARE_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define FLARE_NO_SANITIZE_MEMORY
#endif


// FLARE_NO_SANITIZE_THREAD
//
// Tells the ThreadSanitizer to not instrument a given function.
// NOTE: GCC supports ThreadSanitizer(tsan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define FLARE_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define FLARE_NO_SANITIZE_THREAD
#endif


// FLARE_NO_SANITIZE_ADDRESS
//
// Tells the AddressSanitizer (or other memory testing tools) to ignore a given
// function. Useful for cases when a function reads random locations on stack,
// calls _exit from a cloned subprocess, deliberately accesses buffer
// out of bounds or does other scary things with memory.
// NOTE: GCC supports AddressSanitizer(asan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define FLARE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define FLARE_NO_SANITIZE_ADDRESS
#endif

#if FLARE_COMPILER_HAS_FEATURE(thread_sanitizer) || __SANITIZE_THREAD__
#define FLARE_SANITIZE_THREAD 1
#endif

#if defined(FLARE_SANITIZE_THREAD)
#define FLARE_IFDEF_THREAD_SANITIZER(X) X
#else
#define FLARE_IFDEF_THREAD_SANITIZER(X)
#endif


// FLARE_HOT, FLARE_COLD
//
// Tells GCC that a function is hot or cold. GCC can use this information to
// improve static analysis, i.e. a conditional branch to a cold function
// is likely to be not-taken.
// This annotation is used for function declarations.
//
// Example:
//
//   int foo() FLARE_HOT;
#if FLARE_COMPILER_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
#define FLARE_HOT __attribute__((hot))
#else
#define FLARE_HOT
#endif

#if FLARE_COMPILER_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
#define FLARE_COLD __attribute__((cold))
#else
#define FLARE_COLD
#endif



// ------------------------------------------------------------------------
// FLARE_UNUSED
//
// Makes compiler warnings about unused variables go away.
//
// Example usage:
//    void Function(int x)
//    {
//        int y;
//        FLARE_UNUSED(x);
//        FLARE_UNUSED(y);
//    }
//
#ifndef FLARE_UNUSED
// The EDG solution below is pretty weak and needs to be augmented or replaced.
// It can't handle the C language, is limited to places where template declarations
// can be used, and requires the type x to be usable as a functions reference argument.
#if defined(__cplusplus) && defined(__EDG__)
namespace flare:: base_internal {
template <typename T>
inline void flare_macro_unused(T const volatile & x) { (void)x; }
}
#define FLARE_UNUSED(x) flare:: base_internal::flare_macro_unused(x)
#else
#define FLARE_UNUSED(x) (void)x
#endif
#endif

// ------------------------------------------------------------------------
// FLARE_HIDDEN
//

#ifndef FLARE_HIDDEN
    #if  defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
        #define FLARE_HIDDEN __attribute__((visibility("hidden")))
    #else
        #define FLARE_HIDDEN
    #endif
#endif  // FLARE_HIDDEN

#endif // FLARE_BASE_PROFILE_ATTRIBUTE_H_
