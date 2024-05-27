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
// bad_variant_access.h
// -----------------------------------------------------------------------------
//
// This header file defines the `turbo::bad_variant_access` type.

#ifndef TURBO_TYPES_BAD_VARIANT_ACCESS_H_
#define TURBO_TYPES_BAD_VARIANT_ACCESS_H_

#include <stdexcept>

#include <turbo/base/config.h>

#ifdef TURBO_USES_STD_VARIANT

#include <variant>

namespace turbo {
TURBO_NAMESPACE_BEGIN
using std::bad_variant_access;
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // TURBO_USES_STD_VARIANT

namespace turbo {
TURBO_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
// bad_variant_access
// -----------------------------------------------------------------------------
//
// An `turbo::bad_variant_access` type is an exception type that is thrown in
// the following cases:
//
//   * Calling `turbo::get(turbo::variant) with an index or type that does not
//     match the currently selected alternative type
//   * Calling `turbo::visit on an `turbo::variant` that is in the
//     `variant::valueless_by_exception` state.
//
// Example:
//
//   turbo::variant<int, std::string> v;
//   v = 1;
//   try {
//     turbo::get<std::string>(v);
//   } catch(const turbo::bad_variant_access& e) {
//     std::cout << "Bad variant access: " << e.what() << '\n';
//   }
class bad_variant_access : public std::exception {
 public:
  bad_variant_access() noexcept = default;
  ~bad_variant_access() override;
  const char* what() const noexcept override;
};

namespace variant_internal {

[[noreturn]] TURBO_DLL void ThrowBadVariantAccess();
[[noreturn]] TURBO_DLL void Rethrow();

}  // namespace variant_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_VARIANT

#endif  // TURBO_TYPES_BAD_VARIANT_ACCESS_H_
