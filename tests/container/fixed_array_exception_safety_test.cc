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

#include <turbo/base/config.h>
#include <turbo/container/fixed_array.h>

#ifdef TURBO_HAVE_EXCEPTIONS

#include <initializer_list>

#include <gtest/gtest.h>
#include <tests/base/exception_safety_testing.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

constexpr size_t kInlined = 25;
constexpr size_t kSmallSize = kInlined / 2;
constexpr size_t kLargeSize = kInlined * 2;

constexpr int kInitialValue = 5;
constexpr int kUpdatedValue = 10;

using ::testing::TestThrowingCtor;

using Thrower = testing::ThrowingValue<testing::TypeSpec::kEverythingThrows>;
using ThrowAlloc =
    testing::ThrowingAllocator<Thrower, testing::AllocSpec::kEverythingThrows>;
using MoveThrower = testing::ThrowingValue<testing::TypeSpec::kNoThrowMove>;
using MoveThrowAlloc =
    testing::ThrowingAllocator<MoveThrower,
                               testing::AllocSpec::kEverythingThrows>;

using FixedArr = turbo::FixedArray<Thrower, kInlined>;
using FixedArrWithAlloc = turbo::FixedArray<Thrower, kInlined, ThrowAlloc>;

using MoveFixedArr = turbo::FixedArray<MoveThrower, kInlined>;
using MoveFixedArrWithAlloc =
    turbo::FixedArray<MoveThrower, kInlined, MoveThrowAlloc>;

TEST(FixedArrayExceptionSafety, CopyConstructor) {
  auto small = FixedArr(kSmallSize);
  TestThrowingCtor<FixedArr>(small);

  auto large = FixedArr(kLargeSize);
  TestThrowingCtor<FixedArr>(large);
}

TEST(FixedArrayExceptionSafety, CopyConstructorWithAlloc) {
  auto small = FixedArrWithAlloc(kSmallSize);
  TestThrowingCtor<FixedArrWithAlloc>(small);

  auto large = FixedArrWithAlloc(kLargeSize);
  TestThrowingCtor<FixedArrWithAlloc>(large);
}

TEST(FixedArrayExceptionSafety, MoveConstructor) {
  TestThrowingCtor<FixedArr>(FixedArr(kSmallSize));
  TestThrowingCtor<FixedArr>(FixedArr(kLargeSize));

  // TypeSpec::kNoThrowMove
  TestThrowingCtor<MoveFixedArr>(MoveFixedArr(kSmallSize));
  TestThrowingCtor<MoveFixedArr>(MoveFixedArr(kLargeSize));
}

TEST(FixedArrayExceptionSafety, MoveConstructorWithAlloc) {
  TestThrowingCtor<FixedArrWithAlloc>(FixedArrWithAlloc(kSmallSize));
  TestThrowingCtor<FixedArrWithAlloc>(FixedArrWithAlloc(kLargeSize));

  // TypeSpec::kNoThrowMove
  TestThrowingCtor<MoveFixedArrWithAlloc>(MoveFixedArrWithAlloc(kSmallSize));
  TestThrowingCtor<MoveFixedArrWithAlloc>(MoveFixedArrWithAlloc(kLargeSize));
}

TEST(FixedArrayExceptionSafety, SizeConstructor) {
  TestThrowingCtor<FixedArr>(kSmallSize);
  TestThrowingCtor<FixedArr>(kLargeSize);
}

TEST(FixedArrayExceptionSafety, SizeConstructorWithAlloc) {
  TestThrowingCtor<FixedArrWithAlloc>(kSmallSize);
  TestThrowingCtor<FixedArrWithAlloc>(kLargeSize);
}

TEST(FixedArrayExceptionSafety, SizeValueConstructor) {
  TestThrowingCtor<FixedArr>(kSmallSize, Thrower());
  TestThrowingCtor<FixedArr>(kLargeSize, Thrower());
}

TEST(FixedArrayExceptionSafety, SizeValueConstructorWithAlloc) {
  TestThrowingCtor<FixedArrWithAlloc>(kSmallSize, Thrower());
  TestThrowingCtor<FixedArrWithAlloc>(kLargeSize, Thrower());
}

TEST(FixedArrayExceptionSafety, IteratorConstructor) {
  auto small = FixedArr(kSmallSize);
  TestThrowingCtor<FixedArr>(small.begin(), small.end());

  auto large = FixedArr(kLargeSize);
  TestThrowingCtor<FixedArr>(large.begin(), large.end());
}

TEST(FixedArrayExceptionSafety, IteratorConstructorWithAlloc) {
  auto small = FixedArrWithAlloc(kSmallSize);
  TestThrowingCtor<FixedArrWithAlloc>(small.begin(), small.end());

  auto large = FixedArrWithAlloc(kLargeSize);
  TestThrowingCtor<FixedArrWithAlloc>(large.begin(), large.end());
}

TEST(FixedArrayExceptionSafety, InitListConstructor) {
  constexpr int small_inlined = 3;
  using SmallFixedArr = turbo::FixedArray<Thrower, small_inlined>;

  TestThrowingCtor<SmallFixedArr>(std::initializer_list<Thrower>{});
  // Test inlined allocation
  TestThrowingCtor<SmallFixedArr>(
      std::initializer_list<Thrower>{Thrower{}, Thrower{}});
  // Test out of line allocation
  TestThrowingCtor<SmallFixedArr>(std::initializer_list<Thrower>{
      Thrower{}, Thrower{}, Thrower{}, Thrower{}, Thrower{}});
}

TEST(FixedArrayExceptionSafety, InitListConstructorWithAlloc) {
  constexpr int small_inlined = 3;
  using SmallFixedArrWithAlloc =
      turbo::FixedArray<Thrower, small_inlined, ThrowAlloc>;

  TestThrowingCtor<SmallFixedArrWithAlloc>(std::initializer_list<Thrower>{});
  // Test inlined allocation
  TestThrowingCtor<SmallFixedArrWithAlloc>(
      std::initializer_list<Thrower>{Thrower{}, Thrower{}});
  // Test out of line allocation
  TestThrowingCtor<SmallFixedArrWithAlloc>(std::initializer_list<Thrower>{
      Thrower{}, Thrower{}, Thrower{}, Thrower{}, Thrower{}});
}

template <typename FixedArrT>
testing::AssertionResult ReadMemory(FixedArrT* fixed_arr) {
  int sum = 0;
  for (const auto& thrower : *fixed_arr) {
    sum += thrower.Get();
  }
  return testing::AssertionSuccess() << "Values sum to [" << sum << "]";
}

TEST(FixedArrayExceptionSafety, Fill) {
  auto test_fill = testing::MakeExceptionSafetyTester()
                       .WithContracts(ReadMemory<FixedArr>)
                       .WithOperation([&](FixedArr* fixed_arr_ptr) {
                         auto thrower =
                             Thrower(kUpdatedValue, testing::nothrow_ctor);
                         fixed_arr_ptr->fill(thrower);
                       });

  EXPECT_TRUE(
      test_fill.WithInitialValue(FixedArr(kSmallSize, Thrower(kInitialValue)))
          .Test());
  EXPECT_TRUE(
      test_fill.WithInitialValue(FixedArr(kLargeSize, Thrower(kInitialValue)))
          .Test());
}

TEST(FixedArrayExceptionSafety, FillWithAlloc) {
  auto test_fill = testing::MakeExceptionSafetyTester()
                       .WithContracts(ReadMemory<FixedArrWithAlloc>)
                       .WithOperation([&](FixedArrWithAlloc* fixed_arr_ptr) {
                         auto thrower =
                             Thrower(kUpdatedValue, testing::nothrow_ctor);
                         fixed_arr_ptr->fill(thrower);
                       });

  EXPECT_TRUE(test_fill
                  .WithInitialValue(
                      FixedArrWithAlloc(kSmallSize, Thrower(kInitialValue)))
                  .Test());
  EXPECT_TRUE(test_fill
                  .WithInitialValue(
                      FixedArrWithAlloc(kLargeSize, Thrower(kInitialValue)))
                  .Test());
}

}  // namespace

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_HAVE_EXCEPTIONS
