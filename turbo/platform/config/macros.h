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
// File: macros.h
// -----------------------------------------------------------------------------
//
// This header file defines the set of language macros used within Turbo code.
// For the set of macros used to determine supported compilers and platforms,
// see turbo/base/config.h instead.
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.

#ifndef TURBO_BASE_MACROS_H_
#define TURBO_BASE_MACROS_H_

#include <cassert>
#include <cstddef>

#include "optimization.h"
#include "turbo/platform/config/attributes.h"
#include "turbo/platform/config/config.h"
#include "turbo/platform/port.h"

// TURBO_ARRAY_SIZE()
//
// Returns the number of elements in an array as a compile-time constant, which
// can be used in defining new arrays. If you use this macro on a pointer by
// mistake, you will get a compile-time error.
#define TURBO_ARRAY_SIZE(array) \
  (sizeof(::turbo::macros_internal::ArraySizeHelper(array)))

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace macros_internal {
// Note: this internal template function declaration is used by TURBO_ARRAY_SIZE.
// The function doesn't need a definition, as we only use its type.
template <typename T, size_t N>
auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace macros_internal
TURBO_NAMESPACE_END
}  // namespace turbo

// TURBO_BAD_CALL_IF()
//
// Used on a function overload to trap bad calls: any call that matches the
// overload will cause a compile-time error. This macro uses a clang-specific
// "enable_if" attribute, as described at
// https://clang.llvm.org/docs/AttributeReference.html#enable-if
//
// Overloads which use this macro should be bracketed by
// `#ifdef TURBO_BAD_CALL_IF`.
//
// Example:
//
//   int isdigit(int c);
//   #ifdef TURBO_BAD_CALL_IF
//   int isdigit(int c)
//     TURBO_BAD_CALL_IF(c <= -1 || c > 255,
//                       "'c' must have the value of an unsigned char or EOF");
//   #endif // TURBO_BAD_CALL_IF
#if TURBO_HAVE_ATTRIBUTE(enable_if)
#define TURBO_BAD_CALL_IF(expr, msg) \
  __attribute__((enable_if(expr, "Bad call trap"), unavailable(msg)))
#endif

// TURBO_ASSERT()
//
// In C++11, `assert` can't be used portably within constexpr functions.
// TURBO_ASSERT functions as a runtime assert but works in C++11 constexpr
// functions.  Example:
//
// constexpr double Divide(double a, double b) {
//   return TURBO_ASSERT(b != 0), a / b;
// }
//
// This macro is inspired by
// https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
#if defined(NDEBUG)
#define TURBO_ASSERT(expr) \
  (false ? static_cast<void>(expr) : static_cast<void>(0))
#else
#define TURBO_ASSERT(expr)                           \
  (TURBO_PREDICT_TRUE((expr)) ? static_cast<void>(0) \
                             : [] { assert(false && #expr); }())  // NOLINT
#endif

// `TURBO_INTERNAL_HARDENING_ABORT()` controls how `TURBO_HARDENING_ASSERT()`
// aborts the program in release mode (when NDEBUG is defined). The
// implementation should abort the program as quickly as possible and ideally it
// should not be possible to ignore the abort request.
#define TURBO_INTERNAL_HARDENING_ABORT()   \
  do {                                    \
    TURBO_INTERNAL_IMMEDIATE_ABORT_IMPL(); \
    TURBO_INTERNAL_UNREACHABLE_IMPL();     \
  } while (false)

// TURBO_HARDENING_ASSERT()
//
// `TURBO_HARDENING_ASSERT()` is like `TURBO_ASSERT()`, but used to implement
// runtime assertions that should be enabled in hardened builds even when
// `NDEBUG` is defined.
//
// When `NDEBUG` is not defined, `TURBO_HARDENING_ASSERT()` is identical to
// `TURBO_ASSERT()`.
//
// See `TURBO_OPTION_HARDENED` in `turbo/base/options.h` for more information on
// hardened mode.
#if TURBO_OPTION_HARDENED == 1 && defined(NDEBUG)
#define TURBO_HARDENING_ASSERT(expr)                 \
  (TURBO_PREDICT_TRUE((expr)) ? static_cast<void>(0) \
                             : [] { TURBO_INTERNAL_HARDENING_ABORT(); }())
#else
#define TURBO_HARDENING_ASSERT(expr) TURBO_ASSERT(expr)
#endif

#ifdef TURBO_HAVE_EXCEPTIONS
#define TURBO_INTERNAL_TRY try
#define TURBO_INTERNAL_CATCH_ANY catch (...)
#define TURBO_INTERNAL_RETHROW do { throw; } while (false)
#else  // TURBO_HAVE_EXCEPTIONS
#define TURBO_INTERNAL_TRY if (true)
#define TURBO_INTERNAL_CATCH_ANY else if (false)
#define TURBO_INTERNAL_RETHROW do {} while (false)
#endif  // TURBO_HAVE_EXCEPTIONS

#endif  // TURBO_BASE_MACROS_H_
