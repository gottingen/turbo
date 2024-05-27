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

#include <turbo/functional/function_ref.h>

#include <functional>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/container/test_instance_tracker.h>
#include <turbo/functional/any_invocable.h>
#include <turbo/memory/memory.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

void RunFun(FunctionRef<void()> f) { f(); }

TEST(FunctionRefTest, Lambda) {
  bool ran = false;
  RunFun([&] { ran = true; });
  EXPECT_TRUE(ran);
}

int Function() { return 1337; }

TEST(FunctionRefTest, Function1) {
  FunctionRef<int()> ref(&Function);
  EXPECT_EQ(1337, ref());
}

TEST(FunctionRefTest, Function2) {
  FunctionRef<int()> ref(Function);
  EXPECT_EQ(1337, ref());
}

TEST(FunctionRefTest, ConstFunction) {
  FunctionRef<int() const> ref(Function);
  EXPECT_EQ(1337, ref());
}

int NoExceptFunction() noexcept { return 1337; }

// TODO(jdennett): Add a test for noexcept member functions.
TEST(FunctionRefTest, NoExceptFunction) {
  FunctionRef<int()> ref(NoExceptFunction);
  EXPECT_EQ(1337, ref());
}

TEST(FunctionRefTest, ForwardsArgs) {
  auto l = [](std::unique_ptr<int> i) { return *i; };
  FunctionRef<int(std::unique_ptr<int>)> ref(l);
  EXPECT_EQ(42, ref(turbo::make_unique<int>(42)));
}

TEST(FunctionRef, ReturnMoveOnly) {
  auto l = [] { return turbo::make_unique<int>(29); };
  FunctionRef<std::unique_ptr<int>()> ref(l);
  EXPECT_EQ(29, *ref());
}

TEST(FunctionRef, ManyArgs) {
  auto l = [](int a, int b, int c) { return a + b + c; };
  FunctionRef<int(int, int, int)> ref(l);
  EXPECT_EQ(6, ref(1, 2, 3));
}

TEST(FunctionRef, VoidResultFromNonVoidFunctor) {
  bool ran = false;
  auto l = [&]() -> int {
    ran = true;
    return 2;
  };
  FunctionRef<void()> ref(l);
  ref();
  EXPECT_TRUE(ran);
}

TEST(FunctionRef, CastFromDerived) {
  struct Base {};
  struct Derived : public Base {};

  Derived d;
  auto l1 = [&](Base* b) { EXPECT_EQ(&d, b); };
  FunctionRef<void(Derived*)> ref1(l1);
  ref1(&d);

  auto l2 = [&]() -> Derived* { return &d; };
  FunctionRef<Base*()> ref2(l2);
  EXPECT_EQ(&d, ref2());
}

TEST(FunctionRef, VoidResultFromNonVoidFuncton) {
  FunctionRef<void()> ref(Function);
  ref();
}

TEST(FunctionRef, MemberPtr) {
  struct S {
    int i;
  };

  S s{1100111};
  auto mem_ptr = &S::i;
  FunctionRef<int(const S& s)> ref(mem_ptr);
  EXPECT_EQ(1100111, ref(s));
}

TEST(FunctionRef, MemberFun) {
  struct S {
    int i;
    int get_i() const { return i; }
  };

  S s{22};
  auto mem_fun_ptr = &S::get_i;
  FunctionRef<int(const S& s)> ref(mem_fun_ptr);
  EXPECT_EQ(22, ref(s));
}

TEST(FunctionRef, MemberFunRefqualified) {
  struct S {
    int i;
    int get_i() && { return i; }
  };
  auto mem_fun_ptr = &S::get_i;
  S s{22};
  FunctionRef<int(S && s)> ref(mem_fun_ptr);
  EXPECT_EQ(22, ref(std::move(s)));
}

#if !defined(_WIN32) && defined(GTEST_HAS_DEATH_TEST)

TEST(FunctionRef, MemberFunRefqualifiedNull) {
  struct S {
    int i;
    int get_i() && { return i; }
  };
  auto mem_fun_ptr = &S::get_i;
  mem_fun_ptr = nullptr;
  EXPECT_DEBUG_DEATH({ FunctionRef<int(S && s)> ref(mem_fun_ptr); }, "");
}

TEST(FunctionRef, NullMemberPtrAssertFails) {
  struct S {
    int i;
  };
  using MemberPtr = int S::*;
  MemberPtr mem_ptr = nullptr;
  EXPECT_DEBUG_DEATH({ FunctionRef<int(const S& s)> ref(mem_ptr); }, "");
}

TEST(FunctionRef, NullStdFunctionAssertPasses) {
  std::function<void()> function = []() {};
  FunctionRef<void()> ref(function);
}

TEST(FunctionRef, NullStdFunctionAssertFails) {
  std::function<void()> function = nullptr;
  EXPECT_DEBUG_DEATH({ FunctionRef<void()> ref(function); }, "");
}

TEST(FunctionRef, NullAnyInvocableAssertPasses) {
  AnyInvocable<void() const> invocable = []() {};
  FunctionRef<void()> ref(invocable);
}
TEST(FunctionRef, NullAnyInvocableAssertFails) {
  AnyInvocable<void() const> invocable = nullptr;
  EXPECT_DEBUG_DEATH({ FunctionRef<void()> ref(invocable); }, "");
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(FunctionRef, CopiesAndMovesPerPassByValue) {
  turbo::test_internal::InstanceTracker tracker;
  turbo::test_internal::CopyableMovableInstance instance(0);
  auto l = [](turbo::test_internal::CopyableMovableInstance) {};
  FunctionRef<void(turbo::test_internal::CopyableMovableInstance)> ref(l);
  ref(instance);
  EXPECT_EQ(tracker.copies(), 1);
  EXPECT_EQ(tracker.moves(), 1);
}

TEST(FunctionRef, CopiesAndMovesPerPassByRef) {
  turbo::test_internal::InstanceTracker tracker;
  turbo::test_internal::CopyableMovableInstance instance(0);
  auto l = [](const turbo::test_internal::CopyableMovableInstance&) {};
  FunctionRef<void(const turbo::test_internal::CopyableMovableInstance&)> ref(l);
  ref(instance);
  EXPECT_EQ(tracker.copies(), 0);
  EXPECT_EQ(tracker.moves(), 0);
}

TEST(FunctionRef, CopiesAndMovesPerPassByValueCallByMove) {
  turbo::test_internal::InstanceTracker tracker;
  turbo::test_internal::CopyableMovableInstance instance(0);
  auto l = [](turbo::test_internal::CopyableMovableInstance) {};
  FunctionRef<void(turbo::test_internal::CopyableMovableInstance)> ref(l);
  ref(std::move(instance));
  EXPECT_EQ(tracker.copies(), 0);
  EXPECT_EQ(tracker.moves(), 2);
}

TEST(FunctionRef, CopiesAndMovesPerPassByValueToRef) {
  turbo::test_internal::InstanceTracker tracker;
  turbo::test_internal::CopyableMovableInstance instance(0);
  auto l = [](const turbo::test_internal::CopyableMovableInstance&) {};
  FunctionRef<void(turbo::test_internal::CopyableMovableInstance)> ref(l);
  ref(std::move(instance));
  EXPECT_EQ(tracker.copies(), 0);
  EXPECT_EQ(tracker.moves(), 1);
}

TEST(FunctionRef, PassByValueTypes) {
  using turbo::functional_internal::Invoker;
  using turbo::functional_internal::VoidPtr;
  using turbo::test_internal::CopyableMovableInstance;
  struct Trivial {
    void* p[2];
  };
  struct LargeTrivial {
    void* p[3];
  };

  static_assert(std::is_same<Invoker<void, int>, void (*)(VoidPtr, int)>::value,
                "Scalar types should be passed by value");
  static_assert(
      std::is_same<Invoker<void, Trivial>, void (*)(VoidPtr, Trivial)>::value,
      "Small trivial types should be passed by value");
  static_assert(std::is_same<Invoker<void, LargeTrivial>,
                             void (*)(VoidPtr, LargeTrivial &&)>::value,
                "Large trivial types should be passed by rvalue reference");
  static_assert(
      std::is_same<Invoker<void, CopyableMovableInstance>,
                   void (*)(VoidPtr, CopyableMovableInstance &&)>::value,
      "Types with copy/move ctor should be passed by rvalue reference");

  // References are passed as references.
  static_assert(
      std::is_same<Invoker<void, int&>, void (*)(VoidPtr, int&)>::value,
      "Reference types should be preserved");
  static_assert(
      std::is_same<Invoker<void, CopyableMovableInstance&>,
                   void (*)(VoidPtr, CopyableMovableInstance&)>::value,
      "Reference types should be preserved");
  static_assert(
      std::is_same<Invoker<void, CopyableMovableInstance&&>,
                   void (*)(VoidPtr, CopyableMovableInstance &&)>::value,
      "Reference types should be preserved");

  // Make sure the address of an object received by reference is the same as the
  // address of the object passed by the caller.
  {
    LargeTrivial obj;
    auto test = [&obj](LargeTrivial& input) { ASSERT_EQ(&input, &obj); };
    turbo::FunctionRef<void(LargeTrivial&)> ref(test);
    ref(obj);
  }

  {
    Trivial obj;
    auto test = [&obj](Trivial& input) { ASSERT_EQ(&input, &obj); };
    turbo::FunctionRef<void(Trivial&)> ref(test);
    ref(obj);
  }
}

TEST(FunctionRef, ReferenceToIncompleteType) {
  struct IncompleteType;
  auto test = [](IncompleteType&) {};
  turbo::FunctionRef<void(IncompleteType&)> ref(test);

  struct IncompleteType {};
  IncompleteType obj;
  ref(obj);
}

}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
