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

#include <turbo/utility/utility.h>

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/str_cat.h>

namespace {

using ::testing::ElementsAre;
using ::testing::Pointee;
using ::testing::StaticAssertTypeEq;


int Function(int a, int b) { return a - b; }

int Sink(std::unique_ptr<int> p) { return *p; }

std::unique_ptr<int> Factory(int n) { return turbo::make_unique<int>(n); }

void NoOp() {}

struct ConstFunctor {
  int operator()(int a, int b) const { return a - b; }
};

struct MutableFunctor {
  int operator()(int a, int b) { return a - b; }
};

struct EphemeralFunctor {
  EphemeralFunctor() {}
  EphemeralFunctor(const EphemeralFunctor&) {}
  EphemeralFunctor(EphemeralFunctor&&) {}
  int operator()(int a, int b) && { return a - b; }
};

struct OverloadedFunctor {
  OverloadedFunctor() {}
  OverloadedFunctor(const OverloadedFunctor&) {}
  OverloadedFunctor(OverloadedFunctor&&) {}
  template <typename... Args>
  std::string operator()(const Args&... args) & {
    return turbo::str_cat("&", args...);
  }
  template <typename... Args>
  std::string operator()(const Args&... args) const& {
    return turbo::str_cat("const&", args...);
  }
  template <typename... Args>
  std::string operator()(const Args&... args) && {
    return turbo::str_cat("&&", args...);
  }
};

struct Class {
  int Method(int a, int b) { return a - b; }
  int ConstMethod(int a, int b) const { return a - b; }

  int member;
};

struct FlipFlop {
  int ConstMethod() const { return member; }
  FlipFlop operator*() const { return {-member}; }

  int member;
};

TEST(ApplyTest, Function) {
  EXPECT_EQ(1, turbo::apply(Function, std::make_tuple(3, 2)));
  EXPECT_EQ(1, turbo::apply(&Function, std::make_tuple(3, 2)));
}

TEST(ApplyTest, NonCopyableArgument) {
  EXPECT_EQ(42, turbo::apply(Sink, std::make_tuple(turbo::make_unique<int>(42))));
}

TEST(ApplyTest, NonCopyableResult) {
  EXPECT_THAT(turbo::apply(Factory, std::make_tuple(42)), Pointee(42));
}

TEST(ApplyTest, VoidResult) { turbo::apply(NoOp, std::tuple<>()); }

TEST(ApplyTest, ConstFunctor) {
  EXPECT_EQ(1, turbo::apply(ConstFunctor(), std::make_tuple(3, 2)));
}

TEST(ApplyTest, MutableFunctor) {
  MutableFunctor f;
  EXPECT_EQ(1, turbo::apply(f, std::make_tuple(3, 2)));
  EXPECT_EQ(1, turbo::apply(MutableFunctor(), std::make_tuple(3, 2)));
}
TEST(ApplyTest, EphemeralFunctor) {
  EphemeralFunctor f;
  EXPECT_EQ(1, turbo::apply(std::move(f), std::make_tuple(3, 2)));
  EXPECT_EQ(1, turbo::apply(EphemeralFunctor(), std::make_tuple(3, 2)));
}
TEST(ApplyTest, OverloadedFunctor) {
  OverloadedFunctor f;
  const OverloadedFunctor& cf = f;

  EXPECT_EQ("&", turbo::apply(f, std::tuple<>{}));
  EXPECT_EQ("& 42", turbo::apply(f, std::make_tuple(" 42")));

  EXPECT_EQ("const&", turbo::apply(cf, std::tuple<>{}));
  EXPECT_EQ("const& 42", turbo::apply(cf, std::make_tuple(" 42")));

  EXPECT_EQ("&&", turbo::apply(std::move(f), std::tuple<>{}));
  OverloadedFunctor f2;
  EXPECT_EQ("&& 42", turbo::apply(std::move(f2), std::make_tuple(" 42")));
}

TEST(ApplyTest, ReferenceWrapper) {
  ConstFunctor cf;
  MutableFunctor mf;
  EXPECT_EQ(1, turbo::apply(std::cref(cf), std::make_tuple(3, 2)));
  EXPECT_EQ(1, turbo::apply(std::ref(cf), std::make_tuple(3, 2)));
  EXPECT_EQ(1, turbo::apply(std::ref(mf), std::make_tuple(3, 2)));
}

TEST(ApplyTest, MemberFunction) {
  std::unique_ptr<Class> p(new Class);
  std::unique_ptr<const Class> cp(new Class);
  EXPECT_EQ(
      1, turbo::apply(&Class::Method,
                     std::tuple<std::unique_ptr<Class>&, int, int>(p, 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::Method,
                           std::tuple<Class*, int, int>(p.get(), 3, 2)));
  EXPECT_EQ(
      1, turbo::apply(&Class::Method, std::tuple<Class&, int, int>(*p, 3, 2)));

  EXPECT_EQ(
      1, turbo::apply(&Class::ConstMethod,
                     std::tuple<std::unique_ptr<Class>&, int, int>(p, 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::tuple<Class*, int, int>(p.get(), 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::tuple<Class&, int, int>(*p, 3, 2)));

  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::tuple<std::unique_ptr<const Class>&, int, int>(
                               cp, 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::tuple<const Class*, int, int>(cp.get(), 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::tuple<const Class&, int, int>(*cp, 3, 2)));

  EXPECT_EQ(1, turbo::apply(&Class::Method,
                           std::make_tuple(turbo::make_unique<Class>(), 3, 2)));
  EXPECT_EQ(1, turbo::apply(&Class::ConstMethod,
                           std::make_tuple(turbo::make_unique<Class>(), 3, 2)));
  EXPECT_EQ(
      1, turbo::apply(&Class::ConstMethod,
                     std::make_tuple(turbo::make_unique<const Class>(), 3, 2)));
}

TEST(ApplyTest, DataMember) {
  std::unique_ptr<Class> p(new Class{42});
  std::unique_ptr<const Class> cp(new Class{42});
  EXPECT_EQ(
      42, turbo::apply(&Class::member, std::tuple<std::unique_ptr<Class>&>(p)));
  EXPECT_EQ(42, turbo::apply(&Class::member, std::tuple<Class&>(*p)));
  EXPECT_EQ(42, turbo::apply(&Class::member, std::tuple<Class*>(p.get())));

  turbo::apply(&Class::member, std::tuple<std::unique_ptr<Class>&>(p)) = 42;
  turbo::apply(&Class::member, std::tuple<Class*>(p.get())) = 42;
  turbo::apply(&Class::member, std::tuple<Class&>(*p)) = 42;

  EXPECT_EQ(42, turbo::apply(&Class::member,
                            std::tuple<std::unique_ptr<const Class>&>(cp)));
  EXPECT_EQ(42, turbo::apply(&Class::member, std::tuple<const Class&>(*cp)));
  EXPECT_EQ(42,
            turbo::apply(&Class::member, std::tuple<const Class*>(cp.get())));
}

TEST(ApplyTest, FlipFlop) {
  FlipFlop obj = {42};
  // This call could resolve to (obj.*&FlipFlop::ConstMethod)() or
  // ((*obj).*&FlipFlop::ConstMethod)(). We verify that it's the former.
  EXPECT_EQ(42, turbo::apply(&FlipFlop::ConstMethod, std::make_tuple(obj)));
  EXPECT_EQ(42, turbo::apply(&FlipFlop::member, std::make_tuple(obj)));
}

TEST(MakeFromTupleTest, String) {
  EXPECT_EQ(
      turbo::make_from_tuple<std::string>(std::make_tuple("hello world", 5)),
      "hello");
}

TEST(MakeFromTupleTest, MoveOnlyParameter) {
  struct S {
    S(std::unique_ptr<int> n, std::unique_ptr<int> m) : value(*n + *m) {}
    int value = 0;
  };
  auto tup =
      std::make_tuple(turbo::make_unique<int>(3), turbo::make_unique<int>(4));
  auto s = turbo::make_from_tuple<S>(std::move(tup));
  EXPECT_EQ(s.value, 7);
}

TEST(MakeFromTupleTest, NoParameters) {
  struct S {
    S() : value(1) {}
    int value = 2;
  };
  EXPECT_EQ(turbo::make_from_tuple<S>(std::make_tuple()).value, 1);
}

TEST(MakeFromTupleTest, Pair) {
  EXPECT_EQ(
      (turbo::make_from_tuple<std::pair<bool, int>>(std::make_tuple(true, 17))),
      std::make_pair(true, 17));
}

}  // namespace
