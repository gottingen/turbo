//
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
// Created by jeff on 24-6-2.
//

#pragma once

#include <turbo/base/config.h>


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


// TURBO_ASSUME(cond)
//
// Informs the compiler that a condition is always true and that it can assume
// it to be true for optimization purposes.
//
// WARNING: If the condition is false, the program can produce undefined and
// potentially dangerous behavior.
//
// In !NDEBUG mode, the condition is checked with an assert().
//
// NOTE: The expression must not have side effects, as it may only be evaluated
// in some compilation modes and not others. Some compilers may issue a warning
// if the compiler cannot prove the expression has no side effects. For example,
// the expression should not use a function call since the compiler cannot prove
// that a function call does not have side effects.
//
// Example:
//
//   int x = ...;
//   TURBO_ASSUME(x >= 0);
//   // The compiler can optimize the division to a simple right shift using the
//   // assumption specified above.
//   int y = x / 16;
//
#if !defined(NDEBUG)
#define TURBO_ASSUME(cond) assert(cond)
#elif TURBO_HAVE_BUILTIN(__builtin_assume)
#define TURBO_ASSUME(cond) __builtin_assume(cond)
#elif defined(_MSC_VER)
#define TURBO_ASSUME(cond) __assume(cond)
#elif defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#define TURBO_ASSUME(cond)            \
  do {                               \
    if (!(cond)) std::unreachable(); \
  } while (false)
#elif defined(__GNUC__) || TURBO_HAVE_BUILTIN(__builtin_unreachable)
#define TURBO_ASSUME(cond)                 \
  do {                                    \
    if (!(cond)) __builtin_unreachable(); \
  } while (false)
#else
#define TURBO_ASSUME(cond)               \
  do {                                  \
    static_cast<void>(false && (cond)); \
  } while (false)
#endif

