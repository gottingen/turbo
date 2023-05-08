// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "optional.h"

// This test is a no-op when std::optional is an alias for std::optional.
#if !defined(TURBO_USES_STD_OPTIONAL)

#include <string>
#include <type_traits>
#include <utility>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/meta/type_traits.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"
#include "gtest/gtest.h"

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
  std::optional<int> val1;
  std::optional<int> val2;
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
#pragma warning( push )
#pragma warning( disable : 4521)
#pragma warning( disable : 4522)
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
#pragma warning( pop )
#endif

StructorListener* Listenable::listener = nullptr;

// TURBO_HAVE_NO_CONSTEXPR_INITIALIZER_LIST is defined to 1 when the standard
// library implementation doesn't marked initializer_list's default constructor
// constexpr. The C++11 standard doesn't specify constexpr on it, but C++14
// added it. However, libstdc++ 4.7 marked it constexpr.
#if defined(_LIBCPP_VERSION) && \
    (_LIBCPP_STD_VER <= 11 || defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR))
#define TURBO_HAVE_NO_CONSTEXPR_INITIALIZER_LIST 1
#endif

struct ConstexprType {
  enum CtorTypes {
    kCtorDefault,
    kCtorInt,
    kCtorInitializerList,
    kCtorConstChar
  };
  constexpr ConstexprType() : x(kCtorDefault) {}
  constexpr explicit ConstexprType(int i) : x(kCtorInt) {}
#ifndef TURBO_HAVE_NO_CONSTEXPR_INITIALIZER_LIST
  constexpr ConstexprType(std::initializer_list<int> il)
      : x(kCtorInitializerList) {}
#endif
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
  std::optional<int> empty;
  EXPECT_FALSE(empty);
  constexpr std::optional<int> cempty;
  static_assert(!cempty.has_value(), "");
  EXPECT_TRUE(
      std::is_nothrow_default_constructible<std::optional<int>>::value);
}

TEST(optionalTest, nulloptConstructor) {
  std::optional<int> empty(turbo::nullopt);
  EXPECT_FALSE(empty);
  constexpr std::optional<int> cempty{turbo::nullopt};
  static_assert(!cempty.has_value(), "");
  EXPECT_TRUE((std::is_nothrow_constructible<std::optional<int>,
                                             turbo::nullopt_t>::value));
}

TEST(optionalTest, CopyConstructor) {
  {
    std::optional<int> empty, opt42 = 42;
    std::optional<int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    std::optional<int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
  {
    std::optional<const int> empty, opt42 = 42;
    std::optional<const int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    std::optional<const int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  {
    std::optional<volatile int> empty, opt42 = 42;
    std::optional<volatile int> empty_copy(empty);
    EXPECT_FALSE(empty_copy);
    std::optional<volatile int> opt42_copy(opt42);
    EXPECT_TRUE(opt42_copy);
    EXPECT_EQ(42, *opt42_copy);
  }
#endif
  // test copyablility
  EXPECT_TRUE(std::is_copy_constructible<std::optional<int>>::value);
  EXPECT_TRUE(std::is_copy_constructible<std::optional<Copyable>>::value);
  EXPECT_FALSE(
      std::is_copy_constructible<std::optional<MoveableThrow>>::value);
  EXPECT_FALSE(
      std::is_copy_constructible<std::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(std::is_copy_constructible<std::optional<NonMovable>>::value);

  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<std::optional<Copyable>>::value);
  EXPECT_TRUE(
      turbo::is_trivially_copy_constructible<std::optional<int>>::value);
  EXPECT_TRUE(
      turbo::is_trivially_copy_constructible<std::optional<const int>>::value);
#if !defined(_MSC_VER) && !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // See defect report "Trivial copy/move constructor for class with volatile
  // member" at
  // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#2094
  // A class with non-static data member of volatile-qualified type should still
  // have a trivial copy constructor if the data member is trivial.
  // Also a cv-qualified scalar type should be trivially copyable.
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<
              std::optional<volatile int>>::value);
#endif  // !defined(_MSC_VER) && !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  // constexpr copy constructor for trivially copyable types
  {
    constexpr std::optional<int> o1;
    constexpr std::optional<int> o2 = o1;
    static_assert(!o2, "");
  }
  {
    constexpr std::optional<int> o1 = 42;
    constexpr std::optional<int> o2 = o1;
    static_assert(o2, "");
    static_assert(*o2 == 42, "");
  }
  {
    struct TrivialCopyable {
      constexpr TrivialCopyable() : x(0) {}
      constexpr explicit TrivialCopyable(int i) : x(i) {}
      int x;
    };
    constexpr std::optional<TrivialCopyable> o1(42);
    constexpr std::optional<TrivialCopyable> o2 = o1;
    static_assert(o2, "");
    static_assert((*o2).x == 42, "");
#ifndef TURBO_GLIBCXX_OPTIONAL_TRIVIALITY_BUG
    EXPECT_TRUE(turbo::is_trivially_copy_constructible<
                std::optional<TrivialCopyable>>::value);
    EXPECT_TRUE(turbo::is_trivially_copy_constructible<
                std::optional<const TrivialCopyable>>::value);
#endif
#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
    EXPECT_FALSE(std::is_copy_constructible<
                 std::optional<volatile TrivialCopyable>>::value);
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  }
}

TEST(optionalTest, MoveConstructor) {
  std::optional<int> empty, opt42 = 42;
  std::optional<int> empty_move(std::move(empty));
  EXPECT_FALSE(empty_move);
  std::optional<int> opt42_move(std::move(opt42));
  EXPECT_TRUE(opt42_move);
  EXPECT_EQ(42, opt42_move);
  // test movability
  EXPECT_TRUE(std::is_move_constructible<std::optional<int>>::value);
  EXPECT_TRUE(std::is_move_constructible<std::optional<Copyable>>::value);
  EXPECT_TRUE(std::is_move_constructible<std::optional<MoveableThrow>>::value);
  EXPECT_TRUE(
      std::is_move_constructible<std::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(std::is_move_constructible<std::optional<NonMovable>>::value);
  // test noexcept
  EXPECT_TRUE(std::is_nothrow_move_constructible<std::optional<int>>::value);
  EXPECT_EQ(
      turbo::default_allocator_is_nothrow::value,
      std::is_nothrow_move_constructible<std::optional<MoveableThrow>>::value);
  EXPECT_TRUE(std::is_nothrow_move_constructible<
              std::optional<MoveableNoThrow>>::value);
}

TEST(optionalTest, Destructor) {
  struct Trivial {};

  struct NonTrivial {
    NonTrivial(const NonTrivial&) {}
    NonTrivial& operator=(const NonTrivial&) { return *this; }
    ~NonTrivial() {}
  };

  EXPECT_TRUE(std::is_trivially_destructible<std::optional<int>>::value);
  EXPECT_TRUE(std::is_trivially_destructible<std::optional<Trivial>>::value);
  EXPECT_FALSE(
      std::is_trivially_destructible<std::optional<NonTrivial>>::value);
}

TEST(optionalTest, InPlaceConstructor) {
  constexpr std::optional<ConstexprType> opt0{turbo::in_place_t()};
  static_assert(opt0, "");
  static_assert((*opt0).x == ConstexprType::kCtorDefault, "");
  constexpr std::optional<ConstexprType> opt1{turbo::in_place_t(), 1};
  static_assert(opt1, "");
  static_assert((*opt1).x == ConstexprType::kCtorInt, "");
#ifndef TURBO_HAVE_NO_CONSTEXPR_INITIALIZER_LIST
  constexpr std::optional<ConstexprType> opt2{turbo::in_place_t(), {1, 2}};
  static_assert(opt2, "");
  static_assert((*opt2).x == ConstexprType::kCtorInitializerList, "");
#endif

  EXPECT_FALSE((std::is_constructible<std::optional<ConvertsFromInPlaceT>,
                                      turbo::in_place_t>::value));
  EXPECT_FALSE((std::is_constructible<std::optional<ConvertsFromInPlaceT>,
                                      const turbo::in_place_t&>::value));
  EXPECT_TRUE(
      (std::is_constructible<std::optional<ConvertsFromInPlaceT>,
                             turbo::in_place_t, turbo::in_place_t>::value));

  EXPECT_FALSE((std::is_constructible<std::optional<NoDefault>,
                                      turbo::in_place_t>::value));
  EXPECT_FALSE((std::is_constructible<std::optional<NoDefault>,
                                      turbo::in_place_t&&>::value));
}

// template<U=T> optional(U&&);
TEST(optionalTest, ValueConstructor) {
  constexpr std::optional<int> opt0(0);
  static_assert(opt0, "");
  static_assert(*opt0 == 0, "");
  EXPECT_TRUE((std::is_convertible<int, std::optional<int>>::value));
  // Copy initialization ( = "abc") won't work due to optional(optional&&)
  // is not constexpr. Use list initialization instead. This invokes
  // std::optional<ConstexprType>::std::optional<U>(U&&), with U = const char
  // (&) [4], which direct-initializes the ConstexprType value held by the
  // optional via ConstexprType::ConstexprType(const char*).
  constexpr std::optional<ConstexprType> opt1 = {"abc"};
  static_assert(opt1, "");
  static_assert(ConstexprType::kCtorConstChar == (*opt1).x, "");
  EXPECT_TRUE(
      (std::is_convertible<const char*, std::optional<ConstexprType>>::value));
  // direct initialization
  constexpr std::optional<ConstexprType> opt2{2};
  static_assert(opt2, "");
  static_assert(ConstexprType::kCtorInt == (*opt2).x, "");
  EXPECT_FALSE(
      (std::is_convertible<int, std::optional<ConstexprType>>::value));

  // this invokes std::optional<int>::optional(int&&)
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
  constexpr std::optional<int> opt3({});
  static_assert(opt3, "");
  static_assert(*opt3 == 0, "");
#endif

  // this invokes the move constructor with a default constructed optional
  // because non-template function is a better match than template function.
  std::optional<ConstexprType> opt4({});
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
      const std::optional<Implicit>&)  // NOLINT(runtime/explicit)
      : implicit(true), move(false), from_optional(true) {}
  ConvertFromOptional(std::optional<Implicit>&&)  // NOLINT(runtime/explicit)
      : implicit(true), move(true), from_optional(true) {}
  explicit ConvertFromOptional(const Explicit&)
      : implicit(false), move(false), from_optional(false) {}
  explicit ConvertFromOptional(Explicit&&)
      : implicit(false), move(true), from_optional(false) {}
  explicit ConvertFromOptional(const std::optional<Explicit>&)
      : implicit(false), move(false), from_optional(true) {}
  explicit ConvertFromOptional(std::optional<Explicit>&&)
      : implicit(false), move(true), from_optional(true) {}

  bool implicit;
  bool move;
  bool from_optional;
};

TEST(optionalTest, ConvertingConstructor) {
  std::optional<Implicit> i_empty;
  std::optional<Implicit> i(turbo::in_place);
  std::optional<Explicit> e_empty;
  std::optional<Explicit> e(turbo::in_place);
  {
    // implicitly constructing std::optional<Convert> from
    // std::optional<Implicit>
    std::optional<Convert> empty = i_empty;
    EXPECT_FALSE(empty);
    std::optional<Convert> opt_copy = i;
    EXPECT_TRUE(opt_copy);
    EXPECT_TRUE(opt_copy->implicit);
    EXPECT_FALSE(opt_copy->move);
    std::optional<Convert> opt_move = std::optional<Implicit>(turbo::in_place);
    EXPECT_TRUE(opt_move);
    EXPECT_TRUE(opt_move->implicit);
    EXPECT_TRUE(opt_move->move);
  }
  {
    // explicitly constructing std::optional<Convert> from
    // std::optional<Explicit>
    std::optional<Convert> empty(e_empty);
    EXPECT_FALSE(empty);
    std::optional<Convert> opt_copy(e);
    EXPECT_TRUE(opt_copy);
    EXPECT_FALSE(opt_copy->implicit);
    EXPECT_FALSE(opt_copy->move);
    EXPECT_FALSE((std::is_convertible<const std::optional<Explicit>&,
                                      std::optional<Convert>>::value));
    std::optional<Convert> opt_move{std::optional<Explicit>(turbo::in_place)};
    EXPECT_TRUE(opt_move);
    EXPECT_FALSE(opt_move->implicit);
    EXPECT_TRUE(opt_move->move);
    EXPECT_FALSE((std::is_convertible<std::optional<Explicit>&&,
                                      std::optional<Convert>>::value));
  }
  {
    // implicitly constructing std::optional<ConvertFromOptional> from
    // std::optional<Implicit> via
    // ConvertFromOptional(std::optional<Implicit>&&) check that
    // ConvertFromOptional(Implicit&&) is NOT called
    static_assert(
        std::is_convertible<std::optional<Implicit>,
                            std::optional<ConvertFromOptional>>::value,
        "");
    std::optional<ConvertFromOptional> opt0 = i_empty;
    EXPECT_TRUE(opt0);
    EXPECT_TRUE(opt0->implicit);
    EXPECT_FALSE(opt0->move);
    EXPECT_TRUE(opt0->from_optional);
    std::optional<ConvertFromOptional> opt1 = std::optional<Implicit>();
    EXPECT_TRUE(opt1);
    EXPECT_TRUE(opt1->implicit);
    EXPECT_TRUE(opt1->move);
    EXPECT_TRUE(opt1->from_optional);
  }
  {
    // implicitly constructing std::optional<ConvertFromOptional> from
    // std::optional<Explicit> via
    // ConvertFromOptional(std::optional<Explicit>&&) check that
    // ConvertFromOptional(Explicit&&) is NOT called
    std::optional<ConvertFromOptional> opt0(e_empty);
    EXPECT_TRUE(opt0);
    EXPECT_FALSE(opt0->implicit);
    EXPECT_FALSE(opt0->move);
    EXPECT_TRUE(opt0->from_optional);
    EXPECT_FALSE(
        (std::is_convertible<const std::optional<Explicit>&,
                             std::optional<ConvertFromOptional>>::value));
    std::optional<ConvertFromOptional> opt1{std::optional<Explicit>()};
    EXPECT_TRUE(opt1);
    EXPECT_FALSE(opt1->implicit);
    EXPECT_TRUE(opt1->move);
    EXPECT_TRUE(opt1->from_optional);
    EXPECT_FALSE(
        (std::is_convertible<std::optional<Explicit>&&,
                             std::optional<ConvertFromOptional>>::value));
  }
}

TEST(optionalTest, StructorBasic) {
  StructorListener listener;
  Listenable::listener = &listener;
  {
    std::optional<Listenable> empty;
    EXPECT_FALSE(empty);
    std::optional<Listenable> opt0(turbo::in_place);
    EXPECT_TRUE(opt0);
    std::optional<Listenable> opt1(turbo::in_place, 1);
    EXPECT_TRUE(opt1);
    std::optional<Listenable> opt2(turbo::in_place, 1, 2);
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
  std::optional<Listenable> original(turbo::in_place);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(0, listener.copy);
  EXPECT_EQ(0, listener.move);
  std::optional<Listenable> copy(original);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.copy);
  EXPECT_EQ(0, listener.move);
  std::optional<Listenable> move(std::move(original));
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.copy);
  EXPECT_EQ(1, listener.move);
}

TEST(optionalTest, ListInit) {
  StructorListener listener;
  Listenable::listener = &listener;
  std::optional<Listenable> listinit1(turbo::in_place, {1});
  std::optional<Listenable> listinit2(turbo::in_place, {1, 2});
  EXPECT_EQ(2, listener.listinit);
}

TEST(optionalTest, AssignFromNullopt) {
  std::optional<int> opt(1);
  opt = turbo::nullopt;
  EXPECT_FALSE(opt);

  StructorListener listener;
  Listenable::listener = &listener;
  std::optional<Listenable> opt1(turbo::in_place);
  opt1 = turbo::nullopt;
  EXPECT_FALSE(opt1);
  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.destruct);

  EXPECT_TRUE((
      std::is_nothrow_assignable<std::optional<int>, turbo::nullopt_t>::value));
  EXPECT_TRUE((std::is_nothrow_assignable<std::optional<Listenable>,
                                          turbo::nullopt_t>::value));
}

TEST(optionalTest, CopyAssignment) {
  const std::optional<int> empty, opt1 = 1, opt2 = 2;
  std::optional<int> empty_to_opt1, opt1_to_opt2, opt2_to_empty;

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

  EXPECT_FALSE(turbo::is_copy_assignable<std::optional<const int>>::value);
  EXPECT_TRUE(turbo::is_copy_assignable<std::optional<Copyable>>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<std::optional<MoveableThrow>>::value);
  EXPECT_FALSE(
      turbo::is_copy_assignable<std::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(turbo::is_copy_assignable<std::optional<NonMovable>>::value);

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

    std::optional<volatile Listenable> empty, set(turbo::in_place);
    EXPECT_EQ(1, listener.construct0);
    std::optional<volatile Listenable> empty_to_empty, empty_to_set,
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

    std::optional<Listenable> empty1, empty2, set1(turbo::in_place),
        set2(turbo::in_place);
    EXPECT_EQ(2, listener.construct0);
    std::optional<Listenable> empty_to_empty, empty_to_set,
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

    std::optional<volatile Listenable> empty1, empty2, set1(turbo::in_place),
        set2(turbo::in_place);
    EXPECT_EQ(2, listener.construct0);
    std::optional<volatile Listenable> empty_to_empty, empty_to_set,
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
  EXPECT_FALSE(turbo::is_move_assignable<std::optional<const int>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<std::optional<Copyable>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<std::optional<MoveableThrow>>::value);
  EXPECT_TRUE(turbo::is_move_assignable<std::optional<MoveableNoThrow>>::value);
  EXPECT_FALSE(turbo::is_move_assignable<std::optional<NonMovable>>::value);

  EXPECT_FALSE(
      std::is_nothrow_move_assignable<std::optional<MoveableThrow>>::value);
  EXPECT_TRUE(
      std::is_nothrow_move_assignable<std::optional<MoveableNoThrow>>::value);
}

struct NoConvertToOptional {
  // disable implicit conversion from const NoConvertToOptional&
  // to std::optional<NoConvertToOptional>.
  NoConvertToOptional(const NoConvertToOptional&) = delete;
};

struct CopyConvert {
  CopyConvert(const NoConvertToOptional&);
  CopyConvert& operator=(const CopyConvert&) = delete;
  CopyConvert& operator=(const NoConvertToOptional&);
};

struct CopyConvertFromOptional {
  CopyConvertFromOptional(const NoConvertToOptional&);
  CopyConvertFromOptional(const std::optional<NoConvertToOptional>&);
  CopyConvertFromOptional& operator=(const CopyConvertFromOptional&) = delete;
  CopyConvertFromOptional& operator=(const NoConvertToOptional&);
  CopyConvertFromOptional& operator=(
      const std::optional<NoConvertToOptional>&);
};

struct MoveConvert {
  MoveConvert(NoConvertToOptional&&);
  MoveConvert& operator=(const MoveConvert&) = delete;
  MoveConvert& operator=(NoConvertToOptional&&);
};

struct MoveConvertFromOptional {
  MoveConvertFromOptional(NoConvertToOptional&&);
  MoveConvertFromOptional(std::optional<NoConvertToOptional>&&);
  MoveConvertFromOptional& operator=(const MoveConvertFromOptional&) = delete;
  MoveConvertFromOptional& operator=(NoConvertToOptional&&);
  MoveConvertFromOptional& operator=(std::optional<NoConvertToOptional>&&);
};

// template <typename U = T> std::optional<T>& operator=(U&& v);
TEST(optionalTest, ValueAssignment) {
  std::optional<int> opt;
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
  EXPECT_TRUE((std::is_assignable<std::optional<CopyConvert>&,
                                  const NoConvertToOptional&>::value));
  // U = const std::optional<NoConvertToOptional>&
  EXPECT_TRUE((std::is_assignable<std::optional<CopyConvertFromOptional>&,
                                  const NoConvertToOptional&>::value));
  // U = const NoConvertToOptional& triggers SFINAE because
  // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
  EXPECT_FALSE((std::is_assignable<std::optional<MoveConvert>&,
                                   const NoConvertToOptional&>::value));
  // U = NoConvertToOptional
  EXPECT_TRUE((std::is_assignable<std::optional<MoveConvert>&,
                                  NoConvertToOptional&&>::value));
  // U = const NoConvertToOptional& triggers SFINAE because
  // std::is_constructible_v<MoveConvertFromOptional, const
  // NoConvertToOptional&> is false
  EXPECT_FALSE((std::is_assignable<std::optional<MoveConvertFromOptional>&,
                                   const NoConvertToOptional&>::value));
  // U = NoConvertToOptional
  EXPECT_TRUE((std::is_assignable<std::optional<MoveConvertFromOptional>&,
                                  NoConvertToOptional&&>::value));
  // U = const std::optional<NoConvertToOptional>&
  EXPECT_TRUE(
      (std::is_assignable<std::optional<CopyConvertFromOptional>&,
                          const std::optional<NoConvertToOptional>&>::value));
  // U = std::optional<NoConvertToOptional>
  EXPECT_TRUE(
      (std::is_assignable<std::optional<MoveConvertFromOptional>&,
                          std::optional<NoConvertToOptional>&&>::value));
}

// template <typename U> std::optional<T>& operator=(const std::optional<U>&
// rhs); template <typename U> std::optional<T>& operator=(std::optional<U>&&
// rhs);
TEST(optionalTest, ConvertingAssignment) {
  std::optional<int> opt_i;
  std::optional<char> opt_c('c');
  opt_i = opt_c;
  EXPECT_TRUE(opt_i);
  EXPECT_EQ(*opt_c, *opt_i);
  opt_i = std::optional<char>();
  EXPECT_FALSE(opt_i);
  opt_i = std::optional<char>('d');
  EXPECT_TRUE(opt_i);
  EXPECT_EQ('d', *opt_i);

  std::optional<std::string> opt_str;
  std::optional<const char*> opt_cstr("abc");
  opt_str = opt_cstr;
  EXPECT_TRUE(opt_str);
  EXPECT_EQ(std::string("abc"), *opt_str);
  opt_str = std::optional<const char*>();
  EXPECT_FALSE(opt_str);
  opt_str = std::optional<const char*>("def");
  EXPECT_TRUE(opt_str);
  EXPECT_EQ(std::string("def"), *opt_str);

  // operator=(const std::optional<U>&) with U = NoConvertToOptional
  EXPECT_TRUE(
      (std::is_assignable<std::optional<CopyConvert>,
                          const std::optional<NoConvertToOptional>&>::value));
  // operator=(const std::optional<U>&) with U = NoConvertToOptional
  // triggers SFINAE because
  // std::is_constructible_v<MoveConvert, const NoConvertToOptional&> is false
  EXPECT_FALSE(
      (std::is_assignable<std::optional<MoveConvert>&,
                          const std::optional<NoConvertToOptional>&>::value));
  // operator=(std::optional<U>&&) with U = NoConvertToOptional
  EXPECT_TRUE(
      (std::is_assignable<std::optional<MoveConvert>&,
                          std::optional<NoConvertToOptional>&&>::value));
  // operator=(const std::optional<U>&) with U = NoConvertToOptional triggers
  // SFINAE because std::is_constructible_v<MoveConvertFromOptional, const
  // NoConvertToOptional&> is false. operator=(U&&) with U = const
  // std::optional<NoConverToOptional>& triggers SFINAE because
  // std::is_constructible<MoveConvertFromOptional,
  // std::optional<NoConvertToOptional>&&> is true.
  EXPECT_FALSE(
      (std::is_assignable<std::optional<MoveConvertFromOptional>&,
                          const std::optional<NoConvertToOptional>&>::value));
}

TEST(optionalTest, ResetAndHasValue) {
  StructorListener listener;
  Listenable::listener = &listener;
  std::optional<Listenable> opt;
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

  constexpr std::optional<int> empty;
  static_assert(!empty.has_value(), "");
  constexpr std::optional<int> nonempty(1);
  static_assert(nonempty.has_value(), "");
}

TEST(optionalTest, Emplace) {
  StructorListener listener;
  Listenable::listener = &listener;
  std::optional<Listenable> opt;
  EXPECT_FALSE(opt);
  opt.emplace(1);
  EXPECT_TRUE(opt);
  opt.emplace(1, 2);
  EXPECT_EQ(1, listener.construct1);
  EXPECT_EQ(1, listener.construct2);
  EXPECT_EQ(1, listener.destruct);

  std::optional<std::string> o;
  EXPECT_TRUE((std::is_same<std::string&, decltype(o.emplace("abc"))>::value));
  std::string& ref = o.emplace("abc");
  EXPECT_EQ(&ref, &o.value());
}

TEST(optionalTest, ListEmplace) {
  StructorListener listener;
  Listenable::listener = &listener;
  std::optional<Listenable> opt;
  EXPECT_FALSE(opt);
  opt.emplace({1});
  EXPECT_TRUE(opt);
  opt.emplace({1, 2});
  EXPECT_EQ(2, listener.listinit);
  EXPECT_EQ(1, listener.destruct);

  std::optional<Listenable> o;
  EXPECT_TRUE((std::is_same<Listenable&, decltype(o.emplace({1}))>::value));
  Listenable& ref = o.emplace({1});
  EXPECT_EQ(&ref, &o.value());
}

TEST(optionalTest, Swap) {
  std::optional<int> opt_empty, opt1 = 1, opt2 = 2;
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
// to document the fact that the current implementation of std::optional<T>
// expects such usecases to be malformed and not compile.
TEST(optionalTest, OperatorAddr) {
  constexpr int v = -1;
  {  // constexpr
    constexpr std::optional<DeletedOpAddr<v>> opt(turbo::in_place_t{});
    static_assert(opt.has_value(), "");
    // static_assert(opt->value == v, "");
    static_assert((*opt).value == v, "");
  }
  {  // non-constexpr
    const std::optional<DeletedOpAddr<v>> opt(turbo::in_place_t{});
    EXPECT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->value == v);
    EXPECT_TRUE((*opt).value == v);
  }
}

TEST(optionalTest, PointerStuff) {
  std::optional<std::string> opt(turbo::in_place, "foo");
  EXPECT_EQ("foo", *opt);
  const auto& opt_const = opt;
  EXPECT_EQ("foo", *opt_const);
  EXPECT_EQ(opt->size(), 3u);
  EXPECT_EQ(opt_const->size(), 3u);

  constexpr std::optional<ConstexprType> opt1(1);
  static_assert((*opt1).x == ConstexprType::kCtorInt, "");
}

// gcc has a bug pre 4.9.1 where it doesn't do correct overload resolution
// when overloads are const-qualified and *this is an raluve.
// Skip that test to make the build green again when using the old compiler.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59296 is fixed in 4.9.1.
#if defined(__GNUC__) && !defined(__clang__)
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40901
#define TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
#endif
#endif

// MSVC has a bug with "cv-qualifiers in class construction", fixed in 2017. See
// https://docs.microsoft.com/en-us/cpp/cpp-conformance-improvements-2017#bug-fixes
// The compiler some incorrectly ingores the cv-qualifier when generating a
// class object via a constructor call. For example:
//
// class optional {
//   constexpr T&& value() &&;
//   constexpr const T&& value() const &&;
// }
//
// using COI = const std::optional<int>;
// static_assert(2 == COI(2).value(), "");  // const &&
//
// This should invoke the "const &&" overload but since it ignores the const
// qualifier it finds the "&&" overload the best candidate.
#if defined(_MSC_VER) && _MSC_VER < 1910
#define TURBO_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG
#endif

TEST(optionalTest, Value) {
  using O = std::optional<std::string>;
  using CO = const std::optional<std::string>;
  using OC = std::optional<const std::string>;
  O lvalue(turbo::in_place, "lvalue");
  CO clvalue(turbo::in_place, "clvalue");
  OC lvalue_c(turbo::in_place, "lvalue_c");
  EXPECT_EQ("lvalue", lvalue.value());
  EXPECT_EQ("clvalue", clvalue.value());
  EXPECT_EQ("lvalue_c", lvalue_c.value());
  EXPECT_EQ("xvalue", O(turbo::in_place, "xvalue").value());
  EXPECT_EQ("xvalue_c", OC(turbo::in_place, "xvalue_c").value());
#ifndef TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
  EXPECT_EQ("cxvalue", CO(turbo::in_place, "cxvalue").value());
#endif
  EXPECT_EQ("&", TypeQuals(lvalue.value()));
  EXPECT_EQ("c&", TypeQuals(clvalue.value()));
  EXPECT_EQ("c&", TypeQuals(lvalue_c.value()));
  EXPECT_EQ("&&", TypeQuals(O(turbo::in_place, "xvalue").value()));
#if !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
  EXPECT_EQ("c&&", TypeQuals(CO(turbo::in_place, "cxvalue").value()));
#endif
  EXPECT_EQ("c&&", TypeQuals(OC(turbo::in_place, "xvalue_c").value()));

#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // test on volatile type
  using OV = std::optional<volatile int>;
  OV lvalue_v(turbo::in_place, 42);
  EXPECT_EQ(42, lvalue_v.value());
  EXPECT_EQ(42, OV(42).value());
  EXPECT_TRUE((std::is_same<volatile int&, decltype(lvalue_v.value())>::value));
  EXPECT_TRUE((std::is_same<volatile int&&, decltype(OV(42).value())>::value));
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  // test exception throw on value()
  std::optional<int> empty;
#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW((void)empty.value(), turbo::bad_optional_access);
#else
  EXPECT_DEATH_IF_SUPPORTED((void)empty.value(), "Bad optional access");
#endif

  // test constexpr value()
  constexpr std::optional<int> o1(1);
  static_assert(1 == o1.value(), "");  // const &
#if !defined(_MSC_VER) && !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
  using COI = const std::optional<int>;
  static_assert(2 == COI(2).value(), "");  // const &&
#endif
}

TEST(optionalTest, DerefOperator) {
  using O = std::optional<std::string>;
  using CO = const std::optional<std::string>;
  using OC = std::optional<const std::string>;
  O lvalue(turbo::in_place, "lvalue");
  CO clvalue(turbo::in_place, "clvalue");
  OC lvalue_c(turbo::in_place, "lvalue_c");
  EXPECT_EQ("lvalue", *lvalue);
  EXPECT_EQ("clvalue", *clvalue);
  EXPECT_EQ("lvalue_c", *lvalue_c);
  EXPECT_EQ("xvalue", *O(turbo::in_place, "xvalue"));
  EXPECT_EQ("xvalue_c", *OC(turbo::in_place, "xvalue_c"));
#ifndef TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG
  EXPECT_EQ("cxvalue", *CO(turbo::in_place, "cxvalue"));
#endif
  EXPECT_EQ("&", TypeQuals(*lvalue));
  EXPECT_EQ("c&", TypeQuals(*clvalue));
  EXPECT_EQ("&&", TypeQuals(*O(turbo::in_place, "xvalue")));
#if !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG) && \
    !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
  EXPECT_EQ("c&&", TypeQuals(*CO(turbo::in_place, "cxvalue")));
#endif
  EXPECT_EQ("c&&", TypeQuals(*OC(turbo::in_place, "xvalue_c")));

#if !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)
  // test on volatile type
  using OV = std::optional<volatile int>;
  OV lvalue_v(turbo::in_place, 42);
  EXPECT_EQ(42, *lvalue_v);
  EXPECT_EQ(42, *OV(42));
  EXPECT_TRUE((std::is_same<volatile int&, decltype(*lvalue_v)>::value));
  EXPECT_TRUE((std::is_same<volatile int&&, decltype(*OV(42))>::value));
#endif  // !defined(TURBO_VOLATILE_RETURN_TYPES_DEPRECATED)

  constexpr std::optional<int> opt1(1);
  static_assert(*opt1 == 1, "");
#if !defined(_MSC_VER) && !defined(TURBO_SKIP_OVERLOAD_TEST_DUE_TO_GCC_BUG)
  using COI = const std::optional<int>;
  static_assert(*COI(2) == 2, "");
#endif
}

TEST(optionalTest, ValueOr) {
  std::optional<double> opt_empty, opt_set = 1.2;
  EXPECT_EQ(42.0, opt_empty.value_or(42));
  EXPECT_EQ(1.2, opt_set.value_or(42));
  EXPECT_EQ(42.0, std::optional<double>().value_or(42));
  EXPECT_EQ(1.2, std::optional<double>(1.2).value_or(42));

  constexpr std::optional<double> copt_empty, copt_set = {1.2};
  static_assert(42.0 == copt_empty.value_or(42), "");
  static_assert(1.2 == copt_set.value_or(42), "");
#ifndef TURBO_SKIP_OVERLOAD_TEST_DUE_TO_MSVC_BUG
  using COD = const std::optional<double>;
  static_assert(42.0 == COD().value_or(42), "");
  static_assert(1.2 == COD(1.2).value_or(42), "");
#endif
}

// make_optional cannot be constexpr until C++17
TEST(optionalTest, make_optional) {
  auto opt_int = turbo::make_optional(42);
  EXPECT_TRUE((std::is_same<decltype(opt_int), std::optional<int>>::value));
  EXPECT_EQ(42, opt_int);

  StructorListener listener;
  Listenable::listener = &listener;

  std::optional<Listenable> opt0 = turbo::make_optional<Listenable>();
  EXPECT_EQ(1, listener.construct0);
  std::optional<Listenable> opt1 = turbo::make_optional<Listenable>(1);
  EXPECT_EQ(1, listener.construct1);
  std::optional<Listenable> opt2 = turbo::make_optional<Listenable>(1, 2);
  EXPECT_EQ(1, listener.construct2);
  std::optional<Listenable> opt3 = turbo::make_optional<Listenable>({1});
  std::optional<Listenable> opt4 = turbo::make_optional<Listenable>({1, 2});
  EXPECT_EQ(2, listener.listinit);

  // Constexpr tests on trivially copyable types
  // optional<T> has trivial copy/move ctors when T is trivially copyable.
  // For nontrivial types with constexpr constructors, we need copy elision in
  // C++17 for make_optional to be constexpr.
  {
    constexpr std::optional<int> c_opt = turbo::make_optional(42);
    static_assert(c_opt.value() == 42, "");
  }
  {
    struct TrivialCopyable {
      constexpr TrivialCopyable() : x(0) {}
      constexpr explicit TrivialCopyable(int i) : x(i) {}
      int x;
    };

    constexpr TrivialCopyable v;
    constexpr std::optional<TrivialCopyable> c_opt0 = turbo::make_optional(v);
    static_assert((*c_opt0).x == 0, "");
    constexpr std::optional<TrivialCopyable> c_opt1 =
        turbo::make_optional<TrivialCopyable>();
    static_assert((*c_opt1).x == 0, "");
    constexpr std::optional<TrivialCopyable> c_opt2 =
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
  std::optional<T> ae, a2{2}, a4{4};
  std::optional<U> be, b2{2}, b4{4};
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

  // compare std::optional<std::string> with const char*
  std::optional<std::string> opt_str = "abc";
  const char* cstr = "abc";
  EXPECT_TRUE(opt_str == cstr);
  // compare std::optional<std::string> with std::optional<const char*>
  std::optional<const char*> opt_cstr = cstr;
  EXPECT_TRUE(opt_str == opt_cstr);
  // compare std::optional<std::string> with std::optional<std::string_view>
  std::optional<std::string_view> e1;
  std::optional<std::string> e2;
  EXPECT_TRUE(e1 == e2);
}


TEST(optionalTest, SwapRegression) {
  StructorListener listener;
  Listenable::listener = &listener;

  {
    std::optional<Listenable> a;
    std::optional<Listenable> b(turbo::in_place);
    a.swap(b);
  }

  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.move);
  EXPECT_EQ(2, listener.destruct);

  {
    std::optional<Listenable> a(turbo::in_place);
    std::optional<Listenable> b;
    a.swap(b);
  }

  EXPECT_EQ(2, listener.construct0);
  EXPECT_EQ(2, listener.move);
  EXPECT_EQ(4, listener.destruct);
}

TEST(optionalTest, BigStringLeakCheck) {
  constexpr size_t n = 1 << 16;

  using OS = std::optional<std::string>;

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
    std::optional<Listenable> a;
    Listenable b;
    a = std::move(b);
  }

  EXPECT_EQ(1, listener.construct0);
  EXPECT_EQ(1, listener.move);
  EXPECT_EQ(2, listener.destruct);
}

TEST(optionalTest, ValueType) {
  EXPECT_TRUE((std::is_same<std::optional<int>::value_type, int>::value));
  EXPECT_TRUE((std::is_same<std::optional<std::string>::value_type,
                            std::string>::value));
  EXPECT_FALSE(
      (std::is_same<std::optional<int>::value_type, turbo::nullopt_t>::value));
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
  std::hash<std::optional<int>> hash;
  std::set<size_t> hashcodes;
  hashcodes.insert(hash(turbo::nullopt));
  for (int i = 0; i < 100; ++i) {
    hashcodes.insert(hash(i));
  }
  EXPECT_GT(hashcodes.size(), 90u);

  static_assert(is_hash_enabled_for<std::optional<int>>::value, "");
  static_assert(is_hash_enabled_for<std::optional<Hashable>>::value, "");
  static_assert(
      turbo::type_traits_internal::IsHashable<std::optional<int>>::value, "");
  static_assert(
      turbo::type_traits_internal::IsHashable<std::optional<Hashable>>::value,
      "");
  turbo::type_traits_internal::AssertHashEnabled<std::optional<int>>();
  turbo::type_traits_internal::AssertHashEnabled<std::optional<Hashable>>();

#if TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
  static_assert(!is_hash_enabled_for<std::optional<NonHashable>>::value, "");
  static_assert(!turbo::type_traits_internal::IsHashable<
                    std::optional<NonHashable>>::value,
                "");
#endif

  // libstdc++ std::optional is missing remove_const_t, i.e. it's using
  // std::hash<T> rather than std::hash<std::remove_const_t<T>>.
  // Reference: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82262
#ifndef __GLIBCXX__
  static_assert(is_hash_enabled_for<std::optional<const int>>::value, "");
  static_assert(is_hash_enabled_for<std::optional<const Hashable>>::value, "");
  std::hash<std::optional<const int>> c_hash;
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(hash(i), c_hash(i));
  }
#endif
}

struct MoveMeNoThrow {
  MoveMeNoThrow() : x(0) {}
  [[noreturn]] MoveMeNoThrow(const MoveMeNoThrow& other) : x(other.x) {
    TURBO_RAW_LOG(FATAL, "Should not be called.");
    abort();
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
      std::is_nothrow_move_constructible<std::optional<MoveMeNoThrow>>::value,
      "");
  static_assert(turbo::default_allocator_is_nothrow::value ==
                    std::is_nothrow_move_constructible<
                        std::optional<MoveMeThrow>>::value,
                "");
  std::vector<std::optional<MoveMeNoThrow>> v;
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
  EXPECT_TRUE((std::is_constructible<AnyLike, std::optional<AnyLike>>::value));

  EXPECT_TRUE(
      (std::is_constructible<AnyLike, const std::optional<AnyLike>&>::value));

  EXPECT_TRUE((std::is_constructible<std::optional<AnyLike>, AnyLike>::value));
  EXPECT_TRUE(
      (std::is_constructible<std::optional<AnyLike>, const AnyLike&>::value));

  EXPECT_TRUE((std::is_convertible<std::optional<AnyLike>, AnyLike>::value));

  EXPECT_TRUE(
      (std::is_convertible<const std::optional<AnyLike>&, AnyLike>::value));

  EXPECT_TRUE((std::is_convertible<AnyLike, std::optional<AnyLike>>::value));
  EXPECT_TRUE(
      (std::is_convertible<const AnyLike&, std::optional<AnyLike>>::value));

  EXPECT_TRUE(std::is_move_constructible<std::optional<AnyLike>>::value);
  EXPECT_TRUE(std::is_copy_constructible<std::optional<AnyLike>>::value);
}

TEST(optionalTest, AssignmentConstraints) {
  EXPECT_TRUE((std::is_assignable<AnyLike&, std::optional<AnyLike>>::value));
  EXPECT_TRUE(
      (std::is_assignable<AnyLike&, const std::optional<AnyLike>&>::value));
  EXPECT_TRUE((std::is_assignable<std::optional<AnyLike>&, AnyLike>::value));
  EXPECT_TRUE(
      (std::is_assignable<std::optional<AnyLike>&, const AnyLike&>::value));
  EXPECT_TRUE(std::is_move_assignable<std::optional<AnyLike>>::value);
  EXPECT_TRUE(turbo::is_copy_assignable<std::optional<AnyLike>>::value);
}

#if !defined(__EMSCRIPTEN__)
struct NestedClassBug {
  struct Inner {
    bool dummy = false;
  };
  std::optional<Inner> value;
};

TEST(optionalTest, InPlaceTSFINAEBug) {
  NestedClassBug b;
  ((void)b);
  using Inner = NestedClassBug::Inner;

  EXPECT_TRUE((std::is_default_constructible<Inner>::value));
  EXPECT_TRUE((std::is_constructible<Inner>::value));
  EXPECT_TRUE(
      (std::is_constructible<std::optional<Inner>, turbo::in_place_t>::value));

  std::optional<Inner> o(turbo::in_place);
  EXPECT_TRUE(o.has_value());
  o.emplace();
  EXPECT_TRUE(o.has_value());
}
#endif  // !defined(__EMSCRIPTEN__)

}  // namespace

#endif  // #if !defined(TURBO_USES_STD_OPTIONAL)
