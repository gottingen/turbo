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

#include <turbo/types/optional.h>

#include <turbo/base/config.h>

// This test is a no-op when turbo::optional is an alias for std::optional and
// when exceptions are not enabled.
#if !defined(TURBO_USES_STD_OPTIONAL) && defined(TURBO_HAVE_EXCEPTIONS)

#include <gtest/gtest.h>
#include <tests/base/exception_safety_testing.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using ::testing::MakeExceptionSafetyTester;

using Thrower = testing::ThrowingValue<testing::TypeSpec::kEverythingThrows>;
using Optional = turbo::optional<Thrower>;

using MoveThrower = testing::ThrowingValue<testing::TypeSpec::kNoThrowMove>;
using MoveOptional = turbo::optional<MoveThrower>;

constexpr int kInitialInteger = 5;
constexpr int kUpdatedInteger = 10;

template <typename OptionalT>
bool ValueThrowsBadOptionalAccess(const OptionalT& optional) try {
  return (static_cast<void>(optional.value()), false);
} catch (const turbo::bad_optional_access&) {
  return true;
}

template <typename OptionalT>
AssertionResult OptionalInvariants(OptionalT* optional_ptr) {
  // Check the current state post-throw for validity
  auto& optional = *optional_ptr;

  if (optional.has_value() && ValueThrowsBadOptionalAccess(optional)) {
    return AssertionFailure()
           << "Optional with value should not throw bad_optional_access when "
              "accessing the value.";
  }
  if (!optional.has_value() && !ValueThrowsBadOptionalAccess(optional)) {
    return AssertionFailure()
           << "Optional without a value should throw bad_optional_access when "
              "accessing the value.";
  }

  // Reset to a known state
  optional.reset();

  // Confirm that the known post-reset state is valid
  if (optional.has_value()) {
    return AssertionFailure()
           << "Optional should not contain a value after being reset.";
  }
  if (!ValueThrowsBadOptionalAccess(optional)) {
    return AssertionFailure() << "Optional should throw bad_optional_access "
                                 "when accessing the value after being reset.";
  }

  return AssertionSuccess();
}

template <typename OptionalT>
AssertionResult CheckDisengaged(OptionalT* optional_ptr) {
  auto& optional = *optional_ptr;

  if (optional.has_value()) {
    return AssertionFailure()
           << "Expected optional to not contain a value but a value was found.";
  }

  return AssertionSuccess();
}

template <typename OptionalT>
AssertionResult CheckEngaged(OptionalT* optional_ptr) {
  auto& optional = *optional_ptr;

  if (!optional.has_value()) {
    return AssertionFailure()
           << "Expected optional to contain a value but no value was found.";
  }

  return AssertionSuccess();
}

TEST(OptionalExceptionSafety, ThrowingConstructors) {
  auto thrower_nonempty = Optional(Thrower(kInitialInteger));
  testing::TestThrowingCtor<Optional>(thrower_nonempty);

  auto integer_nonempty = turbo::optional<int>(kInitialInteger);
  testing::TestThrowingCtor<Optional>(integer_nonempty);
  testing::TestThrowingCtor<Optional>(std::move(integer_nonempty));  // NOLINT

  testing::TestThrowingCtor<Optional>(kInitialInteger);
  using ThrowerVec = std::vector<Thrower, testing::ThrowingAllocator<Thrower>>;
  testing::TestThrowingCtor<turbo::optional<ThrowerVec>>(
      turbo::in_place,
      std::initializer_list<Thrower>{Thrower(), Thrower(), Thrower()},
      testing::ThrowingAllocator<Thrower>());
}

TEST(OptionalExceptionSafety, NothrowConstructors) {
  // This constructor is marked noexcept. If it throws, the program will
  // terminate.
  testing::TestThrowingCtor<MoveOptional>(MoveOptional(kUpdatedInteger));
}

TEST(OptionalExceptionSafety, Emplace) {
  // Test the basic guarantee plus test the result of optional::has_value()
  // is false in all cases
  auto disengaged_test = MakeExceptionSafetyTester().WithContracts(
      OptionalInvariants<Optional>, CheckDisengaged<Optional>);
  auto disengaged_test_empty = disengaged_test.WithInitialValue(Optional());
  auto disengaged_test_nonempty =
      disengaged_test.WithInitialValue(Optional(kInitialInteger));

  auto emplace_thrower_directly = [](Optional* optional_ptr) {
    optional_ptr->emplace(kUpdatedInteger);
  };
  EXPECT_TRUE(disengaged_test_empty.Test(emplace_thrower_directly));
  EXPECT_TRUE(disengaged_test_nonempty.Test(emplace_thrower_directly));

  auto emplace_thrower_copy = [](Optional* optional_ptr) {
    auto thrower = Thrower(kUpdatedInteger, testing::nothrow_ctor);
    optional_ptr->emplace(thrower);
  };
  EXPECT_TRUE(disengaged_test_empty.Test(emplace_thrower_copy));
  EXPECT_TRUE(disengaged_test_nonempty.Test(emplace_thrower_copy));
}

TEST(OptionalExceptionSafety, EverythingThrowsSwap) {
  // Test the basic guarantee plus test the result of optional::has_value()
  // remains the same
  auto test =
      MakeExceptionSafetyTester().WithContracts(OptionalInvariants<Optional>);
  auto disengaged_test_empty = test.WithInitialValue(Optional())
                                   .WithContracts(CheckDisengaged<Optional>);
  auto engaged_test_nonempty = test.WithInitialValue(Optional(kInitialInteger))
                                   .WithContracts(CheckEngaged<Optional>);

  auto swap_empty = [](Optional* optional_ptr) {
    auto empty = Optional();
    optional_ptr->swap(empty);
  };
  EXPECT_TRUE(engaged_test_nonempty.Test(swap_empty));

  auto swap_nonempty = [](Optional* optional_ptr) {
    auto nonempty =
        Optional(turbo::in_place, kUpdatedInteger, testing::nothrow_ctor);
    optional_ptr->swap(nonempty);
  };
  EXPECT_TRUE(disengaged_test_empty.Test(swap_nonempty));
  EXPECT_TRUE(engaged_test_nonempty.Test(swap_nonempty));
}

TEST(OptionalExceptionSafety, NoThrowMoveSwap) {
  // Tests the nothrow guarantee for optional of T with non-throwing move
  {
    auto empty = MoveOptional();
    auto nonempty = MoveOptional(kInitialInteger);
    EXPECT_TRUE(testing::TestNothrowOp([&]() { nonempty.swap(empty); }));
  }
  {
    auto nonempty = MoveOptional(kUpdatedInteger);
    auto empty = MoveOptional();
    EXPECT_TRUE(testing::TestNothrowOp([&]() { empty.swap(nonempty); }));
  }
  {
    auto nonempty_from = MoveOptional(kUpdatedInteger);
    auto nonempty_to = MoveOptional(kInitialInteger);
    EXPECT_TRUE(
        testing::TestNothrowOp([&]() { nonempty_to.swap(nonempty_from); }));
  }
}

TEST(OptionalExceptionSafety, CopyAssign) {
  // Test the basic guarantee plus test the result of optional::has_value()
  // remains the same
  auto test =
      MakeExceptionSafetyTester().WithContracts(OptionalInvariants<Optional>);
  auto disengaged_test_empty = test.WithInitialValue(Optional())
                                   .WithContracts(CheckDisengaged<Optional>);
  auto engaged_test_nonempty = test.WithInitialValue(Optional(kInitialInteger))
                                   .WithContracts(CheckEngaged<Optional>);

  auto copyassign_nonempty = [](Optional* optional_ptr) {
    auto nonempty =
        Optional(turbo::in_place, kUpdatedInteger, testing::nothrow_ctor);
    *optional_ptr = nonempty;
  };
  EXPECT_TRUE(disengaged_test_empty.Test(copyassign_nonempty));
  EXPECT_TRUE(engaged_test_nonempty.Test(copyassign_nonempty));

  auto copyassign_thrower = [](Optional* optional_ptr) {
    auto thrower = Thrower(kUpdatedInteger, testing::nothrow_ctor);
    *optional_ptr = thrower;
  };
  EXPECT_TRUE(disengaged_test_empty.Test(copyassign_thrower));
  EXPECT_TRUE(engaged_test_nonempty.Test(copyassign_thrower));
}

TEST(OptionalExceptionSafety, MoveAssign) {
  // Test the basic guarantee plus test the result of optional::has_value()
  // remains the same
  auto test =
      MakeExceptionSafetyTester().WithContracts(OptionalInvariants<Optional>);
  auto disengaged_test_empty = test.WithInitialValue(Optional())
                                   .WithContracts(CheckDisengaged<Optional>);
  auto engaged_test_nonempty = test.WithInitialValue(Optional(kInitialInteger))
                                   .WithContracts(CheckEngaged<Optional>);

  auto moveassign_empty = [](Optional* optional_ptr) {
    auto empty = Optional();
    *optional_ptr = std::move(empty);
  };
  EXPECT_TRUE(engaged_test_nonempty.Test(moveassign_empty));

  auto moveassign_nonempty = [](Optional* optional_ptr) {
    auto nonempty =
        Optional(turbo::in_place, kUpdatedInteger, testing::nothrow_ctor);
    *optional_ptr = std::move(nonempty);
  };
  EXPECT_TRUE(disengaged_test_empty.Test(moveassign_nonempty));
  EXPECT_TRUE(engaged_test_nonempty.Test(moveassign_nonempty));

  auto moveassign_thrower = [](Optional* optional_ptr) {
    auto thrower = Thrower(kUpdatedInteger, testing::nothrow_ctor);
    *optional_ptr = std::move(thrower);
  };
  EXPECT_TRUE(disengaged_test_empty.Test(moveassign_thrower));
  EXPECT_TRUE(engaged_test_nonempty.Test(moveassign_thrower));
}

TEST(OptionalExceptionSafety, NothrowMoveAssign) {
  // Tests the nothrow guarantee for optional of T with non-throwing move
  {
    auto empty = MoveOptional();
    auto nonempty = MoveOptional(kInitialInteger);
    EXPECT_TRUE(testing::TestNothrowOp([&]() { nonempty = std::move(empty); }));
  }
  {
    auto nonempty = MoveOptional(kInitialInteger);
    auto empty = MoveOptional();
    EXPECT_TRUE(testing::TestNothrowOp([&]() { empty = std::move(nonempty); }));
  }
  {
    auto nonempty_from = MoveOptional(kUpdatedInteger);
    auto nonempty_to = MoveOptional(kInitialInteger);
    EXPECT_TRUE(testing::TestNothrowOp(
        [&]() { nonempty_to = std::move(nonempty_from); }));
  }
  {
    auto thrower = MoveThrower(kUpdatedInteger);
    auto empty = MoveOptional();
    EXPECT_TRUE(testing::TestNothrowOp([&]() { empty = std::move(thrower); }));
  }
  {
    auto thrower = MoveThrower(kUpdatedInteger);
    auto nonempty = MoveOptional(kInitialInteger);
    EXPECT_TRUE(
        testing::TestNothrowOp([&]() { nonempty = std::move(thrower); }));
  }
}

}  // namespace

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // #if !defined(TURBO_USES_STD_OPTIONAL) && defined(TURBO_HAVE_EXCEPTIONS)
