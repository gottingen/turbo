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

#ifndef TURBO_RANDOM_INTERNAL_MOCK_OVERLOAD_SET_H_
#define TURBO_RANDOM_INTERNAL_MOCK_OVERLOAD_SET_H_

#include <tuple>
#include <type_traits>

#include <gmock/gmock.h>
#include <turbo/base/config.h>
#include <tests/random/mock_helpers.h>
#include <tests/random/mocking_bit_gen.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

template <typename DistrT, typename ValidatorT, typename Fn>
struct MockSingleOverload;

// MockSingleOverload
//
// MockSingleOverload hooks in to gMock's `ON_CALL` and `EXPECT_CALL` macros.
// EXPECT_CALL(mock_single_overload, Call(...))` will expand to a call to
// `mock_single_overload.gmock_Call(...)`. Because expectations are stored on
// the MockingBitGen (an argument passed inside `Call(...)`), this forwards to
// arguments to MockingBitGen::Register.
//
// The underlying KeyT must match the KeyT constructed by DistributionCaller.
template <typename DistrT, typename ValidatorT, typename Ret, typename... Args>
struct MockSingleOverload<DistrT, ValidatorT, Ret(MockingBitGen&, Args...)> {
  static_assert(std::is_same<typename DistrT::result_type, Ret>::value,
                "Overload signature must have return type matching the "
                "distribution result_type.");
  using KeyT = Ret(DistrT, std::tuple<Args...>);

  template <typename MockURBG>
  auto gmock_Call(MockURBG& gen, const ::testing::Matcher<Args>&... matchers)
      -> decltype(MockHelpers::MockFor<KeyT>(gen, ValidatorT())
                      .gmock_Call(matchers...)) {
    static_assert(
        std::is_base_of<MockingBitGenImpl<true>, MockURBG>::value ||
            std::is_base_of<MockingBitGenImpl<false>, MockURBG>::value,
        "Mocking requires an turbo::MockingBitGen");
    return MockHelpers::MockFor<KeyT>(gen, ValidatorT())
        .gmock_Call(matchers...);
  }
};

template <typename DistrT, typename ValidatorT, typename Ret, typename Arg,
          typename... Args>
struct MockSingleOverload<DistrT, ValidatorT,
                          Ret(Arg, MockingBitGen&, Args...)> {
  static_assert(std::is_same<typename DistrT::result_type, Ret>::value,
                "Overload signature must have return type matching the "
                "distribution result_type.");
  using KeyT = Ret(DistrT, std::tuple<Arg, Args...>);

  template <typename MockURBG>
  auto gmock_Call(const ::testing::Matcher<Arg>& matcher, MockURBG& gen,
                  const ::testing::Matcher<Args>&... matchers)
      -> decltype(MockHelpers::MockFor<KeyT>(gen, ValidatorT())
                      .gmock_Call(matcher, matchers...)) {
    static_assert(
        std::is_base_of<MockingBitGenImpl<true>, MockURBG>::value ||
            std::is_base_of<MockingBitGenImpl<false>, MockURBG>::value,
        "Mocking requires an turbo::MockingBitGen");
    return MockHelpers::MockFor<KeyT>(gen, ValidatorT())
        .gmock_Call(matcher, matchers...);
  }
};

// MockOverloadSetWithValidator
//
// MockOverloadSetWithValidator is a wrapper around MockOverloadSet which takes
// an additional Validator parameter, allowing for customization of the mock
// behavior.
//
// `ValidatorT::Validate(result, args...)` will be called after the mock
// distribution returns a value in `result`, allowing for validation against the
// args.
template <typename DistrT, typename ValidatorT, typename... Fns>
struct MockOverloadSetWithValidator;

template <typename DistrT, typename ValidatorT, typename Sig>
struct MockOverloadSetWithValidator<DistrT, ValidatorT, Sig>
    : public MockSingleOverload<DistrT, ValidatorT, Sig> {
  using MockSingleOverload<DistrT, ValidatorT, Sig>::gmock_Call;
};

template <typename DistrT, typename ValidatorT, typename FirstSig,
          typename... Rest>
struct MockOverloadSetWithValidator<DistrT, ValidatorT, FirstSig, Rest...>
    : public MockSingleOverload<DistrT, ValidatorT, FirstSig>,
      public MockOverloadSetWithValidator<DistrT, ValidatorT, Rest...> {
  using MockSingleOverload<DistrT, ValidatorT, FirstSig>::gmock_Call;
  using MockOverloadSetWithValidator<DistrT, ValidatorT, Rest...>::gmock_Call;
};

// MockOverloadSet
//
// MockOverloadSet takes a distribution and a collection of signatures and
// performs overload resolution amongst all the overloads. This makes
// `EXPECT_CALL(mock_overload_set, Call(...))` expand and do overload resolution
// correctly.
template <typename DistrT, typename... Signatures>
using MockOverloadSet =
    MockOverloadSetWithValidator<DistrT, NoOpValidator, Signatures...>;

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // TURBO_RANDOM_INTERNAL_MOCK_OVERLOAD_SET_H_
