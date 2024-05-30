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

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/base/port.h>

// TURBO_ARRAYSIZE()
//
// Returns the number of elements in an array as a compile-time constant, which
// can be used in defining new arrays. If you use this macro on a pointer by
// mistake, you will get a compile-time error.
#define TURBO_ARRAYSIZE(array) \
  (sizeof(::turbo::macros_internal::ArraySizeHelper(array)))

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace macros_internal {
// Note: this internal template function declaration is used by TURBO_ARRAYSIZE.
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
  (TURBO_LIKELY((expr)) ? static_cast<void>(0) \
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
  (TURBO_LIKELY((expr)) ? static_cast<void>(0) \
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

// TURBO_DEPRECATE_AND_INLINE()
//
// Marks a function or type alias as deprecated and tags it to be picked up for
// automated refactoring by go/cpp-inliner. It can added to inline function
// definitions or type aliases. It should only be used within a header file. It
// differs from `TURBO_DEPRECATED` in the following ways:
//
// 1. New uses of the function or type will be discouraged via Tricorder
//    warnings.
// 2. If enabled via `METADATA`, automated changes will be sent out inlining the
//    functions's body or replacing the type where it is used.
//
// For example:
//
// TURBO_DEPRECATE_AND_INLINE() inline int OldFunc(int x) {
//   return NewFunc(x, 0);
// }
//
// will mark `OldFunc` as deprecated, and the go/cpp-inliner service will
// replace calls to `OldFunc(x)` with calls to `NewFunc(x, 0)`. Once all calls
// to `OldFunc` have been replaced, `OldFunc` can be deleted.
//
// See go/cpp-inliner for more information.
//
// Note: go/cpp-inliner is Google-internal service for automated refactoring.
// While open-source users do not have access to this service, the macro is
// provided for compatibility, and so that users receive deprecation warnings.
#if TURBO_HAVE_CPP_ATTRIBUTE(deprecated) && \
    TURBO_HAVE_CPP_ATTRIBUTE(clang::annotate)
#define TURBO_DEPRECATE_AND_INLINE() [[deprecated, clang::annotate("inline-me")]]
#elif TURBO_HAVE_CPP_ATTRIBUTE(deprecated)
#define TURBO_DEPRECATE_AND_INLINE() [[deprecated]]
#else
#define TURBO_DEPRECATE_AND_INLINE()
#endif

// Requires the compiler to prove that the size of the given object is at least
// the expected amount.
#if TURBO_HAVE_ATTRIBUTE(diagnose_if) && TURBO_HAVE_BUILTIN(__builtin_object_size)
#define TURBO_INTERNAL_NEED_MIN_SIZE(Obj, N)                     \
  __attribute__((diagnose_if(__builtin_object_size(Obj, 0) < N, \
                             "object size provably too small "  \
                             "(this would corrupt memory)",     \
                             "error")))
#else
#define TURBO_INTERNAL_NEED_MIN_SIZE(Obj, N)
#endif

// ------------------------------------------------------------------------
// TURBO_CONCAT
//
// This macro joins the two arguments together, even when one of
// the arguments is itself a macro (see 16.3.1 in C++98 standard).
// This is often used to create a unique name with __LINE__.
//
// For example, this declaration:
//    char TURBO_CONCAT(unique_, __LINE__);
// expands to this:
//    char unique_73;
//
// Note that all versions of MSVC++ up to at least version 7.1
// fail to properly compile macros that use __LINE__ in them
// when the "program database for edit and continue" option
// is enabled. The result is that __LINE__ gets converted to
// something like __LINE__(Var+37).
//
#ifndef TURBO_CONCAT
#define TURBO_CONCAT(a, b)  TURBO_CONCAT1(a, b)
#define TURBO_CONCAT1(a, b) TURBO_CONCAT2(a, b)
#define TURBO_CONCAT2(a, b) a##b
#endif

#endif  // TURBO_BASE_MACROS_H_
