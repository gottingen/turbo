//
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

#ifndef TURBO_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_
#define TURBO_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_

#include <utility>
#include <type_traits>

#include <turbo/base/config.h>
#include <turbo/base/internal/fast_type_id.h>
#include <turbo/meta/utility.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// DistributionCaller provides an opportunity to overload the general
// mechanism for calling a distribution, allowing for mock-RNG classes
// to intercept such calls.
template <typename URBG>
struct DistributionCaller {
  static_assert(!std::is_pointer<URBG>::value,
                "You must pass a reference, not a pointer.");
  // SFINAE to detect whether the URBG type includes a member matching
  // bool InvokeMock(base_internal::FastTypeIdType, void*, void*).
  //
  // These live inside BitGenRef so that they have friend access
  // to MockingBitGen. (see similar methods in DistributionCaller).
  template <template <class...> class Trait, class AlwaysVoid, class... Args>
  struct detector : std::false_type {};
  template <template <class...> class Trait, class... Args>
  struct detector<Trait, turbo::void_t<Trait<Args...>>, Args...>
      : std::true_type {};

  template <class T>
  using invoke_mock_t = decltype(std::declval<T*>()->InvokeMock(
      std::declval<::turbo::base_internal::FastTypeIdType>(),
      std::declval<void*>(), std::declval<void*>()));

  using HasInvokeMock = typename detector<invoke_mock_t, void, URBG>::type;

  // Default implementation of distribution caller.
  template <typename DistrT, typename... Args>
  static typename DistrT::result_type Impl(std::false_type, URBG* urbg,
                                           Args&&... args) {
    DistrT dist(std::forward<Args>(args)...);
    return dist(*urbg);
  }

  // Mock implementation of distribution caller.
  // The underlying KeyT must match the KeyT constructed by MockOverloadSet.
  template <typename DistrT, typename... Args>
  static typename DistrT::result_type Impl(std::true_type, URBG* urbg,
                                           Args&&... args) {
    using ResultT = typename DistrT::result_type;
    using ArgTupleT = std::tuple<turbo::decay_t<Args>...>;
    using KeyT = ResultT(DistrT, ArgTupleT);

    ArgTupleT arg_tuple(std::forward<Args>(args)...);
    ResultT result;
    if (!urbg->InvokeMock(::turbo::base_internal::FastTypeId<KeyT>(), &arg_tuple,
                          &result)) {
      auto dist = turbo::make_from_tuple<DistrT>(arg_tuple);
      result = dist(*urbg);
    }
    return result;
  }

  // Default implementation of distribution caller.
  template <typename DistrT, typename... Args>
  static typename DistrT::result_type Call(URBG* urbg, Args&&... args) {
    return Impl<DistrT, Args...>(HasInvokeMock{}, urbg,
                                 std::forward<Args>(args)...);
  }
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_DISTRIBUTION_CALLER_H_
