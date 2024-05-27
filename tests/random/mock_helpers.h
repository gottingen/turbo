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

#ifndef TURBO_RANDOM_INTERNAL_MOCK_HELPERS_H_
#define TURBO_RANDOM_INTERNAL_MOCK_HELPERS_H_

#include <utility>

#include <turbo/base/config.h>
#include <turbo/base/internal/fast_type_id.h>
#include <turbo/types/optional.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// A no-op validator meeting the ValidatorT requirements for MockHelpers.
//
// Custom validators should follow a similar structure, passing the type to
// MockHelpers::MockFor<KeyT>(m, CustomValidatorT()).
struct NoOpValidator {
  // Default validation: do nothing.
  template <typename ResultT, typename... Args>
  static void Validate(ResultT, Args&&...) {}
};

// MockHelpers works in conjunction with MockOverloadSet, MockingBitGen, and
// BitGenRef to enable the mocking capability for turbo distribution functions.
//
// MockingBitGen registers mocks based on the typeid of a mock signature, KeyT,
// which is used to generate a unique id.
//
// KeyT is a signature of the form:
//   result_type(discriminator_type, std::tuple<args...>)
// The mocked function signature will be composed from KeyT as:
//   result_type(args...)
//
class MockHelpers {
  using IdType = ::turbo::base_internal::FastTypeIdType;

  // Given a key signature type used to index the mock, extract the components.
  // KeyT is expected to have the form:
  //   result_type(discriminator_type, arg_tuple_type)
  template <typename KeyT>
  struct KeySignature;

  template <typename ResultT, typename DiscriminatorT, typename ArgTupleT>
  struct KeySignature<ResultT(DiscriminatorT, ArgTupleT)> {
    using result_type = ResultT;
    using discriminator_type = DiscriminatorT;
    using arg_tuple_type = ArgTupleT;
  };

  // Detector for InvokeMock.
  template <class T>
  using invoke_mock_t = decltype(std::declval<T*>()->InvokeMock(
      std::declval<IdType>(), std::declval<void*>(), std::declval<void*>()));

  // Empty implementation of InvokeMock.
  template <typename KeyT, typename ReturnT, typename ArgTupleT, typename URBG,
            typename... Args>
  static turbo::optional<ReturnT> InvokeMockImpl(char, URBG*, Args&&...) {
    return turbo::nullopt;
  }

  // Non-empty implementation of InvokeMock.
  template <typename KeyT, typename ReturnT, typename ArgTupleT, typename URBG,
            typename = invoke_mock_t<URBG>, typename... Args>
  static turbo::optional<ReturnT> InvokeMockImpl(int, URBG* urbg,
                                                Args&&... args) {
    ArgTupleT arg_tuple(std::forward<Args>(args)...);
    ReturnT result;
    if (urbg->InvokeMock(::turbo::base_internal::FastTypeId<KeyT>(), &arg_tuple,
                         &result)) {
      return result;
    }
    return turbo::nullopt;
  }

 public:
  // InvokeMock is private; this provides access for some specialized use cases.
  template <typename URBG>
  static inline bool PrivateInvokeMock(URBG* urbg, IdType type,
                                       void* args_tuple, void* result) {
    return urbg->InvokeMock(type, args_tuple, result);
  }

  // Invoke a mock for the KeyT (may or may not be a signature).
  //
  // KeyT is used to generate a typeid-based lookup key for the mock.
  // KeyT is a signature of the form:
  //   result_type(discriminator_type, std::tuple<args...>)
  // The mocked function signature will be composed from KeyT as:
  //   result_type(args...)
  //
  // An instance of arg_tuple_type must be constructable from Args..., since
  // the underlying mechanism requires a pointer to an argument tuple.
  template <typename KeyT, typename URBG, typename... Args>
  static auto MaybeInvokeMock(URBG* urbg, Args&&... args)
      -> turbo::optional<typename KeySignature<KeyT>::result_type> {
    // Use function overloading to dispatch to the implementation since
    // more modern patterns (e.g. require + constexpr) are not supported in all
    // compiler configurations.
    return InvokeMockImpl<KeyT, typename KeySignature<KeyT>::result_type,
                          typename KeySignature<KeyT>::arg_tuple_type, URBG>(
        0, urbg, std::forward<Args>(args)...);
  }

  // Acquire a mock for the KeyT (may or may not be a signature), set up to use
  // the ValidatorT to verify that the result is in the range of the RNG
  // function.
  //
  // KeyT is used to generate a typeid-based lookup for the mock.
  // KeyT is a signature of the form:
  //   result_type(discriminator_type, std::tuple<args...>)
  // The mocked function signature will be composed from KeyT as:
  //   result_type(args...)
  // ValidatorT::Validate will be called after the result of the RNG. The
  //   signature is expected to be of the form:
  //      ValidatorT::Validate(result, args...)
  template <typename KeyT, typename ValidatorT, typename MockURBG>
  static auto MockFor(MockURBG& m, ValidatorT)
      -> decltype(m.template RegisterMock<
                  typename KeySignature<KeyT>::result_type,
                  typename KeySignature<KeyT>::arg_tuple_type>(
          m, std::declval<IdType>(), ValidatorT())) {
    return m.template RegisterMock<typename KeySignature<KeyT>::result_type,
                                   typename KeySignature<KeyT>::arg_tuple_type>(
        m, ::turbo::base_internal::FastTypeId<KeyT>(), ValidatorT());
  }

  // Acquire a mock for the KeyT (may or may not be a signature).
  //
  // KeyT is used to generate a typeid-based lookup for the mock.
  // KeyT is a signature of the form:
  //   result_type(discriminator_type, std::tuple<args...>)
  // The mocked function signature will be composed from KeyT as:
  //   result_type(args...)
  template <typename KeyT, typename MockURBG>
  static decltype(auto) MockFor(MockURBG& m) {
    return MockFor<KeyT>(m, NoOpValidator());
  }
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_MOCK_HELPERS_H_
