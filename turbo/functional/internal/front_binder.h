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

// Implementation details for `turbo::bind_front()`.

#ifndef TURBO_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_
#define TURBO_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#include <turbo/base/internal/invoke.h>
#include <turbo/container/internal/compressed_tuple.h>
#include <turbo/meta/type_traits.h>
#include <turbo/meta/utility.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace functional_internal {

// Invoke the method, expanding the tuple of bound arguments.
template <class R, class Tuple, size_t... Idx, class... Args>
R Apply(Tuple&& bound, turbo::index_sequence<Idx...>, Args&&... free) {
  return base_internal::invoke(
      std::forward<Tuple>(bound).template get<Idx>()...,
      std::forward<Args>(free)...);
}

template <class F, class... BoundArgs>
class FrontBinder {
  using BoundArgsT = turbo::container_internal::CompressedTuple<F, BoundArgs...>;
  using Idx = turbo::make_index_sequence<sizeof...(BoundArgs) + 1>;

  BoundArgsT bound_args_;

 public:
  template <class... Ts>
  constexpr explicit FrontBinder(std::in_place_t, Ts&&... ts)
      : bound_args_(std::forward<Ts>(ts)...) {}

  template <class... FreeArgs, class R = base_internal::invoke_result_t<
                                   F&, BoundArgs&..., FreeArgs&&...>>
  R operator()(FreeArgs&&... free_args) & {
    return functional_internal::Apply<R>(bound_args_, Idx(),
                                         std::forward<FreeArgs>(free_args)...);
  }

  template <class... FreeArgs,
            class R = base_internal::invoke_result_t<
                const F&, const BoundArgs&..., FreeArgs&&...>>
  R operator()(FreeArgs&&... free_args) const& {
    return functional_internal::Apply<R>(bound_args_, Idx(),
                                         std::forward<FreeArgs>(free_args)...);
  }

  template <class... FreeArgs, class R = base_internal::invoke_result_t<
                                   F&&, BoundArgs&&..., FreeArgs&&...>>
  R operator()(FreeArgs&&... free_args) && {
    // This overload is called when *this is an rvalue. If some of the bound
    // arguments are stored by value or rvalue reference, we move them.
    return functional_internal::Apply<R>(std::move(bound_args_), Idx(),
                                         std::forward<FreeArgs>(free_args)...);
  }

  template <class... FreeArgs,
            class R = base_internal::invoke_result_t<
                const F&&, const BoundArgs&&..., FreeArgs&&...>>
  R operator()(FreeArgs&&... free_args) const&& {
    // This overload is called when *this is an rvalue. If some of the bound
    // arguments are stored by value or rvalue reference, we move them.
    return functional_internal::Apply<R>(std::move(bound_args_), Idx(),
                                         std::forward<FreeArgs>(free_args)...);
  }
};

template <class F, class... BoundArgs>
using bind_front_t = FrontBinder<decay_t<F>, turbo::decay_t<BoundArgs>...>;

}  // namespace functional_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FUNCTIONAL_INTERNAL_FRONT_BINDER_H_
