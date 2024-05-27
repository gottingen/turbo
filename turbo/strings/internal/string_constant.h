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

#ifndef TURBO_STRINGS_INTERNAL_STRING_CONSTANT_H_
#define TURBO_STRINGS_INTERNAL_STRING_CONSTANT_H_

#include <turbo/meta/type_traits.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

// StringConstant<T> represents a compile time string constant.
// It can be accessed via its `turbo::string_view value` static member.
// It is guaranteed that the `string_view` returned has constant `.data()`,
// constant `.size()` and constant `value[i]` for all `0 <= i < .size()`
//
// The `T` is an opaque type. It is guaranteed that different string constants
// will have different values of `T`. This allows users to associate the string
// constant with other static state at compile time.
//
// Instances should be made using the `MakeStringConstant()` factory function
// below.
template <typename T>
struct StringConstant {
 private:
  static constexpr bool TryConstexprEval(turbo::string_view view) {
    return view.empty() || 2 * view[0] != 1;
  }

 public:
  static constexpr turbo::string_view value = T{}();
  constexpr turbo::string_view operator()() const { return value; }

  // Check to be sure `view` points to constant data.
  // Otherwise, it can't be constant evaluated.
  static_assert(TryConstexprEval(value),
                "The input string_view must point to constant data.");
};

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
template <typename T>
constexpr turbo::string_view StringConstant<T>::value;
#endif

// Factory function for `StringConstant` instances.
// It supports callables that have a constexpr default constructor and a
// constexpr operator().
// It must return an `turbo::string_view` or `const char*` pointing to constant
// data. This is validated at compile time.
template <typename T>
constexpr StringConstant<T> MakeStringConstant(T) {
  return {};
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STRING_CONSTANT_H_
