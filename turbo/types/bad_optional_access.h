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
// bad_optional_access.h
// -----------------------------------------------------------------------------
//
// This header file defines the `turbo::bad_optional_access` type.

#ifndef TURBO_TYPES_BAD_OPTIONAL_ACCESS_H_
#define TURBO_TYPES_BAD_OPTIONAL_ACCESS_H_

#include <stdexcept>

#include <turbo/base/config.h>

#ifdef TURBO_USES_STD_OPTIONAL

#include <optional>

namespace turbo {
TURBO_NAMESPACE_BEGIN
using std::bad_optional_access;
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // TURBO_USES_STD_OPTIONAL

namespace turbo {
TURBO_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
// bad_optional_access
// -----------------------------------------------------------------------------
//
// An `turbo::bad_optional_access` type is an exception type that is thrown when
// attempting to access an `turbo::optional` object that does not contain a
// value.
//
// Example:
//
//   turbo::optional<int> o;
//
//   try {
//     int n = o.value();
//   } catch(const turbo::bad_optional_access& e) {
//     std::cout << "Bad optional access: " << e.what() << '\n';
//   }
class bad_optional_access : public std::exception {
 public:
  bad_optional_access() = default;
  ~bad_optional_access() override;
  const char* what() const noexcept override;
};

namespace optional_internal {

// throw delegator
[[noreturn]] TURBO_DLL void throw_bad_optional_access();

}  // namespace optional_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_OPTIONAL

#endif  // TURBO_TYPES_BAD_OPTIONAL_ACCESS_H_
