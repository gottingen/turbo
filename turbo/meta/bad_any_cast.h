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
// bad_any_cast.h
// -----------------------------------------------------------------------------
//
// This header file defines the `turbo::bad_any_cast` type.

#ifndef TURBO_TYPES_BAD_ANY_CAST_H_
#define TURBO_TYPES_BAD_ANY_CAST_H_

#include <typeinfo>

#include "turbo/platform/port.h"

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
