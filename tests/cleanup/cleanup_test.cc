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

#include <turbo/bootstrap/cleanup.h>

#include <functional>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/utility/utility.h>

namespace {

using Tag = turbo::cleanup_internal::Tag;

template <typename Type1, typename Type2>
constexpr bool IsSame() {
  return (std::is_same<Type1, Type2>::value);
}

struct IdentityFactory {
  template <typename Callback>
  static Callback AsCallback(Callback callback) {
    return Callback(std::move(callback));
  }
};

// `FunctorClass` is a type used for testing `turbo::Cleanup`. It is intended to
// represent users that make their own move-only callback types outside of
// `std::function` and lambda literals.
class FunctorClass {
  using Callback = std::function<void()>;

 public:
  explicit FunctorClass(Callback callback) : callback_(std::move(callback)) {}

  FunctorClass(FunctorClass&& other)
      : callback_(std::exchange(other.callback_, Callback())) {}

  FunctorClass(const FunctorClass&) = delete;

  FunctorClass& operator=(const FunctorClass&) = delete;

  FunctorClass& operator=(FunctorClass&&) = delete;

  void operator()() const& = delete;

  void operator()() && {
    ASSERT_TRUE(callback_);
    callback_();
    callback_ = nullptr;
  }

 private:
  Callback callback_;
};

struct FunctorClassFactory {
  template <typename Callback>
  static FunctorClass AsCallback(Callback callback) {
    return FunctorClass(std::move(callback));
  }
};

struct StdFunctionFactory {
  template <typename Callback>
  static std::function<void()> AsCallback(Callback callback) {
    return std::function<void()>(std::move(callback));
  }
};

using CleanupTestParams =
    ::testing::Types<IdentityFactory, FunctorClassFactory, StdFunctionFactory>;
template <typename>
struct CleanupTest : public ::testing::Test {};
TYPED_TEST_SUITE(CleanupTest, CleanupTestParams);

bool fn_ptr_called = false;
void FnPtrFunction() { fn_ptr_called = true; }

TYPED_TEST(CleanupTest, FactoryProducesCorrectType) {
  {
    auto callback = TypeParam::AsCallback([] {});
    auto cleanup = turbo::MakeCleanup(std::move(callback));

    static_assert(
        IsSame<turbo::Cleanup<Tag, decltype(callback)>, decltype(cleanup)>(),
        "");
  }

  {
    auto cleanup = turbo::MakeCleanup(&FnPtrFunction);

    static_assert(IsSame<turbo::Cleanup<Tag, void (*)()>, decltype(cleanup)>(),
                  "");
  }

  {
    auto cleanup = turbo::MakeCleanup(FnPtrFunction);

    static_assert(IsSame<turbo::Cleanup<Tag, void (*)()>, decltype(cleanup)>(),
                  "");
  }
}

#if defined(TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
TYPED_TEST(CleanupTest, CTADProducesCorrectType) {
  {
    auto callback = TypeParam::AsCallback([] {});
    turbo::Cleanup cleanup = std::move(callback);

    static_assert(
        IsSame<turbo::Cleanup<Tag, decltype(callback)>, decltype(cleanup)>(),
        "");
  }

  {
    turbo::Cleanup cleanup = &FnPtrFunction;

    static_assert(IsSame<turbo::Cleanup<Tag, void (*)()>, decltype(cleanup)>(),
                  "");
  }

  {
    turbo::Cleanup cleanup = FnPtrFunction;

    static_assert(IsSame<turbo::Cleanup<Tag, void (*)()>, decltype(cleanup)>(),
                  "");
  }
}

TYPED_TEST(CleanupTest, FactoryAndCTADProduceSameType) {
  {
    auto callback = IdentityFactory::AsCallback([] {});
    auto factory_cleanup = turbo::MakeCleanup(callback);
    turbo::Cleanup deduction_cleanup = callback;

    static_assert(
        IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
  }

  {
    auto factory_cleanup =
        turbo::MakeCleanup(FunctorClassFactory::AsCallback([] {}));
    turbo::Cleanup deduction_cleanup = FunctorClassFactory::AsCallback([] {});

    static_assert(
        IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
  }

  {
    auto factory_cleanup =
        turbo::MakeCleanup(StdFunctionFactory::AsCallback([] {}));
    turbo::Cleanup deduction_cleanup = StdFunctionFactory::AsCallback([] {});

    static_assert(
        IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
  }

  {
    auto factory_cleanup = turbo::MakeCleanup(&FnPtrFunction);
    turbo::Cleanup deduction_cleanup = &FnPtrFunction;

    static_assert(
        IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
  }

  {
    auto factory_cleanup = turbo::MakeCleanup(FnPtrFunction);
    turbo::Cleanup deduction_cleanup = FnPtrFunction;

    static_assert(
        IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
  }
}
#endif  // defined(TURBO_HAVE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)

TYPED_TEST(CleanupTest, BasicUsage) {
  bool called = false;

  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback([&called] { called = true; }));
    EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback
  }

  EXPECT_TRUE(called);  // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, BasicUsageWithFunctionPointer) {
  fn_ptr_called = false;

  {
    auto cleanup = turbo::MakeCleanup(TypeParam::AsCallback(&FnPtrFunction));
    EXPECT_FALSE(fn_ptr_called);  // Constructor shouldn't invoke the callback
  }

  EXPECT_TRUE(fn_ptr_called);  // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, Cancel) {
  bool called = false;

  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback([&called] { called = true; }));
    EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

    std::move(cleanup).Cancel();
    EXPECT_FALSE(called);  // Cancel shouldn't invoke the callback
  }

  EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, Invoke) {
  bool called = false;

  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback([&called] { called = true; }));
    EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

    std::move(cleanup).Invoke();
    EXPECT_TRUE(called);  // Invoke should invoke the callback

    called = false;  // Reset tracker before destructor runs
  }

  EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, Move) {
  bool called = false;

  {
    auto moved_from_cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback([&called] { called = true; }));
    EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

    {
      auto moved_to_cleanup = std::move(moved_from_cleanup);
      EXPECT_FALSE(called);  // Move shouldn't invoke the callback
    }

    EXPECT_TRUE(called);  // Destructor should invoke the callback

    called = false;  // Reset tracker before destructor runs
  }

  EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

int DestructionCount = 0;

struct DestructionCounter {
  void operator()() {}

  ~DestructionCounter() { ++DestructionCount; }
};

TYPED_TEST(CleanupTest, DestructorDestroys) {
  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback(DestructionCounter()));
    DestructionCount = 0;
  }

  EXPECT_EQ(DestructionCount, 1);  // Engaged cleanup destroys
}

TYPED_TEST(CleanupTest, CancelDestroys) {
  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback(DestructionCounter()));
    DestructionCount = 0;

    std::move(cleanup).Cancel();
    EXPECT_EQ(DestructionCount, 1);  // Cancel destroys
  }

  EXPECT_EQ(DestructionCount, 1);  // Canceled cleanup does not double destroy
}

TYPED_TEST(CleanupTest, InvokeDestroys) {
  {
    auto cleanup =
        turbo::MakeCleanup(TypeParam::AsCallback(DestructionCounter()));
    DestructionCount = 0;

    std::move(cleanup).Invoke();
    EXPECT_EQ(DestructionCount, 1);  // Invoke destroys
  }

  EXPECT_EQ(DestructionCount, 1);  // Invoked cleanup does not double destroy
}

}  // namespace
