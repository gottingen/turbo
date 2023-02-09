// Copyright 2018 The Turbo Authors.
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
// bad_variant_access.h
// -----------------------------------------------------------------------------
//
// This header file defines the `turbo::bad_variant_access` type.

#ifndef TURBO_TYPES_BAD_VARIANT_ACCESS_H_
#define TURBO_TYPES_BAD_VARIANT_ACCESS_H_

#include <stdexcept>

#include "turbo/platform/port.h"

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
