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

// The IfConstexpr and IfConstexprElse utilities in this file are meant to be
// used to emulate `if constexpr` in pre-C++17 mode in library implementation.
// The motivation is to allow for avoiding complex SFINAE.
//
// The functions passed in must depend on the type(s) of the object(s) that
// require SFINAE. For example:
// template<typename T>
// int MaybeFoo(T& t) {
//   if constexpr (HasFoo<T>::value) return t.foo();
//   return 0;
// }
//
// can be written in pre-C++17 as:
//
// template<typename T>
// int MaybeFoo(T& t) {
//   int i = 0;
//   turbo::utility_internal::IfConstexpr<HasFoo<T>::value>(
//       [&](const auto& fooer) { i = fooer.foo(); }, t);
//   return i;
// }

#ifndef TURBO_UTILITY_INTERNAL_IF_CONSTEXPR_H_
#define TURBO_UTILITY_INTERNAL_IF_CONSTEXPR_H_

#include <tuple>
#include <utility>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace utility_internal {

template <bool condition, typename TrueFunc, typename FalseFunc,
          typename... Args>
auto IfConstexprElse(TrueFunc&& true_func, FalseFunc&& false_func,
                     Args&&... args) {
  return std::get<condition>(std::forward_as_tuple(
      std::forward<FalseFunc>(false_func), std::forward<TrueFunc>(true_func)))(
      std::forward<Args>(args)...);
}

template <bool condition, typename Func, typename... Args>
void IfConstexpr(Func&& func, Args&&... args) {
  IfConstexprElse<condition>(std::forward<Func>(func), [](auto&&...){},
                             std::forward<Args>(args)...);
}

}  // namespace utility_internal

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_UTILITY_INTERNAL_IF_CONSTEXPR_H_
