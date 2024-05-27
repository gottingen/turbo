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

#include <turbo/types/variant.h>

#include <turbo/base/config.h>

// This test is a no-op when turbo::variant is an alias for std::variant and when
// exceptions are not enabled.
#if !defined(TURBO_USES_STD_VARIANT) && defined(TURBO_HAVE_EXCEPTIONS)

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/base/exception_safety_testing.h>
#include <turbo/memory/memory.h>

// See comment in turbo/base/config.h
#if !defined(TURBO_INTERNAL_MSVC_2017_DBG_MODE)

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

using ::testing::MakeExceptionSafetyTester;
using ::testing::strong_guarantee;
using ::testing::TestNothrowOp;
using ::testing::TestThrowingCtor;

using Thrower = testing::ThrowingValue<>;
using CopyNothrow = testing::ThrowingValue<testing::TypeSpec::kNoThrowCopy>;
using MoveNothrow = testing::ThrowingValue<testing::TypeSpec::kNoThrowMove>;
using ThrowingAlloc = testing::ThrowingAllocator<Thrower>;
using ThrowerVec = std::vector<Thrower, ThrowingAlloc>;
using ThrowingVariant =
    turbo::variant<Thrower, CopyNothrow, MoveNothrow, ThrowerVec>;

struct ConversionException {};

template <class T>
struct ExceptionOnConversion {
  operator T() const {  // NOLINT
    throw ConversionException();
  }
};

// Forces a variant into the valueless by exception state.
void ToValuelessByException(ThrowingVariant& v) {  // NOLINT
  try {
    v.emplace<Thrower>();
    v.emplace<Thrower>(ExceptionOnConversion<Thrower>());
  } catch (const ConversionException&) {
    // This space intentionally left blank.
  }
}

// Check that variant is still in a usable state after an exception is thrown.
testing::AssertionResult VariantInvariants(ThrowingVariant* v) {
  using testing::AssertionFailure;
  using testing::AssertionSuccess;

  // Try using the active alternative
  if (turbo::holds_alternative<Thrower>(*v)) {
    auto& t = turbo::get<Thrower>(*v);
    t = Thrower{-100};
    if (t.Get() != -100) {
      return AssertionFailure() << "Thrower should be assigned -100";
    }
  } else if (turbo::holds_alternative<ThrowerVec>(*v)) {
    auto& tv = turbo::get<ThrowerVec>(*v);
    tv.clear();
    tv.emplace_back(-100);
    if (tv.size() != 1 || tv[0].Get() != -100) {
      return AssertionFailure() << "ThrowerVec should be {Thrower{-100}}";
    }
  } else if (turbo::holds_alternative<CopyNothrow>(*v)) {
    auto& t = turbo::get<CopyNothrow>(*v);
    t = CopyNothrow{-100};
    if (t.Get() != -100) {
      return AssertionFailure() << "CopyNothrow should be assigned -100";
    }
  } else if (turbo::holds_alternative<MoveNothrow>(*v)) {
    auto& t = turbo::get<MoveNothrow>(*v);
    t = MoveNothrow{-100};
    if (t.Get() != -100) {
      return AssertionFailure() << "MoveNothrow should be assigned -100";
    }
  }

  // Try making variant valueless_by_exception
  if (!v->valueless_by_exception()) ToValuelessByException(*v);
  if (!v->valueless_by_exception()) {
    return AssertionFailure() << "Variant should be valueless_by_exception";
  }
  try {
    auto unused = turbo::get<Thrower>(*v);
    static_cast<void>(unused);
    return AssertionFailure() << "Variant should not contain Thrower";
  } catch (const turbo::bad_variant_access&) {
  } catch (...) {
    return AssertionFailure() << "Unexpected exception throw from turbo::get";
  }

  // Try using the variant
  v->emplace<Thrower>(100);
  if (!turbo::holds_alternative<Thrower>(*v) ||
      turbo::get<Thrower>(*v) != Thrower(100)) {
    return AssertionFailure() << "Variant should contain Thrower(100)";
  }
  v->emplace<ThrowerVec>({Thrower(100)});
  if (!turbo::holds_alternative<ThrowerVec>(*v) ||
      turbo::get<ThrowerVec>(*v)[0] != Thrower(100)) {
    return AssertionFailure()
           << "Variant should contain ThrowerVec{Thrower(100)}";
  }
  return AssertionSuccess();
}

template <typename... Args>
Thrower ExpectedThrower(Args&&... args) {
  return Thrower(42, args...);
}

ThrowerVec ExpectedThrowerVec() { return {Thrower(100), Thrower(200)}; }
ThrowingVariant ValuelessByException() {
  ThrowingVariant v;
  ToValuelessByException(v);
  return v;
}
ThrowingVariant WithThrower() { return Thrower(39); }
ThrowingVariant WithThrowerVec() {
  return ThrowerVec{Thrower(1), Thrower(2), Thrower(3)};
}
ThrowingVariant WithCopyNoThrow() { return CopyNothrow(39); }
ThrowingVariant WithMoveNoThrow() { return MoveNothrow(39); }

TEST(VariantExceptionSafetyTest, DefaultConstructor) {
  TestThrowingCtor<ThrowingVariant>();
}

TEST(VariantExceptionSafetyTest, CopyConstructor) {
  {
    ThrowingVariant v(ExpectedThrower());
    TestThrowingCtor<ThrowingVariant>(v);
  }
  {
    ThrowingVariant v(ExpectedThrowerVec());
    TestThrowingCtor<ThrowingVariant>(v);
  }
  {
    ThrowingVariant v(ValuelessByException());
    TestThrowingCtor<ThrowingVariant>(v);
  }
}

TEST(VariantExceptionSafetyTest, MoveConstructor) {
  {
    ThrowingVariant v(ExpectedThrower());
    TestThrowingCtor<ThrowingVariant>(std::move(v));
  }
  {
    ThrowingVariant v(ExpectedThrowerVec());
    TestThrowingCtor<ThrowingVariant>(std::move(v));
  }
  {
    ThrowingVariant v(ValuelessByException());
    TestThrowingCtor<ThrowingVariant>(std::move(v));
  }
}

TEST(VariantExceptionSafetyTest, ValueConstructor) {
  TestThrowingCtor<ThrowingVariant>(ExpectedThrower());
  TestThrowingCtor<ThrowingVariant>(ExpectedThrowerVec());
}

TEST(VariantExceptionSafetyTest, InPlaceTypeConstructor) {
  TestThrowingCtor<ThrowingVariant>(turbo::in_place_type_t<Thrower>{},
                                    ExpectedThrower());
  TestThrowingCtor<ThrowingVariant>(turbo::in_place_type_t<ThrowerVec>{},
                                    ExpectedThrowerVec());
}

TEST(VariantExceptionSafetyTest, InPlaceIndexConstructor) {
  TestThrowingCtor<ThrowingVariant>(turbo::in_place_index_t<0>{},
                                    ExpectedThrower());
  TestThrowingCtor<ThrowingVariant>(turbo::in_place_index_t<3>{},
                                    ExpectedThrowerVec());
}

TEST(VariantExceptionSafetyTest, CopyAssign) {
  // variant& operator=(const variant& rhs);
  // Let j be rhs.index()
  {
    // - neither *this nor rhs holds a value
    const ThrowingVariant rhs = ValuelessByException();
    ThrowingVariant lhs = ValuelessByException();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = rhs; }));
  }
  {
    // - *this holds a value but rhs does not
    const ThrowingVariant rhs = ValuelessByException();
    ThrowingVariant lhs = WithThrower();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = rhs; }));
  }
  // - index() == j
  {
    const ThrowingVariant rhs(ExpectedThrower());
    auto tester =
        MakeExceptionSafetyTester()
            .WithInitialValue(WithThrower())
            .WithOperation([&rhs](ThrowingVariant* lhs) { *lhs = rhs; });
    EXPECT_TRUE(tester.WithContracts(VariantInvariants).Test());
    EXPECT_FALSE(tester.WithContracts(strong_guarantee).Test());
  }
  {
    const ThrowingVariant rhs(ExpectedThrowerVec());
    auto tester =
        MakeExceptionSafetyTester()
            .WithInitialValue(WithThrowerVec())
            .WithOperation([&rhs](ThrowingVariant* lhs) { *lhs = rhs; });
    EXPECT_TRUE(tester.WithContracts(VariantInvariants).Test());
    EXPECT_FALSE(tester.WithContracts(strong_guarantee).Test());
  }
  // libstdc++ std::variant has bugs on copy assignment regarding exception
  // safety.
#if !(defined(TURBO_USES_STD_VARIANT) && defined(__GLIBCXX__))
  // index() != j
  // if is_nothrow_copy_constructible_v<Tj> or
  // !is_nothrow_move_constructible<Tj> is true, equivalent to
  // emplace<j>(get<j>(rhs))
  {
    // is_nothrow_copy_constructible_v<Tj> == true
    // should not throw because emplace() invokes Tj's copy ctor
    // which should not throw.
    const ThrowingVariant rhs(CopyNothrow{});
    ThrowingVariant lhs = WithThrower();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = rhs; }));
  }
  {
    // is_nothrow_copy_constructible<Tj> == false &&
    // is_nothrow_move_constructible<Tj> == false
    // should provide basic guarantee because emplace() invokes Tj's copy ctor
    // which may throw.
    const ThrowingVariant rhs(ExpectedThrower());
    auto tester =
        MakeExceptionSafetyTester()
            .WithInitialValue(WithCopyNoThrow())
            .WithOperation([&rhs](ThrowingVariant* lhs) { *lhs = rhs; });
    EXPECT_TRUE(tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return lhs->valueless_by_exception();
                                   })
                    .Test());
    EXPECT_FALSE(tester.WithContracts(strong_guarantee).Test());
  }
#endif  // !(defined(TURBO_USES_STD_VARIANT) && defined(__GLIBCXX__))
  {
    // is_nothrow_copy_constructible_v<Tj> == false &&
    // is_nothrow_move_constructible_v<Tj> == true
    // should provide strong guarantee because it is equivalent to
    // operator=(variant(rhs)) which creates a temporary then invoke the move
    // ctor which shouldn't throw.
    const ThrowingVariant rhs(MoveNothrow{});
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithThrower())
                    .WithContracts(VariantInvariants, strong_guarantee)
                    .Test([&rhs](ThrowingVariant* lhs) { *lhs = rhs; }));
  }
}

TEST(VariantExceptionSafetyTest, MoveAssign) {
  // variant& operator=(variant&& rhs);
  // Let j be rhs.index()
  {
    // - neither *this nor rhs holds a value
    ThrowingVariant rhs = ValuelessByException();
    ThrowingVariant lhs = ValuelessByException();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = std::move(rhs); }));
  }
  {
    // - *this holds a value but rhs does not
    ThrowingVariant rhs = ValuelessByException();
    ThrowingVariant lhs = WithThrower();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = std::move(rhs); }));
  }
  {
    // - index() == j
    // assign get<j>(std::move(rhs)) to the value contained in *this.
    // If an exception is thrown during call to Tj's move assignment, the state
    // of the contained value is as defined by the exception safety guarantee of
    // Tj's move assignment; index() will be j.
    ThrowingVariant rhs(ExpectedThrower());
    size_t j = rhs.index();
    // Since Thrower's move assignment has basic guarantee, so should variant's.
    auto tester = MakeExceptionSafetyTester()
                      .WithInitialValue(WithThrower())
                      .WithOperation([&](ThrowingVariant* lhs) {
                        auto copy = rhs;
                        *lhs = std::move(copy);
                      });
    EXPECT_TRUE(tester
                    .WithContracts(
                        VariantInvariants,
                        [&](ThrowingVariant* lhs) { return lhs->index() == j; })
                    .Test());
    EXPECT_FALSE(tester.WithContracts(strong_guarantee).Test());
  }
  {
    // libstdc++ introduced a regression between 2018-09-25 and 2019-01-06.
    // The fix is targeted for gcc-9.
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87431#c7
    // https://gcc.gnu.org/viewcvs/gcc?view=revision&revision=267614
#if !(defined(TURBO_USES_STD_VARIANT) && \
      defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 8)
    // - otherwise (index() != j), equivalent to
    // emplace<j>(get<j>(std::move(rhs)))
    // - If an exception is thrown during the call to Tj's move construction
    // (with j being rhs.index()), the variant will hold no value.
    ThrowingVariant rhs(CopyNothrow{});
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithThrower())
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return lhs->valueless_by_exception();
                                   })
                    .Test([&](ThrowingVariant* lhs) {
                      auto copy = rhs;
                      *lhs = std::move(copy);
                    }));
#endif  // !(defined(TURBO_USES_STD_VARIANT) &&
        //   defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 8)
  }
}

TEST(VariantExceptionSafetyTest, ValueAssign) {
  // template<class T> variant& operator=(T&& t);
  // Let Tj be the type that is selected by overload resolution to be assigned.
  {
    // If *this holds a Tj, assigns std::forward<T>(t) to the value contained in
    // *this. If  an exception is thrown during the assignment of
    // std::forward<T>(t) to the value contained in *this, the state of the
    // contained value and t are as defined by the exception safety guarantee of
    // the assignment expression; valueless_by_exception() will be false.
    // Since Thrower's copy/move assignment has basic guarantee, so should
    // variant's.
    Thrower rhs = ExpectedThrower();
    // copy assign
    auto copy_tester =
        MakeExceptionSafetyTester()
            .WithInitialValue(WithThrower())
            .WithOperation([rhs](ThrowingVariant* lhs) { *lhs = rhs; });
    EXPECT_TRUE(copy_tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return !lhs->valueless_by_exception();
                                   })
                    .Test());
    EXPECT_FALSE(copy_tester.WithContracts(strong_guarantee).Test());
    // move assign
    auto move_tester = MakeExceptionSafetyTester()
                           .WithInitialValue(WithThrower())
                           .WithOperation([&](ThrowingVariant* lhs) {
                             auto copy = rhs;
                             *lhs = std::move(copy);
                           });
    EXPECT_TRUE(move_tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return !lhs->valueless_by_exception();
                                   })
                    .Test());

    EXPECT_FALSE(move_tester.WithContracts(strong_guarantee).Test());
  }
  // Otherwise (*this holds something else), if is_nothrow_constructible_v<Tj,
  // T> || !is_nothrow_move_constructible_v<Tj> is true, equivalent to
  // emplace<j>(std::forward<T>(t)).
  // We simplify the test by letting T = `const Tj&`  or `Tj&&`, so we can reuse
  // the CopyNothrow and MoveNothrow types.

  // if is_nothrow_constructible_v<Tj, T>
  // (i.e. is_nothrow_copy/move_constructible_v<Tj>) is true, emplace() just
  // invokes the copy/move constructor and it should not throw.
  {
    const CopyNothrow rhs;
    ThrowingVariant lhs = WithThrower();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = rhs; }));
  }
  {
    MoveNothrow rhs;
    ThrowingVariant lhs = WithThrower();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs = std::move(rhs); }));
  }
  // if is_nothrow_constructible_v<Tj, T> == false &&
  // is_nothrow_move_constructible<Tj> == false
  // emplace() invokes the copy/move constructor which may throw so it should
  // provide basic guarantee and variant object might not hold a value.
  {
    Thrower rhs = ExpectedThrower();
    // copy
    auto copy_tester =
        MakeExceptionSafetyTester()
            .WithInitialValue(WithCopyNoThrow())
            .WithOperation([&rhs](ThrowingVariant* lhs) { *lhs = rhs; });
    EXPECT_TRUE(copy_tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return lhs->valueless_by_exception();
                                   })
                    .Test());
    EXPECT_FALSE(copy_tester.WithContracts(strong_guarantee).Test());
    // move
    auto move_tester = MakeExceptionSafetyTester()
                           .WithInitialValue(WithCopyNoThrow())
                           .WithOperation([](ThrowingVariant* lhs) {
                             *lhs = ExpectedThrower(testing::nothrow_ctor);
                           });
    EXPECT_TRUE(move_tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* lhs) {
                                     return lhs->valueless_by_exception();
                                   })
                    .Test());
    EXPECT_FALSE(move_tester.WithContracts(strong_guarantee).Test());
  }
  // Otherwise (if is_nothrow_constructible_v<Tj, T> == false &&
  // is_nothrow_move_constructible<Tj> == true),
  // equivalent to operator=(variant(std::forward<T>(t)))
  // This should have strong guarantee because it creates a temporary variant
  // and operator=(variant&&) invokes Tj's move ctor which doesn't throw.
  // libstdc++ std::variant has bugs on conversion assignment regarding
  // exception safety.
#if !(defined(TURBO_USES_STD_VARIANT) && defined(__GLIBCXX__))
  {
    MoveNothrow rhs;
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithThrower())
                    .WithContracts(VariantInvariants, strong_guarantee)
                    .Test([&rhs](ThrowingVariant* lhs) { *lhs = rhs; }));
  }
#endif  // !(defined(TURBO_USES_STD_VARIANT) && defined(__GLIBCXX__))
}

TEST(VariantExceptionSafetyTest, Emplace) {
  // If an exception during the initialization of the contained value, the
  // variant might not hold a value. The standard requires emplace() to provide
  // only basic guarantee.
  {
    Thrower args = ExpectedThrower();
    auto tester = MakeExceptionSafetyTester()
                      .WithInitialValue(WithThrower())
                      .WithOperation([&args](ThrowingVariant* v) {
                        v->emplace<Thrower>(args);
                      });
    EXPECT_TRUE(tester
                    .WithContracts(VariantInvariants,
                                   [](ThrowingVariant* v) {
                                     return v->valueless_by_exception();
                                   })
                    .Test());
    EXPECT_FALSE(tester.WithContracts(strong_guarantee).Test());
  }
}

TEST(VariantExceptionSafetyTest, Swap) {
  // if both are valueless_by_exception(), no effect
  {
    ThrowingVariant rhs = ValuelessByException();
    ThrowingVariant lhs = ValuelessByException();
    EXPECT_TRUE(TestNothrowOp([&]() { lhs.swap(rhs); }));
  }
  // if index() == rhs.index(), calls swap(get<i>(*this), get<i>(rhs))
  // where i is index().
  {
    ThrowingVariant rhs = ExpectedThrower();
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithThrower())
                    .WithContracts(VariantInvariants)
                    .Test([&](ThrowingVariant* lhs) {
                      auto copy = rhs;
                      lhs->swap(copy);
                    }));
  }
  // Otherwise, exchanges the value of rhs and *this. The exception safety
  // involves variant in moved-from state which is not specified in the
  // standard, and since swap is 3-step it's impossible for it to provide a
  // overall strong guarantee. So, we are only checking basic guarantee here.
  {
    ThrowingVariant rhs = ExpectedThrower();
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithCopyNoThrow())
                    .WithContracts(VariantInvariants)
                    .Test([&](ThrowingVariant* lhs) {
                      auto copy = rhs;
                      lhs->swap(copy);
                    }));
  }
  {
    ThrowingVariant rhs = ExpectedThrower();
    EXPECT_TRUE(MakeExceptionSafetyTester()
                    .WithInitialValue(WithCopyNoThrow())
                    .WithContracts(VariantInvariants)
                    .Test([&](ThrowingVariant* lhs) {
                      auto copy = rhs;
                      copy.swap(*lhs);
                    }));
  }
}

}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // !defined(TURBO_INTERNAL_MSVC_2017_DBG_MODE)

#endif  // #if !defined(TURBO_USES_STD_VARIANT) && defined(TURBO_HAVE_EXCEPTIONS)
