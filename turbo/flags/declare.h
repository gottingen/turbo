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
// File: declare.h
// -----------------------------------------------------------------------------
//
// This file defines the TURBO_DECLARE_FLAG macro, allowing you to declare an
// `turbo::Flag` for use within a translation unit. You should place this
// declaration within the header file associated with the .cc file that defines
// and owns the `Flag`.

#ifndef TURBO_FLAGS_DECLARE_H_
#define TURBO_FLAGS_DECLARE_H_

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

// turbo::Flag<T> represents a flag of type 'T' created by TURBO_FLAG.
template <typename T>
class Flag;

}  // namespace flags_internal

// Flag
//
// Forward declaration of the `turbo::Flag` type for use in defining the macro.
template <typename T>
using Flag = flags_internal::Flag<T>;

TURBO_NAMESPACE_END
}  // namespace turbo

// TURBO_DECLARE_FLAG()
//
// This macro is a convenience for declaring use of an `turbo::Flag` within a
// translation unit. This macro should be used within a header file to
// declare usage of the flag within any .cc file including that header file.
//
// The TURBO_DECLARE_FLAG(type, name) macro expands to:
//
//   extern turbo::Flag<type> FLAGS_name;
#define TURBO_DECLARE_FLAG(type, name) TURBO_DECLARE_FLAG_INTERNAL(type, name)

// Internal implementation of TURBO_DECLARE_FLAG to allow macro expansion of its
// arguments. Clients must use TURBO_DECLARE_FLAG instead.
#define TURBO_DECLARE_FLAG_INTERNAL(type, name)               \
  extern turbo::Flag<type> FLAGS_##name;                      \
  namespace turbo /* block flags in namespaces */ {}          \
  /* second redeclaration is to allow applying attributes */ \
  extern turbo::Flag<type> FLAGS_##name

#endif  // TURBO_FLAGS_DECLARE_H_
