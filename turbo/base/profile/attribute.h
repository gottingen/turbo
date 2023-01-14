
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_BASE_PROFILE_ATTRIBUTE_H_
#define TURBO_BASE_PROFILE_ATTRIBUTE_H_

#include "turbo/base/profile/compiler.h"

// Annotate a function indicating it should not be inlined.
// Use like:
//   NOINLINE void DoStuff() { ... }
#ifndef TURBO_NO_INLINE
#if defined(TURBO_COMPILER_GNUC)
#define TURBO_NO_INLINE __attribute__((noinline))
#elif defined(TURBO_COMPILER_MSVC)
#define TURBO_NO_INLINE __declspec(noinline)
#else
#define TURBO_NO_INLINE
#endif
#endif  // TURBO_NO_INLINE

#ifndef TURBO_FORCE_INLINE
#if defined(TURBO_COMPILER_MSVC)
#define TURBO_FORCE_INLINE    __forceinline
#else
#define TURBO_FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif  // TURBO_FORCE_INLINE

#ifndef TURBO_ALLOW_UNUSED
#if defined(TURBO_COMPILER_GNUC)
#define TURBO_ALLOW_UNUSED __attribute__((unused))
#else
#define TURBO_ALLOW_UNUSED
#endif
#endif  // TURBO_ALLOW_UNUSED

#ifndef TURBO_HAVE_ATTRIBUTE
#ifdef __has_attribute
#define TURBO_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define TURBO_HAVE_ATTRIBUTE(x) 0
#endif
#endif  // TURBO_HAVE_ATTRIBUTE

#ifndef TURBO_MUST_USE_RESULT
#if TURBO_HAVE_ATTRIBUTE(nodiscard)
#define TURBO_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && TURBO_HAVE_ATTRIBUTE(warn_unused_result)
#define TURBO_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define TURBO_MUST_USE_RESULT
#endif
#endif  // TURBO_MUST_USE_RESULT

#define TURBO_ARRAY_SIZE(array) \
  (sizeof(::turbo::base::base_internal::ArraySizeHelper(array)))

namespace turbo::base::base_internal {
    template<typename T, size_t N>
    auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace turbo::base::base_internal

// TURBO_DEPRECATED void dont_call_me_anymore(int arg);
// ...
// warning: 'void dont_call_me_anymore(int)' is deprecated
#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
# define TURBO_DEPRECATED __attribute__((deprecated))
#elif defined(TURBO_COMPILER_MSVC)
# define TURBO_DEPRECATED __declspec(deprecated)
#else
# define TURBO_DEPRECATED
#endif

// Mark function as weak. This is GCC only feature.
#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
# define TURBO_WEAK __attribute__((weak))
#else
# define TURBO_WEAK
#endif

#if (defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)) && __cplusplus >= 201103
#define TURBO_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define TURBO_WARN_UNUSED_RESULT
#endif

#ifdef _MSC_VER
# define TURBO_CACHELINE_ALIGNMENT __declspec(align(TURBO_CACHE_LINE_SIZE))
#endif /* _MSC_VER */

#ifdef __GNUC__
# define TURBO_CACHELINE_ALIGNMENT __attribute__((aligned(TURBO_CACHE_LINE_SIZE)))
#endif /* __GNUC__ */

#ifndef TURBO_CACHELINE_ALIGNMENT
# define TURBO_CACHELINE_ALIGNMENT /*TURBO_CACHELINE_ALIGNMENT*/
#endif


#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
#  if defined(__cplusplus)
#    define TURBO_LIKELY(expr) (__builtin_expect(!!(expr), true))
#    define TURBO_UNLIKELY(expr) (__builtin_expect(!!(expr), false))
#  else
#    define TURBO_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#    define TURBO_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#  endif
#else
#  define TURBO_LIKELY(expr) (expr)
#  define TURBO_UNLIKELY(expr) (expr)
#endif


// Specify memory alignment for structs, classes, etc.
// Use like:
//   class TURBO_ALIGN_AS(16) MyClass { ... }
//   TURBO_ALIGN_AS(16) int array[4];
#if defined(TURBO_COMPILER_MSVC)
#define TURBO_ALIGN_AS(byte_alignment) __declspec(align(byte_alignment))
#elif defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
#define TURBO_ALIGN_AS(byte_alignment) __attribute__((aligned(byte_alignment)))
#endif

// Return the byte alignment of the given type (available at compile time).  Use
// sizeof(type) prior to checking __alignof to workaround Visual C++ bug:
// http://goo.gl/isH0C
// Use like:
//   TURBO_ALIGN_OF(int32_t)  // this would be 4
#if defined(TURBO_COMPILER_MSVC)
#define TURBO_ALIGN_OF(type) (sizeof(type) - sizeof(type) + __alignof(type))
#elif defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
#define TURBO_ALIGN_OF(type) __alignof__(type)
#endif


// TURBO_NO_SANITIZE_MEMORY
//
// Tells the  MemorySanitizer to relax the handling of a given function. All
// "Use of uninitialized value" warnings from such functions will be suppressed,
// and all values loaded from memory will be considered fully initialized.
// This attribute is similar to the ADDRESS_SANITIZER attribute above, but deals
// with initialized-ness rather than addressability issues.
// NOTE: MemorySanitizer(msan) is supported by Clang but not GCC.
#if defined(__clang__)
#define TURBO_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define TURBO_NO_SANITIZE_MEMORY
#endif


// TURBO_NO_SANITIZE_THREAD
//
// Tells the ThreadSanitizer to not instrument a given function.
// NOTE: GCC supports ThreadSanitizer(tsan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define TURBO_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define TURBO_NO_SANITIZE_THREAD
#endif


// TURBO_NO_SANITIZE_ADDRESS
//
// Tells the AddressSanitizer (or other memory testing tools) to ignore a given
// function. Useful for cases when a function reads random locations on stack,
// calls _exit from a cloned subprocess, deliberately accesses buffer
// out of bounds or does other scary things with memory.
// NOTE: GCC supports AddressSanitizer(asan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define TURBO_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define TURBO_NO_SANITIZE_ADDRESS
#endif

#if TURBO_COMPILER_HAS_FEATURE(thread_sanitizer) || __SANITIZE_THREAD__
#define TURBO_SANITIZE_THREAD 1
#endif

#if defined(TURBO_SANITIZE_THREAD)
#define TURBO_IFDEF_THREAD_SANITIZER(X) X
#else
#define TURBO_IFDEF_THREAD_SANITIZER(X)
#endif


// TURBO_HOT, TURBO_COLD
//
// Tells GCC that a function is hot or cold. GCC can use this information to
// improve static analysis, i.e. a conditional branch to a cold function
// is likely to be not-taken.
// This annotation is used for function declarations.
//
// Example:
//
//   int foo() TURBO_HOT;
#if TURBO_COMPILER_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
#define TURBO_HOT __attribute__((hot))
#else
#define TURBO_HOT
#endif

#if TURBO_COMPILER_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
#define TURBO_COLD __attribute__((cold))
#else
#define TURBO_COLD
#endif



// ------------------------------------------------------------------------
// TURBO_UNUSED
//
// Makes compiler warnings about unused variables go away.
//
// Example usage:
//    void Function(int x)
//    {
//        int y;
//        TURBO_UNUSED(x);
//        TURBO_UNUSED(y);
//    }
//
#ifndef TURBO_UNUSED
// The EDG solution below is pretty weak and needs to be augmented or replaced.
// It can't handle the C language, is limited to places where template declarations
// can be used, and requires the type x to be usable as a functions reference argument.
#if defined(__cplusplus) && defined(__EDG__)
namespace turbo:: base_internal {
template <typename T>
inline void turbo_macro_unused(T const volatile & x) { (void)x; }
}
#define TURBO_UNUSED(x) turbo:: base_internal::turbo_macro_unused(x)
#else
#define TURBO_UNUSED(x) (void)x
#endif
#endif

// ------------------------------------------------------------------------
// TURBO_HIDDEN
//

#ifndef TURBO_HIDDEN
#if  defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
#define TURBO_HIDDEN __attribute__((visibility("hidden")))
#else
#define TURBO_HIDDEN
#endif
#endif  // TURBO_HIDDEN

#ifndef TURBO_INLINE_VISIBILITY
#define TURBO_INLINE_VISIBILITY TURBO_HIDDEN TURBO_FORCE_INLINE
#endif


#endif // TURBO_BASE_PROFILE_ATTRIBUTE_H_
