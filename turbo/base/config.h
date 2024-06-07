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
//
// -----------------------------------------------------------------------------
// File: config.h
// -----------------------------------------------------------------------------
//
// This header file defines a set of macros for checking the presence of
// important compiler and platform features. Such macros can be used to
// produce portable code by parameterizing compilation based on the presence or
// lack of a given feature.
//
// We define a "feature" as some interface we wish to program to: for example,
// a library function or system call. A value of `1` indicates support for
// that feature; any other value indicates the feature support is undefined.
//
// Example:
//
// Suppose a programmer wants to write a program that uses the 'mmap()' system
// call. The Turbo macro for that feature (`TURBO_HAVE_MMAP`) allows you to
// selectively include the `mmap.h` header and bracket code using that feature
// in the macro:
//
//   #include <turbo/base/config.h>
//
//   #ifdef TURBO_HAVE_MMAP
//   #include <sys/mman.h>
//   #endif  //TURBO_HAVE_MMAP
//
//   ...
//   #ifdef TURBO_HAVE_MMAP
//   void *ptr = mmap(...);
//   ...
//   #endif  // TURBO_HAVE_MMAP

#ifndef TURBO_BASE_CONFIG_H_
#define TURBO_BASE_CONFIG_H_

#include <turbo/base/macros/visibility.h>
// Included for the __GLIBC__ macro (or similar macros on other systems).
#include <limits.h>

#ifdef __cplusplus
// Included for __GLIBCXX__, _LIBCPP_VERSION
#include <cstddef>
#endif  // __cplusplus

// TURBO_INTERNAL_CPLUSPLUS_LANG
//
// MSVC does not set the value of __cplusplus correctly, but instead uses
// _MSVC_LANG as a stand-in.
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
//
// However, there are reports that MSVC even sets _MSVC_LANG incorrectly at
// times, for example:
// https://github.com/microsoft/vscode-cpptools/issues/1770
// https://reviews.llvm.org/D70996
//
// For this reason, this symbol is considered INTERNAL and code outside of
// Turbo must not use it.
#if defined(_MSVC_LANG)
#define TURBO_INTERNAL_CPLUSPLUS_LANG _MSVC_LANG
#elif defined(__cplusplus)
#define TURBO_INTERNAL_CPLUSPLUS_LANG __cplusplus
#endif

#if defined(TURBO_INTERNAL_CPLUSPLUS_LANG) && \
    TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L
// Include library feature test macros.
#include <version>
#endif

#if defined(__APPLE__)
// Included for TARGET_OS_IPHONE, __IPHONE_OS_VERSION_MIN_REQUIRED,
// __IPHONE_8_0.
#include <Availability.h>
#include <TargetConditionals.h>
#endif

#include <turbo/base/options.h>
#include <turbo/base/policy_checks.h>

// Turbo long-term support (LTS) releases will define
// `TURBO_LTS_RELEASE_VERSION` to the integer representing the date string of the
// LTS release version, and will define `TURBO_LTS_RELEASE_PATCH_LEVEL` to the
// integer representing the patch-level for that release.
//
// For example, for LTS release version "20300401.2", this would give us
// TURBO_LTS_RELEASE_VERSION == 20300401 && TURBO_LTS_RELEASE_PATCH_LEVEL == 2
//
// These symbols will not be defined in non-LTS code.
//
// Turbo recommends that clients live-at-head. Therefore, if you are using
// these symbols to assert a minimum version requirement, we recommend you do it
// as
//
// #if defined(TURBO_LTS_RELEASE_VERSION) && TURBO_LTS_RELEASE_VERSION < 20300401
// #error Project foo requires Turbo LTS version >= 20300401
// #endif
//
// The `defined(TURBO_LTS_RELEASE_VERSION)` part of the check excludes
// live-at-head clients from the minimum version assertion.
//
// See https://abseil.io/about/releases for more information on Turbo release
// management.
//
// LTS releases can be obtained from
// https://github.com/abseil/abseil-cpp/releases.
#undef TURBO_LTS_RELEASE_VERSION
#undef TURBO_LTS_RELEASE_PATCH_LEVEL

// Helper macro to convert a CPP variable to a string literal.
#define TURBO_INTERNAL_DO_TOKEN_STR(x) #x
#define TURBO_STRINGIFY(x) TURBO_INTERNAL_DO_TOKEN_STR(x)

// -----------------------------------------------------------------------------
// Turbo namespace annotations
// -----------------------------------------------------------------------------

// TURBO_NAMESPACE_BEGIN/TURBO_NAMESPACE_END
//
// An annotation placed at the beginning/end of each `namespace turbo` scope.
// This is used to inject an inline namespace.
//
// The proper way to write Turbo code in the `turbo` namespace is:
//
// namespace turbo {
// TURBO_NAMESPACE_BEGIN
//
// void Foo();  // turbo::Foo().
//
// TURBO_NAMESPACE_END
// }  // namespace turbo
//
// Users of Turbo should not use these macros, because users of Turbo should
// not write `namespace turbo {` in their own code for any reason.  (Turbo does
// not support forward declarations of its own types, nor does it support
// user-provided specialization of Turbo templates.  Code that violates these
// rules may be broken without warning.)
#if !defined(TURBO_OPTION_USE_INLINE_NAMESPACE) || \
    !defined(TURBO_OPTION_INLINE_NAMESPACE_NAME)
#error options.h is misconfigured.
#endif

// Check that TURBO_OPTION_INLINE_NAMESPACE_NAME is neither "head" nor ""
#if defined(__cplusplus) && TURBO_OPTION_USE_INLINE_NAMESPACE == 1

#define TURBO_INTERNAL_INLINE_NAMESPACE_STR \
  TURBO_STRINGIFY(TURBO_OPTION_INLINE_NAMESPACE_NAME)

static_assert(TURBO_INTERNAL_INLINE_NAMESPACE_STR[0] != '\0',
              "options.h misconfigured: TURBO_OPTION_INLINE_NAMESPACE_NAME must "
              "not be empty.");
static_assert(TURBO_INTERNAL_INLINE_NAMESPACE_STR[0] != 'h' ||
                  TURBO_INTERNAL_INLINE_NAMESPACE_STR[1] != 'e' ||
                  TURBO_INTERNAL_INLINE_NAMESPACE_STR[2] != 'a' ||
                  TURBO_INTERNAL_INLINE_NAMESPACE_STR[3] != 'd' ||
                  TURBO_INTERNAL_INLINE_NAMESPACE_STR[4] != '\0',
              "options.h misconfigured: TURBO_OPTION_INLINE_NAMESPACE_NAME must "
              "be changed to a new, unique identifier name.");

#endif

#if TURBO_OPTION_USE_INLINE_NAMESPACE == 0
#define TURBO_NAMESPACE_BEGIN
#define TURBO_NAMESPACE_END
#define TURBO_INTERNAL_C_SYMBOL(x) x
#elif TURBO_OPTION_USE_INLINE_NAMESPACE == 1
#define TURBO_NAMESPACE_BEGIN \
  inline namespace TURBO_OPTION_INLINE_NAMESPACE_NAME {
#define TURBO_NAMESPACE_END }
#define TURBO_INTERNAL_C_SYMBOL_HELPER_2(x, v) x##_##v
#define TURBO_INTERNAL_C_SYMBOL_HELPER_1(x, v) \
  TURBO_INTERNAL_C_SYMBOL_HELPER_2(x, v)
#define TURBO_INTERNAL_C_SYMBOL(x) \
  TURBO_INTERNAL_C_SYMBOL_HELPER_1(x, TURBO_OPTION_INLINE_NAMESPACE_NAME)
#else
#error options.h is misconfigured.
#endif

// -----------------------------------------------------------------------------
// Compiler Feature Checks
// -----------------------------------------------------------------------------

// TURBO_HAVE_BUILTIN()
//
// Checks whether the compiler supports a Clang Feature Checking Macro, and if
// so, checks whether it supports the provided builtin function "x" where x
// is one of the functions noted in
// https://clang.llvm.org/docs/LanguageExtensions.html
//
// Note: Use this macro to avoid an extra level of #ifdef __has_builtin check.
// http://releases.llvm.org/3.3/tools/clang/docs/LanguageExtensions.html
#ifdef __has_builtin
#define TURBO_HAVE_BUILTIN(x) __has_builtin(x)
#else
#define TURBO_HAVE_BUILTIN(x) 0
#endif

#ifdef __has_feature
#define TURBO_HAVE_FEATURE(f) __has_feature(f)
#else
#define TURBO_HAVE_FEATURE(f) 0
#endif

// Portable check for GCC minimum version:
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define TURBO_INTERNAL_HAVE_MIN_GNUC_VERSION(x, y) \
  (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#define TURBO_INTERNAL_HAVE_MIN_GNUC_VERSION(x, y) 0
#endif

#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#define TURBO_INTERNAL_HAVE_MIN_CLANG_VERSION(x, y) \
  (__clang_major__ > (x) || __clang_major__ == (x) && __clang_minor__ >= (y))
#else
#define TURBO_INTERNAL_HAVE_MIN_CLANG_VERSION(x, y) 0
#endif

// TURBO_HAVE_TLS is defined to 1 when __thread should be supported.
// We assume __thread is supported on Linux when compiled with Clang or
// compiled against libstdc++ with _GLIBCXX_HAVE_TLS defined.
#ifdef TURBO_HAVE_TLS
#error TURBO_HAVE_TLS cannot be directly set
#elif (defined(__linux__)) && (defined(__clang__) || defined(_GLIBCXX_HAVE_TLS))
#define TURBO_HAVE_TLS 1
#endif

// TURBO_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
//
// Checks whether `std::is_trivially_destructible<T>` is supported.
#ifdef TURBO_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
#error TURBO_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE cannot be directly set
#define TURBO_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE 1
#endif

// TURBO_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
//
// Checks whether `std::is_trivially_default_constructible<T>` and
// `std::is_trivially_copy_constructible<T>` are supported.
#ifdef TURBO_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
#error TURBO_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE cannot be directly set
#else
#define TURBO_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE 1
#endif

// TURBO_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
//
// Checks whether `std::is_trivially_copy_assignable<T>` is supported.
#ifdef TURBO_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
#error TURBO_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE cannot be directly set
#else
#define TURBO_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE 1
#endif

// TURBO_HAVE_STD_IS_TRIVIALLY_COPYABLE
//
// Checks whether `std::is_trivially_copyable<T>` is supported.
#ifdef TURBO_HAVE_STD_IS_TRIVIALLY_COPYABLE
#error TURBO_HAVE_STD_IS_TRIVIALLY_COPYABLE cannot be directly set
#define TURBO_HAVE_STD_IS_TRIVIALLY_COPYABLE 1
#endif

// TURBO_HAVE_THREAD_LOCAL
//
// Checks whether C++11's `thread_local` storage duration specifier is
// supported.
#ifdef TURBO_HAVE_THREAD_LOCAL
#error TURBO_HAVE_THREAD_LOCAL cannot be directly set
#elif defined(__APPLE__)
// Notes:
// * Xcode's clang did not support `thread_local` until version 8, and
//   even then not for all iOS < 9.0.
// * Xcode 9.3 started disallowing `thread_local` for 32-bit iOS simulator
//   targeting iOS 9.x.
// * Xcode 10 moves the deployment target check for iOS < 9.0 to link time
//   making TURBO_HAVE_FEATURE unreliable there.
//
#if TURBO_HAVE_FEATURE(cxx_thread_local) && \
    !(TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_9_0)
#define TURBO_HAVE_THREAD_LOCAL 1
#endif
#else  // !defined(__APPLE__)
#define TURBO_HAVE_THREAD_LOCAL 1
#endif

// There are platforms for which TLS should not be used even though the compiler
// makes it seem like it's supported (Android NDK < r12b for example).
// This is primarily because of linker problems and toolchain misconfiguration:
// Turbo does not intend to support this indefinitely. Currently, the newest
// toolchain that we intend to support that requires this behavior is the
// r11 NDK - allowing for a 5 year support window on that means this option
// is likely to be removed around June of 2021.
// TLS isn't supported until NDK r12b per
// https://developer.android.com/ndk/downloads/revision_history.html
// Since NDK r16, `__NDK_MAJOR__` and `__NDK_MINOR__` are defined in
// <android/ndk-version.h>. For NDK < r16, users should define these macros,
// e.g. `-D__NDK_MAJOR__=11 -D__NKD_MINOR__=0` for NDK r11.
#if defined(__ANDROID__) && defined(__clang__)
#if __has_include(<android/ndk-version.h>)
#include <android/ndk-version.h>
#endif  // __has_include(<android/ndk-version.h>)
#if defined(__ANDROID__) && defined(__clang__) && defined(__NDK_MAJOR__) && \
    defined(__NDK_MINOR__) &&                                               \
    ((__NDK_MAJOR__ < 12) || ((__NDK_MAJOR__ == 12) && (__NDK_MINOR__ < 1)))
#undef TURBO_HAVE_TLS
#undef TURBO_HAVE_THREAD_LOCAL
#endif
#endif  // defined(__ANDROID__) && defined(__clang__)

// TURBO_HAVE_INTRINSIC_INT128
//
// Checks whether the __int128 compiler extension for a 128-bit integral type is
// supported.
//
// Note: __SIZEOF_INT128__ is defined by Clang and GCC when __int128 is
// supported, but we avoid using it in certain cases:
// * On Clang:
//   * Building using Clang for Windows, where the Clang runtime library has
//     128-bit support only on LP64 architectures, but Windows is LLP64.
// * On Nvidia's nvcc:
//   * nvcc also defines __GNUC__ and __SIZEOF_INT128__, but not all versions
//     actually support __int128.
#ifdef TURBO_HAVE_INTRINSIC_INT128
#error TURBO_HAVE_INTRINSIC_INT128 cannot be directly set
#elif defined(__SIZEOF_INT128__)
#if (defined(__clang__) && !defined(_WIN32)) ||           \
    (defined(__CUDACC__) && __CUDACC_VER_MAJOR__ >= 9) || \
    (defined(__GNUC__) && !defined(__clang__) && !defined(__CUDACC__))
#define TURBO_HAVE_INTRINSIC_INT128 1
#elif defined(__CUDACC__)
// __CUDACC_VER__ is a full version number before CUDA 9, and is defined to a
// string explaining that it has been removed starting with CUDA 9. We use
// nested #ifs because there is no short-circuiting in the preprocessor.
// NOTE: `__CUDACC__` could be undefined while `__CUDACC_VER__` is defined.
#if __CUDACC_VER__ >= 70000
#define TURBO_HAVE_INTRINSIC_INT128 1
#endif  // __CUDACC_VER__ >= 70000
#endif  // defined(__CUDACC__)
#endif  // TURBO_HAVE_INTRINSIC_INT128

// TURBO_HAVE_EXCEPTIONS
//
// Checks whether the compiler both supports and enables exceptions. Many
// compilers support a "no exceptions" mode that disables exceptions.
//
// Generally, when TURBO_HAVE_EXCEPTIONS is not defined:
//
// * Code using `throw` and `try` may not compile.
// * The `noexcept` specifier will still compile and behave as normal.
// * The `noexcept` operator may still return `false`.
//
// For further details, consult the compiler's documentation.
#ifdef TURBO_HAVE_EXCEPTIONS
#error TURBO_HAVE_EXCEPTIONS cannot be directly set.
#elif TURBO_INTERNAL_HAVE_MIN_CLANG_VERSION(3, 6)
// Clang >= 3.6
#if TURBO_HAVE_FEATURE(cxx_exceptions)
#define TURBO_HAVE_EXCEPTIONS 1
#endif  // TURBO_HAVE_FEATURE(cxx_exceptions)
#elif defined(__clang__)
// Clang < 3.6
// http://releases.llvm.org/3.6.0/tools/clang/docs/ReleaseNotes.html#the-exceptions-macro
#if defined(__EXCEPTIONS) && TURBO_HAVE_FEATURE(cxx_exceptions)
#define TURBO_HAVE_EXCEPTIONS 1
#endif  // defined(__EXCEPTIONS) && TURBO_HAVE_FEATURE(cxx_exceptions)
// Handle remaining special cases and default to exceptions being supported.
#elif !(defined(__GNUC__) && !defined(__cpp_exceptions)) && \
    !(defined(_MSC_VER) && !defined(_CPPUNWIND))
#define TURBO_HAVE_EXCEPTIONS 1
#endif

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
//   WebAssembly (Emscripten)          __EMSCRIPTEN__
//   Fuchsia                           __Fuchsia__
//
// Note that since Android defines both __ANDROID__ and __linux__, one
// may probe for either Linux or Android by simply testing for __linux__.

// TURBO_HAVE_MMAP
//
// Checks whether the platform has an mmap(2) implementation as defined in
// POSIX.1-2001.
#ifdef TURBO_HAVE_MMAP
#error TURBO_HAVE_MMAP cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||    \
    defined(_AIX) || defined(__ros__) || defined(__native_client__) ||       \
    defined(__asmjs__) || defined(__EMSCRIPTEN__) || defined(__Fuchsia__) || \
    defined(__sun) || defined(__myriad2__) || defined(__HAIKU__) ||          \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__QNX__) ||       \
    defined(__VXWORKS__) || defined(__hexagon__)
#define TURBO_HAVE_MMAP 1
#endif

// TURBO_HAVE_PTHREAD_GETSCHEDPARAM
//
// Checks whether the platform implements the pthread_(get|set)schedparam(3)
// functions as defined in POSIX.1-2001.
#ifdef TURBO_HAVE_PTHREAD_GETSCHEDPARAM
#error TURBO_HAVE_PTHREAD_GETSCHEDPARAM cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(_AIX) || defined(__ros__) || defined(__OpenBSD__) ||          \
    defined(__NetBSD__) || defined(__VXWORKS__)
#define TURBO_HAVE_PTHREAD_GETSCHEDPARAM 1
#endif

// TURBO_HAVE_SCHED_GETCPU
//
// Checks whether sched_getcpu is available.
#ifdef TURBO_HAVE_SCHED_GETCPU
#error TURBO_HAVE_SCHED_GETCPU cannot be directly set
#elif defined(__linux__)
#define TURBO_HAVE_SCHED_GETCPU 1
#endif

// TURBO_HAVE_SCHED_YIELD
//
// Checks whether the platform implements sched_yield(2) as defined in
// POSIX.1-2001.
#ifdef TURBO_HAVE_SCHED_YIELD
#error TURBO_HAVE_SCHED_YIELD cannot be directly set
#elif defined(__linux__) || defined(__ros__) || defined(__native_client__) || \
    defined(__VXWORKS__)
#define TURBO_HAVE_SCHED_YIELD 1
#endif

// TURBO_HAVE_SEMAPHORE_H
//
// Checks whether the platform supports the <semaphore.h> header and sem_init(3)
// family of functions as standardized in POSIX.1-2001.
//
// Note: While Apple provides <semaphore.h> for both iOS and macOS, it is
// explicitly deprecated and will cause build failures if enabled for those
// platforms.  We side-step the issue by not defining it here for Apple
// platforms.
#ifdef TURBO_HAVE_SEMAPHORE_H
#error TURBO_HAVE_SEMAPHORE_H cannot be directly set
#elif defined(__linux__) || defined(__ros__) || defined(__VXWORKS__)
#define TURBO_HAVE_SEMAPHORE_H 1
#endif

// TURBO_HAVE_ALARM
//
// Checks whether the platform supports the <signal.h> header and alarm(2)
// function as standardized in POSIX.1-2001.
#ifdef TURBO_HAVE_ALARM
#error TURBO_HAVE_ALARM cannot be directly set
#elif defined(__GOOGLE_GRTE_VERSION__)
// feature tests for Google's GRTE
#define TURBO_HAVE_ALARM 1
#elif defined(__GLIBC__)
// feature test for glibc
#define TURBO_HAVE_ALARM 1
#elif defined(_MSC_VER)
// feature tests for Microsoft's library
#elif defined(__MINGW32__)
// mingw32 doesn't provide alarm(2):
// https://osdn.net/projects/mingw/scm/git/mingw-org-wsl/blobs/5.2-trunk/mingwrt/include/unistd.h
// mingw-w64 provides a no-op implementation:
// https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-crt/misc/alarm.c
#elif defined(__EMSCRIPTEN__)
// emscripten doesn't support signals
#elif defined(__wasi__)
// WASI doesn't support signals
#elif defined(__Fuchsia__)
// Signals don't exist on fuchsia.
#elif defined(__native_client__)
// Signals don't exist on hexagon/QuRT
#elif defined(__hexagon__)
#else
// other standard libraries
#define TURBO_HAVE_ALARM 1
#endif

// TURBO_IS_LITTLE_ENDIAN
// TURBO_IS_BIG_ENDIAN
//
// Checks the endianness of the platform.
//
// Notes: uses the built in endian macros provided by GCC (since 4.6) and
// Clang (since 3.2); see
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html.
// Otherwise, if _WIN32, assume little endian. Otherwise, bail with an error.
#if defined(TURBO_IS_BIG_ENDIAN)
#error "TURBO_IS_BIG_ENDIAN cannot be directly set."
#endif
#if defined(TURBO_IS_LITTLE_ENDIAN)
#error "TURBO_IS_LITTLE_ENDIAN cannot be directly set."
#endif

#if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define TURBO_IS_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define TURBO_IS_BIG_ENDIAN 1
#elif defined(_WIN32)
#define TURBO_IS_LITTLE_ENDIAN 1
#else
#error "turbo endian detection needs to be set up for your compiler"
#endif

// macOS < 10.13 and iOS < 12 don't support <any>, <optional>, or <variant>
// because the libc++ shared library shipped on the system doesn't have the
// requisite exported symbols.  See
// https://github.com/abseil/abseil-cpp/issues/207 and
// https://developer.apple.com/documentation/xcode_release_notes/xcode_10_release_notes
//
// libc++ spells out the availability requirements in the file
// llvm-project/libcxx/include/__config via the #define
// _LIBCPP_AVAILABILITY_BAD_OPTIONAL_ACCESS. The set of versions has been
// modified a few times, via
// https://github.com/llvm/llvm-project/commit/7fb40e1569dd66292b647f4501b85517e9247953
// and
// https://github.com/llvm/llvm-project/commit/0bc451e7e137c4ccadcd3377250874f641ca514a
// The second has the actually correct versions, thus, is what we copy here.
#if defined(__APPLE__) &&                                         \
    ((defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) &&   \
      __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101300) ||  \
     (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) &&  \
      __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ < 120000) || \
     (defined(__ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__) &&   \
      __ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__ < 50000) ||   \
     (defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__) &&      \
      __ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__ < 120000))
#define TURBO_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 1
#else
#define TURBO_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 0
#endif


// TURBO_HAVE_STD_ORDERING
//
// Checks whether C++20 std::{partial,weak,strong}_ordering are available.
//
// __cpp_lib_three_way_comparison is missing on libc++
// (https://github.com/llvm/llvm-project/issues/73953) so treat it as defined
// when building in C++20 mode.
#ifdef TURBO_HAVE_STD_ORDERING
#error "TURBO_HAVE_STD_ORDERING cannot be directly set."
#elif (defined(__cpp_lib_three_way_comparison) &&    \
       __cpp_lib_three_way_comparison >= 201907L) || \
    (defined(TURBO_INTERNAL_CPLUSPLUS_LANG) &&        \
     TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L)
#define TURBO_HAVE_STD_ORDERING 1
#endif

// TURBO_USES_STD_ORDERING
//
// Indicates whether turbo::{partial,weak,strong}_ordering are aliases for the
// std:: ordering types.
#if !defined(TURBO_OPTION_USE_STD_ORDERING)
#error options.h is misconfigured.
#elif TURBO_OPTION_USE_STD_ORDERING == 0 || \
    (TURBO_OPTION_USE_STD_ORDERING == 2 && !defined(TURBO_HAVE_STD_ORDERING))
#undef TURBO_USES_STD_ORDERING
#elif TURBO_OPTION_USE_STD_ORDERING == 1 || \
    (TURBO_OPTION_USE_STD_ORDERING == 2 && defined(TURBO_HAVE_STD_ORDERING))
#define TURBO_USES_STD_ORDERING 1
#else
#error options.h is misconfigured.
#endif

// In debug mode, MSVC 2017's std::variant throws a EXCEPTION_ACCESS_VIOLATION
// SEH exception from emplace for variant<SomeStruct> when constructing the
// struct can throw. This defeats some of variant_test and
// variant_exception_safety_test.
#if defined(_MSC_VER) && _MSC_VER >= 1700 && defined(_DEBUG)
#define TURBO_INTERNAL_MSVC_2017_DBG_MODE
#endif

// TURBO_INTERNAL_MANGLED_NS
// TURBO_INTERNAL_MANGLED_BACKREFERENCE
//
// Internal macros for building up mangled names in our internal fork of CCTZ.
// This implementation detail is only needed and provided for the MSVC build.
//
// These macros both expand to string literals.  TURBO_INTERNAL_MANGLED_NS is
// the mangled spelling of the `turbo` namespace, and
// TURBO_INTERNAL_MANGLED_BACKREFERENCE is a back-reference integer representing
// the proper count to skip past the CCTZ fork namespace names.  (This number
// is one larger when there is an inline namespace name to skip.)
#if defined(_MSC_VER)
#if TURBO_OPTION_USE_INLINE_NAMESPACE == 0
#define TURBO_INTERNAL_MANGLED_NS "turbo"
#define TURBO_INTERNAL_MANGLED_BACKREFERENCE "5"
#else
#define TURBO_INTERNAL_MANGLED_NS \
  TURBO_STRINGIFY(TURBO_OPTION_INLINE_NAMESPACE_NAME) "@turbo"
#define TURBO_INTERNAL_MANGLED_BACKREFERENCE "6"
#endif
#endif

#if defined(_MSC_VER)
#if defined(TURBO_BUILD_TEST_DLL)
#define TURBO_TEST_DLL __declspec(dllexport)
#elif defined(TURBO_CONSUME_TEST_DLL)
#define TURBO_TEST_DLL __declspec(dllimport)
#else
#define TURBO_TEST_DLL
#endif
#else
#define TURBO_TEST_DLL
#endif  // defined(_MSC_VER)

// TURBO_HAVE_MEMORY_SANITIZER
//
// MemorySanitizer (MSan) is a detector of uninitialized reads. It consists of
// a compiler instrumentation module and a run-time library.
#ifdef TURBO_HAVE_MEMORY_SANITIZER
#error "TURBO_HAVE_MEMORY_SANITIZER cannot be directly set."
#elif !defined(__native_client__) && TURBO_HAVE_FEATURE(memory_sanitizer)
#define TURBO_HAVE_MEMORY_SANITIZER 1
#endif

// TURBO_HAVE_THREAD_SANITIZER
//
// ThreadSanitizer (TSan) is a fast data race detector.
#ifdef TURBO_HAVE_THREAD_SANITIZER
#error "TURBO_HAVE_THREAD_SANITIZER cannot be directly set."
#elif defined(__SANITIZE_THREAD__)
#define TURBO_HAVE_THREAD_SANITIZER 1
#elif TURBO_HAVE_FEATURE(thread_sanitizer)
#define TURBO_HAVE_THREAD_SANITIZER 1
#endif

// TURBO_HAVE_ADDRESS_SANITIZER
//
// AddressSanitizer (ASan) is a fast memory error detector.
#ifdef TURBO_HAVE_ADDRESS_SANITIZER
#error "TURBO_HAVE_ADDRESS_SANITIZER cannot be directly set."
#elif defined(__SANITIZE_ADDRESS__)
#define TURBO_HAVE_ADDRESS_SANITIZER 1
#elif TURBO_HAVE_FEATURE(address_sanitizer)
#define TURBO_HAVE_ADDRESS_SANITIZER 1
#endif

// TURBO_HAVE_HWADDRESS_SANITIZER
//
// Hardware-Assisted AddressSanitizer (or HWASAN) is even faster than asan
// memory error detector which can use CPU features like ARM TBI, Intel LAM or
// AMD UAI.
#ifdef TURBO_HAVE_HWADDRESS_SANITIZER
#error "TURBO_HAVE_HWADDRESS_SANITIZER cannot be directly set."
#elif defined(__SANITIZE_HWADDRESS__)
#define TURBO_HAVE_HWADDRESS_SANITIZER 1
#elif TURBO_HAVE_FEATURE(hwaddress_sanitizer)
#define TURBO_HAVE_HWADDRESS_SANITIZER 1
#endif

// TURBO_HAVE_DATAFLOW_SANITIZER
//
// Dataflow Sanitizer (or DFSAN) is a generalised dynamic data flow analysis.
#ifdef TURBO_HAVE_DATAFLOW_SANITIZER
#error "TURBO_HAVE_DATAFLOW_SANITIZER cannot be directly set."
#elif defined(DATAFLOW_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// DataFlowSanitizer (-fsanitize=dataflow), so GCC users of -fsanitize=dataflow
// should also use -DDATAFLOW_SANITIZER.
#define TURBO_HAVE_DATAFLOW_SANITIZER 1
#elif TURBO_HAVE_FEATURE(dataflow_sanitizer)
#define TURBO_HAVE_DATAFLOW_SANITIZER 1
#endif

// TURBO_HAVE_LEAK_SANITIZER
//
// LeakSanitizer (or lsan) is a detector of memory leaks.
// https://clang.llvm.org/docs/LeakSanitizer.html
// https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer
//
// The macro TURBO_HAVE_LEAK_SANITIZER can be used to detect at compile-time
// whether the LeakSanitizer is potentially available. However, just because the
// LeakSanitizer is available does not mean it is active. Use the
// always-available run-time interface in //turbo/debugging/leak_check.h for
// interacting with LeakSanitizer.
#ifdef TURBO_HAVE_LEAK_SANITIZER
#error "TURBO_HAVE_LEAK_SANITIZER cannot be directly set."
#elif defined(LEAK_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// LeakSanitizer (-fsanitize=leak), so GCC users of -fsanitize=leak should also
// use -DLEAK_SANITIZER.
#define TURBO_HAVE_LEAK_SANITIZER 1
// Clang standalone LeakSanitizer (-fsanitize=leak)
#elif TURBO_HAVE_FEATURE(leak_sanitizer)
#define TURBO_HAVE_LEAK_SANITIZER 1
#elif defined(TURBO_HAVE_ADDRESS_SANITIZER)
// GCC or Clang using the LeakSanitizer integrated into AddressSanitizer.
#define TURBO_HAVE_LEAK_SANITIZER 1
#endif

// TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
//
// Class template argument deduction is a language feature added in C++17.
#ifdef TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
#error "TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION cannot be directly set."
#elif defined(__cpp_deduction_guides)
#define TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION 1
#endif

// TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
//
// Prior to C++17, static constexpr variables defined in classes required a
// separate definition outside of the class body, for example:
//
// class Foo {
//   static constexpr int kBar = 0;
// };
// constexpr int Foo::kBar;
//
// In C++17, these variables defined in classes are considered inline variables,
// and the extra declaration is redundant. Since some compilers warn on the
// extra declarations, TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL can be used
// conditionally ignore them:
//
// #ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
// constexpr int Foo::kBar;
// #endif
#if defined(TURBO_INTERNAL_CPLUSPLUS_LANG) && \
    TURBO_INTERNAL_CPLUSPLUS_LANG < 201703L
#define TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL 1
#endif

// `TURBO_INTERNAL_HAS_RTTI` determines whether abseil is being compiled with
// RTTI support.
#ifdef TURBO_INTERNAL_HAS_RTTI
#error TURBO_INTERNAL_HAS_RTTI cannot be directly set
#elif TURBO_HAVE_FEATURE(cxx_rtti)
#define TURBO_INTERNAL_HAS_RTTI 1
#elif defined(__GNUC__) && defined(__GXX_RTTI)
#define TURBO_INTERNAL_HAS_RTTI 1
#elif defined(_MSC_VER) && defined(_CPPRTTI)
#define TURBO_INTERNAL_HAS_RTTI 1
#elif !defined(__GNUC__) && !defined(_MSC_VER)
// Unknown compiler, default to RTTI
#define TURBO_INTERNAL_HAS_RTTI 1
#endif

// `TURBO_INTERNAL_HAS_CXA_DEMANGLE` determines whether `abi::__cxa_demangle` is
// available.
#ifdef TURBO_INTERNAL_HAS_CXA_DEMANGLE
#error TURBO_INTERNAL_HAS_CXA_DEMANGLE cannot be directly set
#elif defined(OS_ANDROID) && (defined(__i386__) || defined(__x86_64__))
#define TURBO_INTERNAL_HAS_CXA_DEMANGLE 0
#elif defined(__GNUC__)
#define TURBO_INTERNAL_HAS_CXA_DEMANGLE 1
#elif defined(__clang__) && !defined(_MSC_VER)
#define TURBO_INTERNAL_HAS_CXA_DEMANGLE 1
#endif

// TURBO_INTERNAL_HAVE_SSE is used for compile-time detection of SSE support.
// See https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html for an overview of
// which architectures support the various x86 instruction sets.
#ifdef TURBO_INTERNAL_HAVE_SSE
#error TURBO_INTERNAL_HAVE_SSE cannot be directly set
#elif defined(__SSE__)
#define TURBO_INTERNAL_HAVE_SSE 1
#elif (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)) && \
    !defined(_M_ARM64EC)
// MSVC only defines _M_IX86_FP for x86 32-bit code, and _M_IX86_FP >= 1
// indicates that at least SSE was targeted with the /arch:SSE option.
// All x86-64 processors support SSE, so support can be assumed.
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#define TURBO_INTERNAL_HAVE_SSE 1
#endif

// TURBO_INTERNAL_HAVE_SSE2 is used for compile-time detection of SSE2 support.
// See https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html for an overview of
// which architectures support the various x86 instruction sets.
#ifdef TURBO_INTERNAL_HAVE_SSE2
#error TURBO_INTERNAL_HAVE_SSE2 cannot be directly set
#elif defined(__SSE2__)
#define TURBO_INTERNAL_HAVE_SSE2 1
#elif (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)) && \
    !defined(_M_ARM64EC)
// MSVC only defines _M_IX86_FP for x86 32-bit code, and _M_IX86_FP >= 2
// indicates that at least SSE2 was targeted with the /arch:SSE2 option.
// All x86-64 processors support SSE2, so support can be assumed.
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#define TURBO_INTERNAL_HAVE_SSE2 1
#endif

// TURBO_INTERNAL_HAVE_SSSE3 is used for compile-time detection of SSSE3 support.
// See https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html for an overview of
// which architectures support the various x86 instruction sets.
//
// MSVC does not have a mode that targets SSSE3 at compile-time. To use SSSE3
// with MSVC requires either assuming that the code will only every run on CPUs
// that support SSSE3, otherwise __cpuid() can be used to detect support at
// runtime and fallback to a non-SSSE3 implementation when SSSE3 is unsupported
// by the CPU.
#ifdef TURBO_INTERNAL_HAVE_SSSE3
#error TURBO_INTERNAL_HAVE_SSSE3 cannot be directly set
#elif defined(__SSSE3__)
#define TURBO_INTERNAL_HAVE_SSSE3 1
#endif

// TURBO_INTERNAL_HAVE_ARM_NEON is used for compile-time detection of NEON (ARM
// SIMD).
//
// If __CUDA_ARCH__ is defined, then we are compiling CUDA code in device mode.
// In device mode, NEON intrinsics are not available, regardless of host
// platform.
// https://llvm.org/docs/CompileCudaWithLLVM.html#detecting-clang-vs-nvcc-from-code
#ifdef TURBO_INTERNAL_HAVE_ARM_NEON
#error TURBO_INTERNAL_HAVE_ARM_NEON cannot be directly set
#elif defined(__ARM_NEON) && !defined(__CUDA_ARCH__)
#define TURBO_INTERNAL_HAVE_ARM_NEON 1
#endif

// TURBO_HAVE_CONSTANT_EVALUATED is used for compile-time detection of
// constant evaluation support through `turbo::is_constant_evaluated`.
#ifdef TURBO_HAVE_CONSTANT_EVALUATED
#error TURBO_HAVE_CONSTANT_EVALUATED cannot be directly set
#endif
#ifdef __cpp_lib_is_constant_evaluated
#define TURBO_HAVE_CONSTANT_EVALUATED 1
#elif TURBO_HAVE_BUILTIN(__builtin_is_constant_evaluated)
#define TURBO_HAVE_CONSTANT_EVALUATED 1
#endif

// TURBO_INTERNAL_EMSCRIPTEN_VERSION combines Emscripten's three version macros
// into an integer that can be compared against.
#ifdef TURBO_INTERNAL_EMSCRIPTEN_VERSION
#error TURBO_INTERNAL_EMSCRIPTEN_VERSION cannot be directly set
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/version.h>
#ifdef __EMSCRIPTEN_major__
#if __EMSCRIPTEN_minor__ >= 1000
#error __EMSCRIPTEN_minor__ is too big to fit in TURBO_INTERNAL_EMSCRIPTEN_VERSION
#endif
#if __EMSCRIPTEN_tiny__ >= 1000
#error __EMSCRIPTEN_tiny__ is too big to fit in TURBO_INTERNAL_EMSCRIPTEN_VERSION
#endif
#define TURBO_INTERNAL_EMSCRIPTEN_VERSION                              \
  ((__EMSCRIPTEN_major__) * 1000000 + (__EMSCRIPTEN_minor__) * 1000 + \
   (__EMSCRIPTEN_tiny__))
#endif
#endif

#endif  // TURBO_BASE_CONFIG_H_
