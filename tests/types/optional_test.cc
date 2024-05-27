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

// This test is a no-op when turbo::optional is an alias for std::optional.
#if !defined(TURBO_USES_STD_OPTIONAL)

#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/log/log.h>
#include <turbo/meta/type_traits.h>
#include <turbo/strings/string_view.h>

#if defined(__cplusplus) && __cplusplus >= 202002L
// In C++20, volatile-qualified return types are deprecated.
#define TURBO_VOLATILE_RETURN_TYPES_DEPRECATED 1
#endif

// The following types help test an internal compiler error in GCC5 though
// GCC10. The case OptionalTest.InternalCompilerErrorInGcc5ToGcc10 crashes the
// compiler without a workaround. This test case should remain at the beginning
// of the file as the internal compiler error is sensitive to other constructs
// in this file.
template <class T, class...>
using GccIceHelper1 = T;
template <typename T>
struct GccIceHelper2 {};
template <typename T>
class GccIce {
  template <typename U,
            typename SecondTemplateArgHasToExistForSomeReason = void,
            typename DependentType = void,
            typename = std::is_assignable<GccIceHelper1<T, DependentType>&, U>>
  GccIce& operator=(GccIceHelper2<U> const&) {}
};

TEST(OptionalTest, InternalCompilerErrorInGcc5ToGcc10) {
  GccIce<int> instantiate_ice_with_same_type_as_optional;
  static_cast<void>(instantiate_ice_with_same_type_as_optional);
  turbo::optional<int> val1;
  turbo::optional<int> val2;
  val1 = val2;
}

struct Hashable {};

namespace std {
template <>
struct hash<Hashable> {
  size_t operator()(const Hashable&) { return 0; }
};
}  // namespace std

struct NonHashable {};

namespace {

std::string TypeQuals(std::string&) { return "&"; }
std::string TypeQuals(std::string&&) { return "&&"; }
std::string TypeQuals(const std::string&) { return "c&"; }
std::string TypeQuals(const std::string&&) { return "c&&"; }

struct StructorListener {
  int construct0 = 0;
  int construct1 = 0;
  int construct2 = 0;
  int listinit = 0;
  int copy = 0;
  int move = 0;
  int copy_assign = 0;
  int move_assign = 0;
  int destruct = 0;
  int volatile_copy = 0;
  int volatile_move = 0;
  int volatile_copy_assign = 0;
  int volatile_move_assign = 0;
};

// Suppress MSVC warnings.
// 4521: multiple copy constructors specified
// 4522: multiple assignment operators specified
// We wrote multiple of them to test that the correct overloads are selected.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4521)
#pragma warning(disable : 4522)
#endif
struct Listenable {
  static StructorListener* listener;

  Listenable() { ++listener->construct0; }
  explicit Listenable(int /*unused*/) { ++listener->construct1; }
  Listenable(int /*unused*/, int /*unused*/) { ++listener->construct2; }
  Listenable(std::initializer_list<int> /*unused*/) { ++listener->listinit; }
  Listenable(const Listenable& /*unused*/) { ++listener->copy; }
  Listenable(const volatile Listenable& /*unused*/) {
    ++listener->volatile_copy;
  }
  Listenable(volatile Listenable&& /*unused*/) { ++listener->volatile_move; }
  Listenable(Listenable&& /*unused*/) { ++listener->move; }
  Listenable& operator=(const Listenable& /*unused*/) {
    ++listener->copy_assign;
    return *this;
  }
  Listenable& operator=(Listenable&& /*unused*/) {
    ++listener->move_assign;
    return *this;
  }
  // use void return type instead of volatile T& to work around GCC warning
  // when the assignment's returned reference is ignored.
  void operator=(const volatile Listenable& /*unused*/) volatile {
    ++listener->volatile_copy_assign;
  }
  void operator=(volatile Listenable&& /*unused*/) volatile {
    ++listener->volatile_move_assign;
  }
  ~Listenable() { ++listener->destruct; }
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

StructorListener* Listenable::listener = nullptr;

struct ConstexprType {
  enum CtorTypes {
    kCtorDefault,
    kCtorInt,
    kCtorInitializerList,
    kCtorConstChar
  };
  constexpr ConstexprType() : x(kCtorDefault) {}
  constexpr explicit ConstexprType(int i) : x(kCtorInt) {}
  constexpr ConstexprType(std::initializer_list<int> il)
      : x(kCtorInitializerList) {}
  constexpr ConstexprType(const char*)  // NOLINT(runtime/explicit)
      : x(kCtorConstChar) {}
  int x;
};

struct Copyable {
  Copyable() {}
  Copyable(const Copyable&) {}
  Copyable& operator=(const Copyable&) { return *this; }
};

struct MoveableThrow {
  MoveableThrow() {}
  MoveableThrow(MoveableThrow&&) {}
  MoveableThrow& operator=(MoveableThrow&&) { return *this; }
};

struct MoveableNoThrow {
  MoveableNoThrow() {}
  MoveableNoThrow(MoveableNoThrow&&) noexcept {}
  MoveableNoThrow& operator=(MoveableNoThrow&&) noexcept { return *this; }
};

struct NonMovable {
  NonMovable() {}
  NonMovable(const NonMovable&) = delete;
  NonMovable& operator=(const NonMovable&) = delete;
  NonMovable(NonMovable&&) = delete;
  NonMovable& operator=(NonMovable&&) = delete;
};

struct NoDefault {
  NoDefault() = delete;
  NoDefault(const NoDefault&) {}
  NoDefault& operator=(const NoDefault&) { return *this; }
};

struct ConvertsFromInPlaceT {
  ConvertsFromInPlaceT(turbo::in_place_t) {}  // NOLINT
};

TEST(optionalTest, DefaultConstructor) {
  turbo::optional<int> empty;
  EXPECT_FALSE(empty);
  constexpr turbo::optional<int> cempty;
  static_assert(!cempty.has_value(), "");
  EXPECT_TRUE(
      std::is_nothrow_default_constructible<turbo::optional<int>>::value);
}

TEST(optionalTest, nulloptConstructor) {
  turbo::optional<int> empty(turbo::nullopt);
  EXPECT_FALSE(empty);
  constexpr turbo::optional<int> cempty{turbo::nullopt};
  static_assert(!cempty.has_value(), "");
  EXPECT_TRUE((std::is_nothrow_constructible<turbo::optional<int>,
                                             turbo::nullopt_t>::value));
}

TEST(optionalTest, CopyConstructor) {
  {
    turbo::optional<int> empty, opt42 = 42;
    turbo::optional<int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    turbo::optional<int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
  {
    turbo::optional<const int> empty, opt42 = 42;
    turbo::optional<const int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    turbo::optional<const int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  {
    turbo::optional<volatile int> empty, opt42 = 42;
    turbo::optional<volatile int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    turbo::optional<volatile int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
#endif
  // test copyablility
  EXPECT_TRUE(std::is_copy_constructible<turbo::optional<int>>::value);
  EXPECT_TRUE(std::is_copy_constructible<turbo::optional<Copyable>>::value);
  EXPECT_FALSE(
      std::is_copy_constructible<turbo::optional<MoveableThrow>>::value);
  EXPECT_FALSE(
      std::is_copy_constructible<turbo::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(std::is_copy_constructible<turbo::optional<NonMovable>>::value);

  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<turbo::optional<Copyable>>::value);
  EXPECT_TRUE(
      turbo::is_trivially_copy_constructible<turbo::optional<int>>::value);
  EXPECT_TRUE(
      turbo::is_trivially_copy_constructible<turbo::optional<const int>>::value);
#if !defined(_MSC_VER) && !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // See defect report "Trivial copy/move constructor for class with volatile
  // member" at
  // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#2094
  // A class with non-static data member of volatile-qualified type should still
  // have a trivial copy constructor if the data member is trivial.
  // Also a cv-qualified scalar type should be trivially copyable.
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<
              turbo::optional<volatile int>>::value);
#endif  // !defined(_MSC_VER) && !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  // constexpr copy constructor for trivially copyable types
  {
    constexpr turbo::optional<int> o1;
    constexpr turbo::optional<int> o2 = o1;
    static_assert(!o2, "");
  }
  {
    constexpr turbo::optional<int> o1 = 42;
    constexpr turbo::optional<int> o2 = o1;
    static_assert(o2, "");
    static_assert(*o2 == 42, "");
  }
  {
    struct TrivialCopyable {
      constexpr TrivialCopyable() : x(0) {}
      constexpr explicit TrivialCopyable(int i) : x(i) {}
      int x;
    };
    constexpr turbo::optional<TrivialCopyable> o1(42);
    constexpr turbo::optional<TrivialCopyable> o2 = o1;
    static_assert(o2, "");
    static_assert((*o2).x == 42, "");
#ifndef TURBO_GLIBCXX_OPTIONAL_TRIVIALITY_BUG
    EXPECT_TRUE(turbo::is_trivially_copy_constructible<
                turbo::optional<TrivialCopyable>>::value);
    EXPECT_TRUE(turbo::is_trivially_copy_constructible<
                turbo::optional<const TrivialCopyable>>::value);
#endif
#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
    EXPECT_FALSE(std::is_copy_constructible<
                 turbo::optional<volatile TrivialCopyable>>::value);
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  }
}

TEST(optionalTest, MoveConstructor) {
  turbo::optional<int> empty, opt42 = 42;
  turbo::optional<int> empty_move(std::move(empty));
  EXPECT_FALSE(empty_move);
  turbo::optional<int> opt42_move(std::move(opt42));
  EXPECT_TRUE(opt42_move);
  EXPECT_EQ(42, opt42_move);
  // test movability
  EXPECT_TRUE(std::is_move_constructible<turbo::optional<int>>::value);
  EXPECT_TRUE(std::is_move_constructible<turbo::optional<Copyable>>::value);
  EXPECT_TRUE(std::is_move_constructible<turbo::optional<MoveableThrow>>::value);
  EXPECT_TRUE(
      std::is_move_constructible<turbo::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(std::is_move_constructible<turbo::optional<NonMovable>>::value);
  // test noexcept
  EXPECT_TRUE(std::is_nothrow_move_constructible<turbo::optional<int>>::value);
  EXPECT_EQ(
      turbo::default_allocator_is_nothrow::value,
      std::is_nothrow_move_constructible<turbo::optional<MoveableThrow>>::value);
  EXPECT_TRUE(std::is_nothrow_move_constructible<
              turbo::optional<MoveableNoThrow>>::value);
}

TEST(optionalTest, Destructor) {
  struct Trivial {};

  struct NonTrivial {
    NonTrivial(const NonTrivial&) {}
    NonTrivial& operator=(const NonTrivial&) { return *this; }
    ~NonTrivial() {}
  };

  EXPECT_TRUE(std::is_trivially_destructible<turbo::optional<int>>::value);
  EXPECT_TRUE(std::is_trivially_destructible<turbo::optional<Trivial>>::value);
  EXPECT_FALSE(
      std::is_trivially_destructible<turbo::optional<NonTrivial>>::value);
}

TEST(optionalTest, InPlaceConstructor) {
  constexpr turbo::optional<ConstexprType> opt0{turbo::in_place_t()};
  static_assert(opt0, "");
  static_assert((*opt0).x == ConstexprType::kCtorDefault, "");
  constexpr turbo::optional<ConstexprType> opt1{turbo::in_place_t(), 1};
  static_assert(opt1, "");
  static_assert((*opt1).x == ConstexprType::kCtorInt, "");
  constexpr turbo::optional<ConstexprType> opt2{turbo::in_place_t(), {1, 2}};
  static_assert(opt2, "");
  static_assert((*opt2).x == ConstexprType::kCtorInitializerList, "");

  EXPECT_FALSE((std::is_constructible<turbo::optional<ConvertsFromInPlaceT>,
                                      turbo::in_place_t>::value));
  EXPECT_FALSE((std::is_constructible<turbo::optional<ConvertsFromInPlaceT>,
                                      const turbo::in_place_t&>::value));
  EXPECT_TRUE(
      (std::is_constructible<turbo::optional<ConvertsFromInPlaceT>,
                             turbo::in_place_t, turbo::in_place_t>::value));

  EXPECT_FALSE((std::is_constructible<turbo::optional<NoDefault>,
                                      turbo::in_place_t>::value));
  EXPECT_FALSE((std::is_constructible<turbo::optional<NoDefault>,
                                      turbo::in_place_t&&>::value));
}

// template<U=T> optional(U&&);
TEST(optionalTest, ValueConstructor) {
  constexpr turbo::optional<int> opt0(0);
  static_assert(opt0, "");
  static_assert(*opt0 == 0, "");
  EXPECT_TRUE((std::is_convertible<int, turbo::optional<int>>::value));
  // Copy initialization ( = "abc") won't work due to optional(optional&&)
  // is not constexpr. Use list initialization instead. This invokes
  // turbo::optional<ConstexprType>::turbo::optional<U>(U&&), with U = const char
  // (&) [4], which direct-initializes the ConstexprType value held by the
  // optional via ConstexprType::ConstexprType(const char*).
  constexpr turbo::optional<ConstexprType> opt1 = {"abc"};
  static_assert(opt1, "");
  static_assert(ConstexprType::kCtorConstChar == (*opt1).x, "");
  EXPECT_TRUE(
      (std::is_convertible<const char*, turbo::optional<ConstexprType>>::value));
  // direct initialization
  constexpr turbo::optional<ConstexprType> opt2{2};
  static_assert(opt2, "");
  static_assert(ConstexprType::kCtorInt == (*opt2).x, "");
  EXPECT_FALSE(
      (std::is_convertible<int, turbo::optional<ConstexprType>>::value));

  // this invokes turbo::optional<int>::optional(int&&)
  // NOTE: this has different behavior than assignment, e.g.
  // "opt3 = {};" clears the optional rather than setting the value to 0
  // According to C++17 standard N4659 [over.ics.list] 16.3.3.1.5, (9.2)- "if
  // the initializer list has no elements, the implicit conversion is the
  // identity conversion", so `optional(int&&)` should be a better match than
  // `optional(optional&&)` which is a user-defined conversion.
  // Note: GCC 7 has a bug with this overload selection when compiled with
  // `-std=c++17`.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 7 && \
    __cplusplus == 201703L
#define TURBO_GCC7_OVER_ICS_LIST_BUG 1
#endif
#ifndef TURBO_GCC7_OVER_ICS_LIST_BUG
  constexpr turbo::optional<int> opt3({});
  static_assert(opt3, "");
  static_assert(*opt3 == 0, "");
#endif

  // this invokes the move constructor with a default constructed optional
  // because non-template function is a better match than template function.
  turbo::optional<ConstexprType> opt4({});
  EXPECT_FALSE(opt4);
}

struct Implicit {};

struct Explicit {};

struct Convert {
  Convert(const Implicit&)  // NOLINT(runtime/explicit)
      : implicit(true), move(false) {}
  Convert(Implicit&&)  // NOLINT(runtime/explicit)
      : implicit(true), move(true) {}
  explicit Convert(const Explicit&) : implicit(false), move(false) {}
  explicit Convert(Explicit&&) : implicit(false), move(true) {}

  bool implicit;
  bool move;
};

struct ConvertFromOptional {
  ConvertFromOptional(const Implicit&)  // NOLINT(runtime/explicit)
      : implicit(true), move(false), from_optional(false) {}
  ConvertFromOptional(Implicit&&)  // NOLINT(runtime/explicit)
      : implicit(true), move(true), from_optional(false) {}
  ConvertFromOptional(
      const turbo::optional<Implicit>&)  // NOLINT(runtime/explicit)
      : implicit(true), move(false), from_optional(true) {}
  ConvertFromOptional(turbo::optional<Implicit>&&)  // NOLINT(runtime/explicit)
      : implicit(true), move(true), from_optional(true) {}
  explicit ConvertFromOptional(const Explicit&)
      : implicit(false), move(false), from_optional(false) {}
  explicit ConvertFromOptional(Explicit&&)
      : implicit(false), move(true), from_optional(false) {}
  explicit ConvertFromOptional(const turbo::optional<Explicit>&)
      : implicit(false), move(false), from_optional(true) {}
  explicit ConvertFromOptional(turbo::optional<Explicit>&&)
      : implicit(false), move(true), from_optional(true) {}

  bool implicit;
  bool move;
  bool from_optional;
};

TEST(optionalTest, ConvertingConstructor) {
  turbo::optional<Implicit> i_empty;
  turbo::optional<Implicit> i(turbo::in_place);
  turbo::optional<Explicit> e_empty;
  turbo::optional<Explicit> e(turbo::in_place);
  {
    // implicitly constructing turbo::optional<Convert> from
    // turbo::optional<Implicit>
    turbo::optional<Convert> empty = i_empty;
    EXPECT_FALSE(empty);
    turbo::optional<Convert> opt_copy = i;
    EXPECT_TRUE(opt_copy);
    EXPECT_TRUE(opt_copy->implicit);
    EXPECT_FALSE(opt_copy->move);
    turbo::optional<Convert> opt_move = turbo::optional<Implicit>(turbo::in_place);
    EXPECT_TRUE(opt_move);
    EXPECT_TRUE(opt_move->implicit);
    EXPECT_TRUE(opt_move->move);
  }
  {
    // explicitly constructing turbo::optional<Convert> from
    // turbo::optional<Explicit>
    turbo::optional<Convert> empty(e_empty);
    EXPECT_FALSE(empty);
    turbo::optional<Convert> opt_copy(e);
    EXPECT_TRUE(opt_copy);
    EXPECT_FALSE(opt_copy->implicit);
    EXPECT_FALSE(opt_copy->move);
    EXPECT_FALSE((std::is_convertible<const turbo::optional<Explicit>&,
                                      turbo::optional<Convert>>::value));
    turbo::optional<Convert> opt_move{turbo::optional<Explicit>(turbo::in_place)};
    EXPECT_TRUE(opt_move);
    EXPECT_FALSE(opt_move->implicit);
    EXPECT_TRUE(opt_move->move);
    EXPECT_FALSE((std::is_convertible<turbo::optional<Explicit>&&,
                                      turbo::optional<Convert>>::value));
  }
  {
    // implicitly constructing turbo::optional<ConvertFromOptional> from
    // turbo::optional<Implicit> via
    // ConvertFromOptional(turbo::optional<Implicit>&&) check that
    // ConvertFromOptional(Implicit&&) is NOT called
    static_assert(
        std::is_convertible<turbo::optional<Implicit>,
                            turbo::optional<ConvertFromOptional>>::value,
        "");
    turbo::optional<ConvertFromOptional> opt0 = i_empty;
    EXPECT_TRUE(opt0);
    EXPECT_TRUE(opt0->implicit);
    EXPECT_FALSE(opt0->move);
    EXPECT_TRUE(opt0->from_optional);
    turbo::optional<ConvertFromOptional> opt1 = turbo::optional<Implicit>();
    EXPECT_TRUE(opt1);
    EXPECT_TRUE(opt1->implicit);
    EXPECT_TRUE(opt1->move);
    EXPECT_TRUE(opt1->from_optional);
  }
  {
    // implicitly constructing turbo::optional<ConvertFromOptional> from
    // turbo::optional<Explicit> via
    // ConvertFromOptional(turbo::optional<Explicit>&&) check that
    // ConvertFromOptional(Explicit&&) is NOT called
    turbo::optional<ConvertFromOptional> opt0(e_empty);
    EXPECT_TRUE(opt0);
    EXPECT_FALSE(opt0->implicit);
    EXPECT_FALSE(opt0->move);
    EXPECT_TRUE(opt0->from_optional);
    EXPECT_FALSE(
        (std::is_convertible<const turbo::optional<Explicit>&,
                             turbo::optional<ConvertFromOptional>>::value));
    turbo::optional<ConvertFromOptional> opt1{turbo::optional<Explicit>()};
    EXPECT_TRUE(opt1);
    EXPECT_FALSE(opt1->implicit);
    EXPECT_TRUE(opt1->move);
    EXPECT_TRUE(opt1->from_optional);
    EXPECT_FALSE(
        (std::is_convertible<turbo::optional<Explicit>&&,
                             turbo::optional<ConvertFromOptional>>::value));
  }
}

TEST(optionalTest, StructorBasic) {
  StructorListener listener;
  Listenable::listener = &listener;
  {
    turbo::optional<Listenable> empty;
    EXPECT_FALSE(empty);
    turbo::optional<Listenable> opt0(turbo::in_place);
    EXPECT_TRUE(opt0);
    turbo::optional<Listenable> opt1(turbo::in_place, 1);
    EXPECT_TRUE(opt1);
    turbo::optional<Listenable> opt2(turbo::in_place, 1, 2);
    EXPECT_TRUE(opt2);
  }
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.construct1);
  EXPECT_EQ(1, listener.construct2);
  EXPECT_EQ(3, listener.destruct);
}

TEST(optionalTest, CopyMoveStructor) {
  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> original(turbo::in_place);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(0, listener.copy);
  EXPECT_EQ(0, listener.move);
  turbo::optional<Listenable> copy(original);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.copy);
  EXPECT_EQ(0, listener.move);
  turbo::optional<Listenable> move(std::move(original));
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.copy);
  EXPECT_EQ(1, listener.move);
}

TEST(optionalTest, ListInit) {
  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> listinit1(turbo::in_place, {1});
  turbo::optional<Listenable> listinit2(turbo::in_place, {1, 2});
  EXPECT_EQ(2, listener.listinit);
}

TEST(optionalTest, AssignFromNullopt) {
  turbo::optional<int> opt(1);
  opt = turbo::nullopt;
  EXPECT_FALSE(opt);

  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> opt1(turbo::in_place);
  opt1 = turbo::nullopt;
  EXPECT_FALSE(opt1);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.destruct);

  EXPECT_TRUE((
      std::is_nothrow_assignable<turbo::optional<int>, turbo::nullopt_t>::value));
  EXPECT_TRUE((std::is_nothrow_assignable<turbo::optional<Listenable>,
                                          turbo::nullopt_t>::value));
}

TEST(optionalTest, CopyAssignment) {
  const turbo::optional<int> empty, opt1 = 1, opt2 = 2;
  turbo::optional<int> empty_to_opt1, opt1_to_opt2, opt2_to_empty;

  EXPECT_FALSE(empty_to_opt1);
  empty_to_opt1 = empty;
  EXPECT_FALSE(empty_to_opt1);
  empty_to_opt1 = opt1;
  EXPECT_TRUE(empty_to_opt1);
  EXPECT_EQ(1, empty_to_opt1.value());

  EXPECT_FALSE(opt1_to_opt2);
  opt1_to_opt2 = opt1;
  EXPECT_TRUE(opt1_to_opt2);
  EXPECT_EQ(1, opt1_to_opt2.value());
  opt1_to_opt2 = opt2;
  EXPECT_TRUE(opt1_to_opt2);
  EXPECT_EQ(2, opt1_to_opt2.value());

  EXPECT_FALSE(opt2_to_empty);
  opt2_to_empty = opt2;
  EXPECT_TRUE(opt2_to_empty);
  EXPECT_EQ(2, opt2_to_empty.value());
  opt2_to_empty = empty;
  EXPECT_FALSE(opt2_to_empty);

  EXPECT_FALSE(turbo::is_copy_assignable<turbo::optional<const int>>::value);
  EXPECT_TRUE(turbo::is_copy_assignable<turbo::optional<Copyable>>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<turbo::optional<MoveableThrow>>::value);
  EXPECT_FALSE(
      turbo::is_copy_assignable<turbo::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<turbo::optional<NonMovable>>::value);

  EXPECT_TRUE(turbo::is_trivially_copy_assignable<int>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<volatile int>::value);

  struct Trivial {
    int i;
  };
  struct NonTrivial {
    NonTrivial& operator=(const NonTrivial&) { return *this; }
    int i;
  };

  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<const Trivial>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<volatile Trivial>::value);
  EXPECT_TRUE(turbo::is_copy_assignable<NonTrivial>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<NonTrivial>::value);

#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  {
    StructorListener listener;
    Listenable::listener = &listener;

    turbo::optional<volatile Listenable> empty, set(turbo::in_place);
    EXPECT_EQ(1, listener.construct0);
    turbo::optional<volatile Listenable> empty_to_empty, empty_to_set,
        set_to_empty(turbo::in_place), set_to_set(turbo::in_place);
    EXPECT_EQ(3, listener.construct0);
    empty_to_empty = empty;  // no effect
    empty_to_set = set;      // copy construct
    set_to_empty = empty;    // destruct
    set_to_set = set;        // copy assign
    EXPECT_EQ(1, listener.volatile_copy);
    EXPECT_EQ(0, listener.volatile_move);
    EXPECT_EQ(1, listener.destruct);
    EXPECT_EQ(1, listener.volatile_copy_assign);
  }
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
}

TEST(optionalTest, MoveAssignment) {
  {
    StructorListener listener;
    Listenable::listener = &listener;

    turbo::optional<Listenable> empty1, empty2, set1(turbo::in_place),
        set2(turbo::in_place);
    EXPECT_EQ(2, listener.construct0);
    turbo::optional<Listenable> empty_to_empty, empty_to_set,
        set_to_empty(turbo::in_place), set_to_set(turbo::in_place);
    EXPECT_EQ(4, listener.construct0);
    empty_to_empty = std::move(empty1);
    empty_to_set = std::move(set1);
    set_to_empty = std::move(empty2);
    set_to_set = std::move(set2);
    EXPECT_EQ(0, listener.copy);
    EXPECT_EQ(1, listener.move);
    EXPECT_EQ(1, listener.destruct);
    EXPECT_EQ(1, listener.move_assign);
  }
#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  {
    StructorListener listener;
    Listenable::listener = &listener;

    turbo::optional<volatile Listenable> empty1, empty2, set1(turbo::in_place),
        set2(turbo::in_place);
    EXPECT_EQ(2, listener.construct0);
    turbo::optional<volatile Listenable> empty_to_empty, empty_to_set,
        set_to_empty(turbo::in_place), set_to_set(turbo::in_place);
    EXPECT_EQ(4, listener.construct0);
    empty_to_empty = std::move(empty1);  // no effect
    empty_to_set = std::move(set1);      // move construct
    set_to_empty = std::move(empty2);    // destruct
    set_to_set = std::move(set2);        // move assign
    EXPECT_EQ(0, listener.volatile_copy);
    EXPECT_EQ(1, listener.volatile_move);
    EXPECT_EQ(1, listener.destruct);
    EXPECT_EQ(1, listener.volatile_move_assign);
  }
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  EXPECT_FALSE(turbo::is_move_assignable<turbo::optional<const int>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<turbo::optional<Copyable>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<turbo::optional<MoveableThrow>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<turbo::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(turbo::is_move_assignable<turbo::optional<NonMovable>>::value);

  EXPECT_FALSE(
      std::is_nothrow_move_assignable<turbo::optional<MoveableThrow>>::value);
  EXPECT_TRUE(
      std::is_nothrow_move_assignable<turbo::optional<MoveableNoThrow>>::value);
}

struct NoConvertToOptional {
  // disable implicit conversion from const NoConvertToOptional&
  // to turbo::optional<NoConvertToOptional>.
  NoConvertToOptional(const NoConvertToOptional&) = delete;
};

struct CopyConvert {
  CopyConvert(const NoConvertToOptional&);
  CopyConvert& operator=(const CopyConvert&) = delete;
  CopyConvert& operator=(const NoConvertToOptional&);
};

struct CopyConvertFromOptional {
  CopyConvertFromOptional(const NoConvertToOptional&);
  CopyConvertFromOptional(const turbo::optional<NoConvertToOptional>&);
  CopyConvertFromOptional& operator=(const CopyConvertFromOptional&) = delete;
  CopyConvertFromOptional& operator=(const NoConvertToOptional&);
  CopyConvertFromOptional& operator=(
      const turbo::optional<NoConvertToOptional>&);
};

struct MoveConvert {
  MoveConvert(NoConvertToOptional&&);
  MoveConvert& operator=(const MoveConvert&) = delete;
  MoveConvert& operator=(NoConvertToOptional&&);
};

struct MoveConvertFromOptional {
  MoveConvertFromOptional(NoConvertToOptional&&);
  MoveConvertFromOptional(turbo::optional<NoConvertToOptional>&&);
  MoveConvertFromOptional& operator=(const MoveConvertFromOptional&) = delete;
  MoveConvertFromOptional& operator=(NoConvertToOptional&&);
  MoveConvertFromOptional& operator=(turbo::optional<NoConvertToOptional>&&);
};

// template <typename U = T> turbo::optional<T>& operator=(U&& v);
TEST(optionalTest, ValueAssignment) {
  turbo::optional<int> opt;
  EXPECT_FALSE(opt);
  opt = 42;
  EXPECT_TRUE(opt);
  EXPECT_EQ(42, opt.value());
  opt = turbo::nullopt;
  EXPECT_FALSE(opt);
  opt = 42;
  EXPECT_TRUE(opt);
  EXPECT_EQ(42, opt.value());
  opt = 43;
  EXPECT_TRUE(opt);
  EXPECT_EQ(43, opt.value());
  opt = {};  // this should clear optional
  EXPECT_FALSE(opt);

  opt = {44};
  EXPECT_TRUE(opt);
  EXPECT_EQ(44, opt.value());

  // U = const NoConvertToOptional&
  EXPECT_TRUE((std::is_assignable<turbo::optional<CopyConvert>&,
                                  const NoConvertToOptional&>::value));
  // U = const turbo::optional<NoConvertToOptional>&
  EXPECT_TRUE((std::is_assignable<turbo::optional<CopyConvertFromOptional>&,
                                  const NoConvertToOptional&>::value));
  // U = const NoConvertToOptional& triggers SFINAE because
  // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
  EXPECT_FALSE((std::is_assignable<turbo::optional<MoveConvert>&,
                                   const NoConvertToOptional&>::value));
  // U = NoConvertToOptional
  EXPECT_TRUE((std::is_assignable<turbo::optional<MoveConvert>&,
                                  NoConvertToOptional&&>::value));
  // U = const NoConvertToOptional& triggers SFINAE because
  // std::is_constructible_v<MoveConvertFromOptional, const
  // NoConvertToOptional&> is false
  EXPECT_FALSE((std::is_assignable<turbo::optional<MoveConvertFromOptional>&,
                                   const NoConvertToOptional&>::value));
  // U = NoConvertToOptional
  EXPECT_TRUE((std::is_assignable<turbo::optional<MoveConvertFromOptional>&,
                                  NoConvertToOptional&&>::value));
  // U = const turbo::optional<NoConvertToOptional>&
  EXPECT_TRUE(
      (std::is_assignable<turbo::optional<CopyConvertFromOptional>&,
                          const turbo::optional<NoConvertToOptional>&>::value));
  // U = turbo::optional<NoConvertToOptional>
  EXPECT_TRUE(
      (std::is_assignable<turbo::optional<MoveConvertFromOptional>&,
                          turbo::optional<NoConvertToOptional>&&>::value));
}

// template <typename U> turbo::optional<T>& operator=(const turbo::optional<U>&
// rhs); template <typename U> turbo::optional<T>& operator=(turbo::optional<U>&&
// rhs);
TEST(optionalTest, ConvertingAssignment) {
  turbo::optional<int> opt_i;
  turbo::optional<char> opt_c('c');
  opt_i = opt_c;
  EXPECT_TRUE(opt_i);
  EXPECT_EQ(*opt_c, *opt_i);
  opt_i = turbo::optional<char>();
  EXPECT_FALSE(opt_i);
  opt_i = turbo::optional<char>('d');
  EXPECT_TRUE(opt_i);
  EXPECT_EQ('d', *opt_i);

  turbo::optional<std::string> opt_str;
  turbo::optional<const char*> opt_cstr("abc");
  opt_str = opt_cstr;
  EXPECT_TRUE(opt_str);
  EXPECT_EQ(std::string("abc"), *opt_str);
  opt_str = turbo::optional<const char*>();
  EXPECT_FALSE(opt_str);
  opt_str = turbo::optional<const char*>("def");
  EXPECT_TRUE(opt_str);
  EXPECT_EQ(std::string("def"), *opt_str);

  // operator=(const turbo::optional<U>&) with U = NoConvertToOptional
  EXPECT_TRUE(
      (std::is_assignable<turbo::optional<CopyConvert>,
                          const turbo::optional<NoConvertToOptional>&>::value));
  // operator=(const turbo::optional<U>&) with U = NoConvertToOptional
  // triggers SFINAE because
  // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
  EXPECT_FALSE(
      (std::is_assignable<turbo::optional<MoveConvert>&,
                          const turbo::optional<NoConvertToOptional>&>::value));
  // operator=(turbo::optional<U>&&) with U = NoConvertToOptional
  EXPECT_TRUE(
      (std::is_assignable<turbo::optional<MoveConvert>&,
                          turbo::optional<NoConvertToOptional>&&>::value));
  // operator=(const turbo::optional<U>&) with U = NoConvertToOptional triggers
  // SFINAE because std::is_constructible_v<MoveConvertFromOptional, const
  // NoConvertToOptional&> is false. operator=(U&&) with U = const
  // turbo::optional<NoConverToOptional>& triggers SFINAE because
  // std::is_constructible<MoveConvertFromOptional,
  // turbo::optional<NoConvertToOptional>&&> is true.
  EXPECT_FALSE(
      (std::is_assignable<turbo::optional<MoveConvertFromOptional>&,
                          const turbo::optional<NoConvertToOptional>&>::value));
}

TEST(optionalTest, ResetAndHasValue) {
  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> opt;
  EXPECT_FALSE(opt);
  EXPECT_FALSE(opt.has_value());
  opt.emplace();
  EXPECT_TRUE(opt);
  EXPECT_TRUE(opt.has_value());
  opt.reset();
  EXPECT_FALSE(opt);
  EXPECT_FALSE(opt.has_value());
  EXPECT_EQ(1, listener.destruct);
  opt.reset();
  EXPECT_FALSE(opt);
  EXPECT_FALSE(opt.has_value());

  constexpr turbo::optional<int> empty;
  static_assert(!empty.has_value(), "");
  constexpr turbo::optional<int> nonempty(1);
  static_assert(nonempty.has_value(), "");
}

TEST(optionalTest, Emplace) {
  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> opt;
  EXPECT_FALSE(opt);
  opt.emplace(1);
  EXPECT_TRUE(opt);
  opt.emplace(1, 2);
  EXPECT_EQ(1, listener.construct1);
  EXPECT_EQ(1, listener.construct2);
  EXPECT_EQ(1, listener.destruct);

  turbo::optional<std::string> o;
  EXPECT_TRUE((std::is_same<std::string&, decltype(o.emplace("abc"))>::value));
  std::string& ref = o.emplace("abc");
  EXPECT_EQ(&ref, &o.value());
}

TEST(optionalTest, ListEmplace) {
  StructorListener listener;
  Listenable::listener = &listener;
  turbo::optional<Listenable> opt;
  EXPECT_FALSE(opt);
  opt.emplace({1});
  EXPECT_TRUE(opt);
  opt.emplace({1, 2});
  EXPECT_EQ(2, listener.listinit);
  EXPECT_EQ(1, listener.destruct);

  turbo::optional<Listenable> o;
  EXPECT_TRUE((std::is_same<Listenable&, decltype(o.emplace({1}))>::value));
  Listenable& ref = o.emplace({1});
  EXPECT_EQ(&ref, &o.value());
}

TEST(optionalTest, Swap) {
  turbo::optional<int> opt_empty, opt1 = 1, opt2 = 2;
  EXPECT_FALSE(opt_empty);
  EXPECT_TRUE(opt1);
  EXPECT_EQ(1, opt1.value());
  EXPECT_TRUE(opt2);
  EXPECT_EQ(2, opt2.value());
  swap(opt_empty, opt1);
  EXPECT_FALSE(opt1);
  EXPECT_TRUE(opt_empty);
  EXPECT_EQ(1, opt_empty.value());
  EXPECT_TRUE(opt2);
  EXPECT_EQ(2, opt2.value());
  swap(opt_empty, opt1);
  EXPECT_FALSE(opt_empty);
  EXPECT_TRUE(opt1);
  EXPECT_EQ(1, opt1.value());
  EXPECT_TRUE(opt2);
  EXPECT_EQ(2, opt2.value());
  swap(opt1, opt2);
  EXPECT_FALSE(opt_empty);
  EXPECT_TRUE(opt1);
  EXPECT_EQ(2, opt1.value());
  EXPECT_TRUE(opt2);
  EXPECT_EQ(1, opt2.value());

  EXPECT_TRUE(noexcept(opt1.swap(opt2)));
  EXPECT_TRUE(noexcept(swap(opt1, opt2)));
}

template <int v>
struct DeletedOpAddr {
  int value = v;
  constexpr DeletedOpAddr() = default;
  constexpr const DeletedOpAddr<v>* operator&() const = delete;  // NOLINT
  DeletedOpAddr<v>* operator&() = delete;                        // NOLINT
};

// The static_assert featuring a constexpr call to operator->() is commented out
// to document the fact that the current implementation of turbo::optional<T>
// expects such usecases to be malformed and not compile.
TEST(optionalTest, OperatorAddr) {
  constexpr int v = -1;
  {  // constexpr
    constexpr turbo::optional<DeletedOpAddr<v>> opt(turbo::in_place_t{});
    static_assert(opt.has_value(), "");
    // static_assert(opt->value == v, "");
    static_assert((*opt).value == v, "");
  }
  {  // non-constexpr
    const turbo::optional<DeletedOpAddr<v>> opt(turbo::in_place_t{});
    EXPECT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->value == v);
    EXPECT_TRUE((*opt).value == v);
  }
}

TEST(optionalTest, PointerStuff) {
  turbo::optional<std::string> opt(turbo::in_place, "foo");
  EXPECT_EQ("foo", *opt);
  const auto& opt_const = opt;
  EXPECT_EQ("foo", *opt_const);
  EXPECT_EQ(opt->size(), 3u);
  EXPECT_EQ(opt_const->size(), 3u);

  constexpr turbo::optional<ConstexprType> opt1(1);
  static_assert((*opt1).x == ConstexprType::kCtorInt, "");
}

TEST(optionalTest, Value) {
  using O = turbo::optional<std::string>;
  using CO = const turbo::optional<std::string>;
  using OC = turbo::optional<const std::string>;
  O lvalue(turbo::in_place, "lvalue");
  CO clvalue(turbo::in_place, "clvalue");
  OC lvalue_c(turbo::in_place, "lvalue_c");
  EXPECT_EQ("lvalue", lvalue.value());
  EXPECT_EQ("clvalue", clvalue.value());
  EXPECT_EQ("lvalue_c", lvalue_c.value());
  EXPECT_EQ("xvalue", O(turbo::in_place, "xvalue").value());
  EXPECT_EQ("xvalue_c", OC(turbo::in_place, "xvalue_c").value());
  EXPECT_EQ("cxvalue", CO(turbo::in_place, "cxvalue").value());
  EXPECT_EQ("&", TypeQuals(lvalue.value()));
  EXPECT_EQ("c&", TypeQuals(clvalue.value()));
  EXPECT_EQ("c&", TypeQuals(lvalue_c.value()));
  EXPECT_EQ("&&", TypeQuals(O(turbo::in_place, "xvalue").value()));
  EXPECT_EQ("c&&", TypeQuals(CO(turbo::in_place, "cxvalue").value()));
  EXPECT_EQ("c&&", TypeQuals(OC(turbo::in_place, "xvalue_c").value()));

#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // test on volatile type
  using OV = turbo::optional<volatile int>;
  OV lvalue_v(turbo::in_place, 42);
  EXPECT_EQ(42, lvalue_v.value());
  EXPECT_EQ(42, OV(42).value());
  EXPECT_TRUE((std::is_same<volatile int&, decltype(lvalue_v.value())>::value));
  EXPECT_TRUE((std::is_same<volatile int&&, decltype(OV(42).value())>::value));
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  // test exception throw on value()
  turbo::optional<int> empty;
#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW((void)empty.value(), turbo::bad_optional_access);
#else
  EXPECT_DEATH_IF_SUPPORTED((void)empty.value(), "Bad optional access");
#endif

  // test constexpr value()
  constexpr turbo::optional<int> o1(1);
  static_assert(1 == o1.value(), "");  // const &
#ifndef _MSC_VER
  using COI = const turbo::optional<int>;
  static_assert(2 == COI(2).value(), "");  // const &&
#endif
}

TEST(optionalTest, DerefOperator) {
  using O = turbo::optional<std::string>;
  using CO = const turbo::optional<std::string>;
  using OC = turbo::optional<const std::string>;
  O lvalue(turbo::in_place, "lvalue");
  CO clvalue(turbo::in_place, "clvalue");
  OC lvalue_c(turbo::in_place, "lvalue_c");
  EXPECT_EQ("lvalue", *lvalue);
  EXPECT_EQ("clvalue", *clvalue);
  EXPECT_EQ("lvalue_c", *lvalue_c);
  EXPECT_EQ("xvalue", *O(turbo::in_place, "xvalue"));
  EXPECT_EQ("xvalue_c", *OC(turbo::in_place, "xvalue_c"));
  EXPECT_EQ("cxvalue", *CO(turbo::in_place, "cxvalue"));
  EXPECT_EQ("&", TypeQuals(*lvalue));
  EXPECT_EQ("c&", TypeQuals(*clvalue));
  EXPECT_EQ("&&", TypeQuals(*O(turbo::in_place, "xvalue")));
  EXPECT_EQ("c&&", TypeQuals(*CO(turbo::in_place, "cxvalue")));
  EXPECT_EQ("c&&", TypeQuals(*OC(turbo::in_place, "xvalue_c")));

#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // test on volatile type
  using OV = turbo::optional<volatile int>;
  OV lvalue_v(turbo::in_place, 42);
  EXPECT_EQ(42, *lvalue_v);
  EXPECT_EQ(42, *OV(42));
  EXPECT_TRUE((std::is_same<volatile int&, decltype(*lvalue_v)>::value));
  EXPECT_TRUE((std::is_same<volatile int&&, decltype(*OV(42))>::value));
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  constexpr turbo::optional<int> opt1(1);
  static_assert(*opt1 == 1, "");
#if !defined(_MSC_VER) && !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
  using COI = const turbo::optional<int>;
  static_assert(*COI(2) == 2, "");
#endif
}

TEST(optionalTest, ValueOr) {
  turbo::optional<double> opt_empty, opt_set = 1.2;
  EXPECT_EQ(42.0, opt_empty.value_or(42));
  EXPECT_EQ(1.2, opt_set.value_or(42));
  EXPECT_EQ(42.0, turbo::optional<double>().value_or(42));
  EXPECT_EQ(1.2, turbo::optional<double>(1.2).value_or(42));

  constexpr turbo::optional<double> copt_empty, copt_set = {1.2};
  static_assert(42.0 == copt_empty.value_or(42), "");
  static_assert(1.2 == copt_set.value_or(42), "");
  using COD = const turbo::optional<double>;
  static_assert(42.0 == COD().value_or(42), "");
  static_assert(1.2 == COD(1.2).value_or(42), "");
}

// make_optional cannot be constexpr until C++17
TEST(optionalTest, make_optional) {
  auto opt_int = turbo::make_optional(42);
  EXPECT_TRUE((std::is_same<decltype(opt_int), turbo::optional<int>>::value));
  EXPECT_EQ(42, opt_int);

  StructorListener listener;
  Listenable::listener = &listener;

  turbo::optional<Listenable> opt0 = turbo::make_optional<Listenable>();
  EXPECT_EQ(1, listener.construct0);
  turbo::optional<Listenable> opt1 = turbo::make_optional<Listenable>(1);
  EXPECT_EQ(1, listener.construct1);
  turbo::optional<Listenable> opt2 = turbo::make_optional<Listenable>(1, 2);
  EXPECT_EQ(1, listener.construct2);
  turbo::optional<Listenable> opt3 = turbo::make_optional<Listenable>({1});
  turbo::optional<Listenable> opt4 = turbo::make_optional<Listenable>({1, 2});
  EXPECT_EQ(2, listener.listinit);

  // Constexpr tests on trivially copyable types
  // optional<T> has trivial copy/move ctors when T is trivially copyable.
  // For nontrivial types with constexpr constructors, we need copy elision in
  // C++17 for make_optional to be constexpr.
  {
    constexpr turbo::optional<int> c_opt = turbo::make_optional(42);
    static_assert(c_opt.value() == 42, "");
  }
  {
    struct TrivialCopyable {
      constexpr TrivialCopyable() : x(0) {}
      constexpr explicit TrivialCopyable(int i) : x(i) {}
      int x;
    };

    constexpr TrivialCopyable v;
    constexpr turbo::optional<TrivialCopyable> c_opt0 = turbo::make_optional(v);
    static_assert((*c_opt0).x == 0, "");
    constexpr turbo::optional<TrivialCopyable> c_opt1 =
        turbo::make_optional<TrivialCopyable>();
    static_assert((*c_opt1).x == 0, "");
    constexpr turbo::optional<TrivialCopyable> c_opt2 =
        turbo::make_optional<TrivialCopyable>(42);
    static_assert((*c_opt2).x == 42, "");
  }
}

template <typename T, typename U>
void optionalTest_Comparisons_EXPECT_LESS(T x, U y) {
  EXPECT_FALSE(x == y);
  EXPECT_TRUE(x != y);
  EXPECT_TRUE(x < y);
  EXPECT_FALSE(x > y);
  EXPECT_TRUE(x <= y);
  EXPECT_FALSE(x >= y);
}

template <typename T, typename U>
void optionalTest_Comparisons_EXPECT_SAME(T x, U y) {
  EXPECT_TRUE(x == y);
  EXPECT_FALSE(x != y);
  EXPECT_FALSE(x < y);
  EXPECT_FALSE(x > y);
  EXPECT_TRUE(x <= y);
  EXPECT_TRUE(x >= y);
}

template <typename T, typename U>
void optionalTest_Comparisons_EXPECT_GREATER(T x, U y) {
  EXPECT_FALSE(x == y);
  EXPECT_TRUE(x != y);
  EXPECT_FALSE(x < y);
  EXPECT_TRUE(x > y);
  EXPECT_FALSE(x <= y);
  EXPECT_TRUE(x >= y);
}

template <typename T, typename U, typename V>
void TestComparisons() {
  turbo::optional<T> ae, a2{2}, a4{4};
  turbo::optional<U> be, b2{2}, b4{4};
  V v3 = 3;

  // LHS: turbo::nullopt, ae, a2, v3, a4
  // RHS: turbo::nullopt, be, b2, v3, b4

  // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(turbo::nullopt,turbo::nullopt);
  optionalTest_Comparisons_EXPECT_SAME(turbo::nullopt, be);
  optionalTest_Comparisons_EXPECT_LESS(turbo::nullopt, b2);
  // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(turbo::nullopt,v3);
  optionalTest_Comparisons_EXPECT_LESS(turbo::nullopt, b4);

  optionalTest_Comparisons_EXPECT_SAME(ae, turbo::nullopt);
  optionalTest_Comparisons_EXPECT_SAME(ae, be);
  optionalTest_Comparisons_EXPECT_LESS(ae, b2);
  optionalTest_Comparisons_EXPECT_LESS(ae, v3);
  optionalTest_Comparisons_EXPECT_LESS(ae, b4);

  optionalTest_Comparisons_EXPECT_GREATER(a2, turbo::nullopt);
  optionalTest_Comparisons_EXPECT_GREATER(a2, be);
  optionalTest_Comparisons_EXPECT_SAME(a2, b2);
  optionalTest_Comparisons_EXPECT_LESS(a2, v3);
  optionalTest_Comparisons_EXPECT_LESS(a2, b4);

  // optionalTest_Comparisons_EXPECT_NOT_TO_WORK(v3,turbo::nullopt);
  optionalTest_Comparisons_EXPECT_GREATER(v3, be);
  optionalTest_Comparisons_EXPECT_GREATER(v3, b2);
  optionalTest_Comparisons_EXPECT_SAME(v3, v3);
  optionalTest_Comparisons_EXPECT_LESS(v3, b4);

  optionalTest_Comparisons_EXPECT_GREATER(a4, turbo::nullopt);
  optionalTest_Comparisons_EXPECT_GREATER(a4, be);
  optionalTest_Comparisons_EXPECT_GREATER(a4, b2);
  optionalTest_Comparisons_EXPECT_GREATER(a4, v3);
  optionalTest_Comparisons_EXPECT_SAME(a4, b4);
}

struct Int1 {
  Int1() = default;
  Int1(int i) : i(i) {}  // NOLINT(runtime/explicit)
  int i;
};

struct Int2 {
  Int2() = default;
  Int2(int i) : i(i) {}  // NOLINT(runtime/explicit)
  int i;
};

// comparison between Int1 and Int2
constexpr bool operator==(const Int1& lhs, const Int2& rhs) {
  return lhs.i == rhs.i;
}
constexpr bool operator!=(const Int1& lhs, const Int2& rhs) {
  return !(lhs == rhs);
}
constexpr bool operator<(const Int1& lhs, const Int2& rhs) {
  return lhs.i < rhs.i;
}
constexpr bool operator<=(const Int1& lhs, const Int2& rhs) {
  return lhs < rhs || lhs == rhs;
}
constexpr bool operator>(const Int1& lhs, const Int2& rhs) {
  return !(lhs <= rhs);
}
constexpr bool operator>=(const Int1& lhs, const Int2& rhs) {
  return !(lhs < rhs);
}

TEST(optionalTest, Comparisons) {
  TestComparisons<int, int, int>();
  TestComparisons<const int, int, int>();
  TestComparisons<Int1, int, int>();
  TestComparisons<int, Int2, int>();
  TestComparisons<Int1, Int2, int>();

  // compare turbo::optional<std::string> with const char*
  turbo::optional<std::string> opt_str = "abc";
  const char* cstr = "abc";
  EXPECT_TRUE(opt_str == cstr);
  // compare turbo::optional<std::string> with turbo::optional<const char*>
  turbo::optional<const char*> opt_cstr = cstr;
  EXPECT_TRUE(opt_str == opt_cstr);
  // compare turbo::optional<std::string> with turbo::optional<turbo::string_view>
  turbo::optional<turbo::string_view> e1;
  turbo::optional<std::string> e2;
  EXPECT_TRUE(e1 == e2);
}

TEST(optionalTest, SwapRegression) {
  StructorListener listener;
  Listenable::listener = &listener;

  {
    turbo::optional<Listenable> a;
    turbo::optional<Listenable> b(turbo::in_place);
    a.swap(b);
  }

  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.move);
  EXPECT_EQ(2, listener.destruct);

  {
    turbo::optional<Listenable> a(turbo::in_place);
    turbo::optional<Listenable> b;
    a.swap(b);
  }

  EXPECT_EQ(2, listener.construct0);
  EXPECT_EQ(2, listener.move);
  EXPECT_EQ(4, listener.destruct);
}

TEST(optionalTest, BigStringLeakCheck) {
  constexpr size_t n = 1 << 16;

  using OS = turbo::optional<std::string>;

  OS a;
  OS b = turbo::nullopt;
  OS c = std::string(n, 'c');
  std::string sd(n, 'd');
  OS d = sd;
  OS e(turbo::in_place, n, 'e');
  OS f;
  f.emplace(n, 'f');

  OS ca(a);
  OS cb(b);
  OS cc(c);
  OS cd(d);
  OS ce(e);

  OS oa;
  OS ob = turbo::nullopt;
  OS oc = std::string(n, 'c');
  std::string sod(n, 'd');
  OS od = sod;
  OS oe(turbo::in_place, n, 'e');
  OS of;
  of.emplace(n, 'f');

  OS ma(std::move(oa));
  OS mb(std::move(ob));
  OS mc(std::move(oc));
  OS md(std::move(od));
  OS me(std::move(oe));
  OS mf(std::move(of));

  OS aa1;
  OS ab1 = turbo::nullopt;
  OS ac1 = std::string(n, 'c');
  std::string sad1(n, 'd');
  OS ad1 = sad1;
  OS ae1(turbo::in_place, n, 'e');
  OS af1;
  af1.emplace(n, 'f');

  OS aa2;
  OS ab2 = turbo::nullopt;
  OS ac2 = std::string(n, 'c');
  std::string sad2(n, 'd');
  OS ad2 = sad2;
  OS ae2(turbo::in_place, n, 'e');
  OS af2;
  af2.emplace(n, 'f');

  aa1 = af2;
  ab1 = ae2;
  ac1 = ad2;
  ad1 = ac2;
  ae1 = ab2;
  af1 = aa2;

  OS aa3;
  OS ab3 = turbo::nullopt;
  OS ac3 = std::string(n, 'c');
  std::string sad3(n, 'd');
  OS ad3 = sad3;
  OS ae3(turbo::in_place, n, 'e');
  OS af3;
  af3.emplace(n, 'f');

  aa3 = turbo::nullopt;
  ab3 = turbo::nullopt;
  ac3 = turbo::nullopt;
  ad3 = turbo::nullopt;
  ae3 = turbo::nullopt;
  af3 = turbo::nullopt;

  OS aa4;
  OS ab4 = turbo::nullopt;
  OS ac4 = std::string(n, 'c');
  std::string sad4(n, 'd');
  OS ad4 = sad4;
  OS ae4(turbo::in_place, n, 'e');
  OS af4;
  af4.emplace(n, 'f');

  aa4 = OS(turbo::in_place, n, 'a');
  ab4 = OS(turbo::in_place, n, 'b');
  ac4 = OS(turbo::in_place, n, 'c');
  ad4 = OS(turbo::in_place, n, 'd');
  ae4 = OS(turbo::in_place, n, 'e');
  af4 = OS(turbo::in_place, n, 'f');

  OS aa5;
  OS ab5 = turbo::nullopt;
  OS ac5 = std::string(n, 'c');
  std::string sad5(n, 'd');
  OS ad5 = sad5;
  OS ae5(turbo::in_place, n, 'e');
  OS af5;
  af5.emplace(n, 'f');

  std::string saa5(n, 'a');
  std::string sab5(n, 'a');
  std::string sac5(n, 'a');
  std::string sad52(n, 'a');
  std::string sae5(n, 'a');
  std::string saf5(n, 'a');

  aa5 = saa5;
  ab5 = sab5;
  ac5 = sac5;
  ad5 = sad52;
  ae5 = sae5;
  af5 = saf5;

  OS aa6;
  OS ab6 = turbo::nullopt;
  OS ac6 = std::string(n, 'c');
  std::string sad6(n, 'd');
  OS ad6 = sad6;
  OS ae6(turbo::in_place, n, 'e');
  OS af6;
  af6.emplace(n, 'f');

  aa6 = std::string(n, 'a');
  ab6 = std::string(n, 'b');
  ac6 = std::string(n, 'c');
  ad6 = std::string(n, 'd');
  ae6 = std::string(n, 'e');
  af6 = std::string(n, 'f');

  OS aa7;
  OS ab7 = turbo::nullopt;
  OS ac7 = std::string(n, 'c');
  std::string sad7(n, 'd');
  OS ad7 = sad7;
  OS ae7(turbo::in_place, n, 'e');
  OS af7;
  af7.emplace(n, 'f');

  aa7.emplace(n, 'A');
  ab7.emplace(n, 'B');
  ac7.emplace(n, 'C');
  ad7.emplace(n, 'D');
  ae7.emplace(n, 'E');
  af7.emplace(n, 'F');
}

TEST(optionalTest, MoveAssignRegression) {
  StructorListener listener;
  Listenable::listener = &listener;

  {
    turbo::optional<Listenable> a;
    Listenable b;
    a = std::move(b);
  }

  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.move);
  EXPECT_EQ(2, listener.destruct);
}

TEST(optionalTest, ValueType) {
  EXPECT_TRUE((std::is_same<turbo::optional<int>::value_type, int>::value));
  EXPECT_TRUE((std::is_same<turbo::optional<std::string>::value_type,
                            std::string>::value));
  EXPECT_FALSE(
      (std::is_same<turbo::optional<int>::value_type, turbo::nullopt_t>::value));
}

template <typename T>
struct is_hash_enabled_for {
  template <typename U, typename = decltype(std::hash<U>()(std::declval<U>()))>
  static std::true_type test(int);

  template <typename U>
  static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(0))::value;
};

TEST(optionalTest, Hash) {
  std::hash<turbo::optional<int>> hash;
  std::set<size_t> hashcodes;
  hashcodes.insert(hash(turbo::nullopt));
  for (int i = 0; i < 100; ++i) {
    hashcodes.insert(hash(i));
  }
  EXPECT_GT(hashcodes.size(), 90u);

  static_assert(is_hash_enabled_for<turbo::optional<int>>::value, "");
  static_assert(is_hash_enabled_for<turbo::optional<Hashable>>::value, "");
  static_assert(
      turbo::type_traits_internal::IsHashable<turbo::optional<int>>::value, "");
  static_assert(
      turbo::type_traits_internal::IsHashable<turbo::optional<Hashable>>::value,
      "");
  turbo::type_traits_internal::AssertHashEnabled<turbo::optional<int>>();
  turbo::type_traits_internal::AssertHashEnabled<turbo::optional<Hashable>>();

#if TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
  static_assert(!is_hash_enabled_for<turbo::optional<NonHashable>>::value, "");
  static_assert(!turbo::type_traits_internal::IsHashable<
                    turbo::optional<NonHashable>>::value,
                "");
#endif

  // libstdc++ std::optional is missing remove_const_t, i.e. it's using
  // std::hash<T> rather than std::hash<std::remove_const_t<T>>.
  // Reference: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82262
#ifndef __GLIBCXX__
  static_assert(is_hash_enabled_for<turbo::optional<const int>>::value, "");
  static_assert(is_hash_enabled_for<turbo::optional<const Hashable>>::value, "");
  std::hash<turbo::optional<const int>> c_hash;
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(hash(i), c_hash(i));
  }
#endif
}

struct MoveMeNoThrow {
  MoveMeNoThrow() : x(0) {}
  [[noreturn]] MoveMeNoThrow(const MoveMeNoThrow& other) : x(other.x) {
    LOG(FATAL) << "Should not be called.";
  }
  MoveMeNoThrow(MoveMeNoThrow&& other) noexcept : x(other.x) {}
  int x;
};

struct MoveMeThrow {
  MoveMeThrow() : x(0) {}
  MoveMeThrow(const MoveMeThrow& other) : x(other.x) {}
  MoveMeThrow(MoveMeThrow&& other) : x(other.x) {}
  int x;
};

TEST(optionalTest, NoExcept) {
  static_assert(
      std::is_nothrow_move_constructible<turbo::optional<MoveMeNoThrow>>::value,
      "");
  static_assert(turbo::default_allocator_is_nothrow::value ==
                    std::is_nothrow_move_constructible<
                        turbo::optional<MoveMeThrow>>::value,
                "");
  std::vector<turbo::optional<MoveMeNoThrow>> v;
  for (int i = 0; i < 10; ++i) v.emplace_back();
}

struct AnyLike {
  AnyLike(AnyLike&&) = default;
  AnyLike(const AnyLike&) = default;

  template <typename ValueType,
            typename T = typename std::decay<ValueType>::type,
            typename std::enable_if<
                !turbo::disjunction<
                    std::is_same<AnyLike, T>,
                    turbo::negation<std::is_copy_constructible<T>>>::value,
                int>::type = 0>
  AnyLike(ValueType&&) {}  // NOLINT(runtime/explicit)

  AnyLike& operator=(AnyLike&&) = default;
  AnyLike& operator=(const AnyLike&) = default;

  template <typename ValueType,
            typename T = typename std::decay<ValueType>::type>
  typename std::enable_if<
      turbo::conjunction<turbo::negation<std::is_same<AnyLike, T>>,
                        std::is_copy_constructible<T>>::value,
      AnyLike&>::type
  operator=(ValueType&& /* rhs */) {
    return *this;
  }
};

TEST(optionalTest, ConstructionConstraints) {
  EXPECT_TRUE((std::is_constructible<AnyLike, turbo::optional<AnyLike>>::value));

  EXPECT_TRUE(
      (std::is_constructible<AnyLike, const turbo::optional<AnyLike>&>::value));

  EXPECT_TRUE((std::is_constructible<turbo::optional<AnyLike>, AnyLike>::value));
  EXPECT_TRUE(
      (std::is_constructible<turbo::optional<AnyLike>, const AnyLike&>::value));

  EXPECT_TRUE((std::is_convertible<turbo::optional<AnyLike>, AnyLike>::value));

  EXPECT_TRUE(
      (std::is_convertible<const turbo::optional<AnyLike>&, AnyLike>::value));

  EXPECT_TRUE((std::is_convertible<AnyLike, turbo::optional<AnyLike>>::value));
  EXPECT_TRUE(
      (std::is_convertible<const AnyLike&, turbo::optional<AnyLike>>::value));

  EXPECT_TRUE(std::is_move_constructible<turbo::optional<AnyLike>>::value);
  EXPECT_TRUE(std::is_copy_constructible<turbo::optional<AnyLike>>::value);
}

TEST(optionalTest, AssignmentConstraints) {
  EXPECT_TRUE((std::is_assignable<AnyLike&, turbo::optional<AnyLike>>::value));
  EXPECT_TRUE(
      (std::is_assignable<AnyLike&, const turbo::optional<AnyLike>&>::value));
  EXPECT_TRUE((std::is_assignable<turbo::optional<AnyLike>&, AnyLike>::value));
  EXPECT_TRUE(
      (std::is_assignable<turbo::optional<AnyLike>&, const AnyLike&>::value));
  EXPECT_TRUE(std::is_move_assignable<turbo::optional<AnyLike>>::value);
  EXPECT_TRUE(turbo::is_copy_assignable<turbo::optional<AnyLike>>::value);
}

#if !defined(__EMSCRIPTEN__)
struct NestedClassBug {
  struct Inner {
    bool dummy = false;
  };
  turbo::optional<Inner> value;
};

TEST(optionalTest, InPlaceTSFINAEBug) {
  NestedClassBug b;
  ((void)b);
  using Inner = NestedClassBug::Inner;

  EXPECT_TRUE((std::is_default_constructible<Inner>::value));
  EXPECT_TRUE((std::is_constructible<Inner>::value));
  EXPECT_TRUE(
      (std::is_constructible<turbo::optional<Inner>, turbo::in_place_t>::value));

  turbo::optional<Inner> o(turbo::in_place);
  EXPECT_TRUE(o.has_value());
  o.emplace();
  EXPECT_TRUE(o.has_value());
}
#endif  // !defined(__EMSCRIPTEN__)

}  // namespace

#endif  // #if !defined(TURBO_USES_STD_OPTIONAL)
