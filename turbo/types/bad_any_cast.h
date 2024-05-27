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
// bad_any_cast.h
// -----------------------------------------------------------------------------
//
// This header file defines the `turbo::bad_any_cast` type.

#ifndef TURBO_TYPES_BAD_ANY_CAST_H_
#define TURBO_TYPES_BAD_ANY_CAST_H_

#include <typeinfo>

#include <turbo/base/config.h>

#ifdef TURBO_USES_STD_ANY

#include <any>

namespace turbo {
TURBO_NAMESPACE_BEGIN
using std::bad_any_cast;
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // TURBO_USES_STD_ANY

namespace turbo {
TURBO_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
// bad_any_cast
// -----------------------------------------------------------------------------
//
// An `turbo::bad_any_cast` type is an exception type that is thrown when
// failing to successfully cast the return value of an `turbo::any` object.
//
// Example:
//
//   auto a = turbo::any(65);
//   turbo::any_cast<int>(a);         // 65
//   try {
//     turbo::any_cast<char>(a);
//   } catch(const turbo::bad_any_cast& e) {
//     std::cout << "Bad any cast: " << e.what() << '\n';
//   }
class bad_any_cast : public std::bad_cast {
 public:
  ~bad_any_cast() override;
  const char* what() const noexcept override;
};

namespace any_internal {

[[noreturn]] void ThrowBadAnyCast();

}  // namespace any_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_ANY

#endif  // TURBO_TYPES_BAD_ANY_CAST_H_
