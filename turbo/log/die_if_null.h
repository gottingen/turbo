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
// File: log/die_if_null.h
// -----------------------------------------------------------------------------
//
// This header declares macro `TURBO_DIE_IF_NULL`.

#ifndef TURBO_LOG_DIE_IF_NULL_H_
#define TURBO_LOG_DIE_IF_NULL_H_

#include <stdint.h>

#include <utility>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>

// TURBO_DIE_IF_NULL()
//
// `TURBO_DIE_IF_NULL` behaves as `CHECK_NE` against `nullptr` but *also*
// "returns" its argument.  It is useful in initializers where statements (like
// `CHECK_NE`) can't be used.  Outside initializers, prefer `CHECK` or
// `CHECK_NE`. `TURBO_DIE_IF_NULL` works for both raw pointers and (compatible)
// smart pointers including `std::unique_ptr` and `std::shared_ptr`; more
// generally, it works for any type that can be compared to nullptr_t.  For
// types that aren't raw pointers, `TURBO_DIE_IF_NULL` returns a reference to
// its argument, preserving the value category. Example:
//
//   Foo() : bar_(TURBO_DIE_IF_NULL(MethodReturningUniquePtr())) {}
//
// Use `CHECK(ptr)` or `CHECK(ptr != nullptr)` if the returned pointer is
// unused.
#define TURBO_DIE_IF_NULL(val) \
  ::turbo::log_internal::DieIfNull(__FILE__, __LINE__, #val, (val))

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// Crashes the process after logging `exprtext` annotated at the `file` and
// `line` location. Called when `TURBO_DIE_IF_NULL` fails. Calling this function
// generates less code than its implementation would if inlined, for a slight
// code size reduction each time `TURBO_DIE_IF_NULL` is called.
TURBO_ATTRIBUTE_NORETURN TURBO_ATTRIBUTE_NOINLINE void DieBecauseNull(
    const char* file, int line, const char* exprtext);

// Helper for `TURBO_DIE_IF_NULL`.
template <typename T>
TURBO_MUST_USE_RESULT T DieIfNull(const char* file, int line,
                                 const char* exprtext, T&& t) {
  if (TURBO_PREDICT_FALSE(t == nullptr)) {
    // Call a non-inline helper function for a small code size improvement.
    DieBecauseNull(file, line, exprtext);
  }
  return std::forward<T>(t);
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_DIE_IF_NULL_H_
