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

#include "turbo/meta/type_traits.h"

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "turbo/platform/port.h"
#include "turbo/time/clock.h"
#include "turbo/time/time.h"
#include "gtest/gtest.h"

namespace {

using ::testing::StaticAssertTypeEq;

template <class T, class U>
struct simple_pair {
  T first;
  U second;
};

struct Dummy {};

struct ReturnType {};
struct ConvertibleToReturnType {
  operator ReturnType() const;  // NOLINT
};

// Unique types used as parameter types for testing the detection idiom.
struct StructA {};
struct StructB {};
struct StructC {};

struct TypeWithBarFunction {
  template <class T,
            turbo::enable_if_t<std::is_same<T&&, StructA&>::value, int> = 0>
  ReturnType bar(T&&, const StructB&, StructC&&) &&;  // NOLINT
};

struct TypeWithBarFunctionAndConvertibleReturnType {
  template <class T,
            turbo::enable_if_t<std::is_same<T&&, StructA&>::value, int> = 0>
  ConvertibleToReturnType bar(T&&, const StructB&, StructC&&) &&;  // NOLINT
};

template <class Class, class... Ts>
using BarIsCallableImpl =
    decltype(std::declval<Class>().bar(std::declval<Ts>()...));

template <class Class, class... T>
using BarIsCallable =
    turbo::type_traits_internal::is_detected<BarIsCallableImpl, Class, T...>;

template <class Class, class... T>
using BarIsCallableConv = turbo::type_traits_internal::is_detected_convertible<
    ReturnType, BarIsCallableImpl, Class, T...>;

// NOTE: Test of detail type_traits_internal::is_detected.
TEST(IsDetectedTest, BasicUsage) {
  EXPECT_TRUE((BarIsCallable<TypeWithBarFunction, StructA&, const StructB&,
                             StructC>::value));
  EXPECT_TRUE(
      (BarIsCallable<TypeWithBarFunction, StructA&, StructB&, StructC>::value));
  EXPECT_TRUE(
      (BarIsCallable<TypeWithBarFunction, StructA&, StructB, StructC>::value));

  EXPECT_FALSE((BarIsCallable<int, StructA&, const StructB&, StructC>::value));
  EXPECT_FALSE((BarIsCallable<TypeWithBarFunction&, StructA&, const StructB&,
                              StructC>::value));
  EXPECT_FALSE((BarIsCallable<TypeWithBarFunction, StructA, const StructB&,
                              StructC>::value));
}

// NOTE: Test of detail type_traits_internal::is_detected_convertible.
TEST(IsDetectedConvertibleTest, BasicUsage) {
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA&, const StructB&,
                                 StructC>::value));
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA&, StructB&,
                                 StructC>::value));
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunction, StructA&, StructB,
                                 StructC>::value));
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                                 StructA&, const StructB&, StructC>::value));
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                                 StructA&, StructB&, StructC>::value));
  EXPECT_TRUE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                                 StructA&, StructB, StructC>::value));

  EXPECT_FALSE(
      (BarIsCallableConv<int, StructA&, const StructB&, StructC>::value));
  EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunction&, StructA&,
                                  const StructB&, StructC>::value));
  EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunction, StructA, const StructB&,
                                  StructC>::value));
  EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType&,
                                  StructA&, const StructB&, StructC>::value));
  EXPECT_FALSE((BarIsCallableConv<TypeWithBarFunctionAndConvertibleReturnType,
                                  StructA, const StructB&, StructC>::value));
}

TEST(VoidTTest, BasicUsage) {
  StaticAssertTypeEq<void, turbo::void_t<Dummy>>();
  StaticAssertTypeEq<void, turbo::void_t<Dummy, Dummy, Dummy>>();
}

TEST(ConjunctionTest, BasicBooleanLogic) {
  EXPECT_TRUE(turbo::conjunction<>::value);
  EXPECT_TRUE(turbo::conjunction<std::true_type>::value);
  EXPECT_TRUE((turbo::conjunction<std::true_type, std::true_type>::value));
  EXPECT_FALSE((turbo::conjunction<std::true_type, std::false_type>::value));
  EXPECT_FALSE((turbo::conjunction<std::false_type, std::true_type>::value));
  EXPECT_FALSE((turbo::conjunction<std::false_type, std::false_type>::value));
}

struct MyTrueType {
  static constexpr bool value = true;
};

struct MyFalseType {
  static constexpr bool value = false;
};

TEST(ConjunctionTest, ShortCircuiting) {
  EXPECT_FALSE(
      (turbo::conjunction<std::true_type, std::false_type, Dummy>::value));
  EXPECT_TRUE((std::is_base_of<MyFalseType,
                               turbo::conjunction<std::true_type, MyFalseType,
                                                 std::false_type>>::value));
  EXPECT_TRUE(
      (std::is_base_of<MyTrueType,
                       turbo::conjunction<std::true_type, MyTrueType>>::value));
}

TEST(DisjunctionTest, BasicBooleanLogic) {
  EXPECT_FALSE(turbo::disjunction<>::value);
  EXPECT_FALSE(turbo::disjunction<std::false_type>::value);
  EXPECT_TRUE((turbo::disjunction<std::true_type, std::true_type>::value));
  EXPECT_TRUE((turbo::disjunction<std::true_type, std::false_type>::value));
  EXPECT_TRUE((turbo::disjunction<std::false_type, std::true_type>::value));
  EXPECT_FALSE((turbo::disjunction<std::false_type, std::false_type>::value));
}

TEST(DisjunctionTest, ShortCircuiting) {
  EXPECT_TRUE(
      (turbo::disjunction<std::false_type, std::true_type, Dummy>::value));
  EXPECT_TRUE((
      std::is_base_of<MyTrueType, turbo::disjunction<std::false_type, MyTrueType,
                                                    std::true_type>>::value));
  EXPECT_TRUE((
      std::is_base_of<MyFalseType,
                      turbo::disjunction<std::false_type, MyFalseType>>::value));
}

TEST(NegationTest, BasicBooleanLogic) {
  EXPECT_FALSE(turbo::negation<std::true_type>::value);
  EXPECT_FALSE(turbo::negation<MyTrueType>::value);
  EXPECT_TRUE(turbo::negation<std::false_type>::value);
  EXPECT_TRUE(turbo::negation<MyFalseType>::value);
}

// all member functions are trivial
class Trivial {
  int n_;
};

struct TrivialDestructor {
  ~TrivialDestructor() = default;
};

struct NontrivialDestructor {
  ~NontrivialDestructor() {}
};

struct DeletedDestructor {
  ~DeletedDestructor() = delete;
};

class TrivialDefaultCtor {
 public:
  TrivialDefaultCtor() = default;
  explicit TrivialDefaultCtor(int n) : n_(n) {}

 private:
  int n_;
};

class NontrivialDefaultCtor {
 public:
  NontrivialDefaultCtor() : n_(1) {}

 private:
  int n_;
};

class DeletedDefaultCtor {
 public:
  DeletedDefaultCtor() = delete;
  explicit DeletedDefaultCtor(int n) : n_(n) {}

 private:
  int n_;
};

class TrivialMoveCtor {
 public:
  explicit TrivialMoveCtor(int n) : n_(n) {}
  TrivialMoveCtor(TrivialMoveCtor&&) = default;
  TrivialMoveCtor& operator=(const TrivialMoveCtor& t) {
    n_ = t.n_;
    return *this;
  }

 private:
  int n_;
};

class NontrivialMoveCtor {
 public:
  explicit NontrivialMoveCtor(int n) : n_(n) {}
  NontrivialMoveCtor(NontrivialMoveCtor&& t) noexcept : n_(t.n_) {}
  NontrivialMoveCtor& operator=(const NontrivialMoveCtor&) = default;

 private:
  int n_;
};

class TrivialCopyCtor {
 public:
  explicit TrivialCopyCtor(int n) : n_(n) {}
  TrivialCopyCtor(const TrivialCopyCtor&) = default;
  TrivialCopyCtor& operator=(const TrivialCopyCtor& t) {
    n_ = t.n_;
    return *this;
  }

 private:
  int n_;
};

class NontrivialCopyCtor {
 public:
  explicit NontrivialCopyCtor(int n) : n_(n) {}
  NontrivialCopyCtor(const NontrivialCopyCtor& t) : n_(t.n_) {}
  NontrivialCopyCtor& operator=(const NontrivialCopyCtor&) = default;

 private:
  int n_;
};

class DeletedCopyCtor {
 public:
  explicit DeletedCopyCtor(int n) : n_(n) {}
  DeletedCopyCtor(const DeletedCopyCtor&) = delete;
  DeletedCopyCtor& operator=(const DeletedCopyCtor&) = default;

 private:
  int n_;
};

class TrivialMoveAssign {
 public:
  explicit TrivialMoveAssign(int n) : n_(n) {}
  TrivialMoveAssign(const TrivialMoveAssign& t) : n_(t.n_) {}
  TrivialMoveAssign& operator=(TrivialMoveAssign&&) = default;
  ~TrivialMoveAssign() {}  // can have nontrivial destructor
 private:
  int n_;
};

class NontrivialMoveAssign {
 public:
  explicit NontrivialMoveAssign(int n) : n_(n) {}
  NontrivialMoveAssign(const NontrivialMoveAssign&) = default;
  NontrivialMoveAssign& operator=(NontrivialMoveAssign&& t) noexcept {
    n_ = t.n_;
    return *this;
  }

 private:
  int n_;
};

class TrivialCopyAssign {
 public:
  explicit TrivialCopyAssign(int n) : n_(n) {}
  TrivialCopyAssign(const TrivialCopyAssign& t) : n_(t.n_) {}
  TrivialCopyAssign& operator=(const TrivialCopyAssign& t) = default;
  ~TrivialCopyAssign() {}  // can have nontrivial destructor
 private:
  int n_;
};

class NontrivialCopyAssign {
 public:
  explicit NontrivialCopyAssign(int n) : n_(n) {}
  NontrivialCopyAssign(const NontrivialCopyAssign&) = default;
  NontrivialCopyAssign& operator=(const NontrivialCopyAssign& t) {
    n_ = t.n_;
    return *this;
  }

 private:
  int n_;
};

class DeletedCopyAssign {
 public:
  explicit DeletedCopyAssign(int n) : n_(n) {}
  DeletedCopyAssign(const DeletedCopyAssign&) = default;
  DeletedCopyAssign& operator=(const DeletedCopyAssign&) = delete;

 private:
  int n_;
};

struct MovableNonCopyable {
  MovableNonCopyable() = default;
  MovableNonCopyable(const MovableNonCopyable&) = delete;
  MovableNonCopyable(MovableNonCopyable&&) = default;
  MovableNonCopyable& operator=(const MovableNonCopyable&) = delete;
  MovableNonCopyable& operator=(MovableNonCopyable&&) = default;
};

struct NonCopyableOrMovable {
  NonCopyableOrMovable() = default;
  virtual ~NonCopyableOrMovable() = default;
  NonCopyableOrMovable(const NonCopyableOrMovable&) = delete;
  NonCopyableOrMovable(NonCopyableOrMovable&&) = delete;
  NonCopyableOrMovable& operator=(const NonCopyableOrMovable&) = delete;
  NonCopyableOrMovable& operator=(NonCopyableOrMovable&&) = delete;
};

class Base {
 public:
  virtual ~Base() {}
};

// Old versions of libc++, around Clang 3.5 to 3.6, consider deleted destructors
// as also being trivial. With the resolution of CWG 1928 and CWG 1734, this
// is no longer considered true and has thus been amended.
// Compiler Explorer: https://godbolt.org/g/zT59ZL
// CWG issue 1734: http://open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#1734
// CWG issue 1928: http://open-std.org/JTC1/SC22/WG21/docs/cwg_closed.html#1928
#if !defined(_LIBCPP_VERSION) || _LIBCPP_VERSION >= 3700
#define TURBO_TRIVIALLY_DESTRUCTIBLE_CONSIDER_DELETED_DESTRUCTOR_NOT_TRIVIAL 1
#endif

// As of the moment, GCC versions >5.1 have a problem compiling for
// std::is_trivially_default_constructible<NontrivialDestructor[10]>, where
// NontrivialDestructor is a struct with a custom nontrivial destructor. Note
// that this problem only occurs for arrays of a known size, so something like
// std::is_trivially_default_constructible<NontrivialDestructor[]> does not
// have any problems.
// Compiler Explorer: https://godbolt.org/g/dXRbdK
// GCC bug 83689: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83689
#if defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ < 5)
#define TURBO_GCC_BUG_TRIVIALLY_CONSTRUCTIBLE_ON_ARRAY_OF_NONTRIVIAL 1
#endif

TEST(TypeTraitsTest, TestIsFunction) {
  struct Callable {
    void operator()() {}
  };
  EXPECT_TRUE(turbo::is_function<void()>::value);
  EXPECT_TRUE(turbo::is_function<void()&>::value);
  EXPECT_TRUE(turbo::is_function<void() const>::value);
  EXPECT_TRUE(turbo::is_function<void() noexcept>::value);
  EXPECT_TRUE(turbo::is_function<void(...) noexcept>::value);

  EXPECT_FALSE(turbo::is_function<void(*)()>::value);
  EXPECT_FALSE(turbo::is_function<void(&)()>::value);
  EXPECT_FALSE(turbo::is_function<int>::value);
  EXPECT_FALSE(turbo::is_function<Callable>::value);
}

TEST(TypeTraitsTest, TestTrivialDestructor) {
  // Verify that arithmetic types and pointers have trivial destructors.
  EXPECT_TRUE(turbo::is_trivially_destructible<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<char>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<int>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<float>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<double>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<Trivial**>::value);

  // classes with destructors
  EXPECT_TRUE(turbo::is_trivially_destructible<Trivial>::value);
  EXPECT_TRUE(turbo::is_trivially_destructible<TrivialDestructor>::value);

  // Verify that types with a nontrivial or deleted destructor
  // are marked as such.
  EXPECT_FALSE(turbo::is_trivially_destructible<NontrivialDestructor>::value);
#ifdef TURBO_TRIVIALLY_DESTRUCTIBLE_CONSIDER_DELETED_DESTRUCTOR_NOT_TRIVIAL
  EXPECT_FALSE(turbo::is_trivially_destructible<DeletedDestructor>::value);
#endif

  // simple_pair of such types is trivial
  EXPECT_TRUE((turbo::is_trivially_destructible<simple_pair<int, int>>::value));
  EXPECT_TRUE((turbo::is_trivially_destructible<
               simple_pair<Trivial, TrivialDestructor>>::value));

  // Verify that types without trivial destructors are correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_destructible<std::string>::value);
  EXPECT_FALSE(turbo::is_trivially_destructible<std::vector<int>>::value);

  // Verify that simple_pairs of types without trivial destructors
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_destructible<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_destructible<
                simple_pair<std::string, int>>::value));

  // array of such types is trivial
  using int10 = int[10];
  EXPECT_TRUE(turbo::is_trivially_destructible<int10>::value);
  using Trivial10 = Trivial[10];
  EXPECT_TRUE(turbo::is_trivially_destructible<Trivial10>::value);
  using TrivialDestructor10 = TrivialDestructor[10];
  EXPECT_TRUE(turbo::is_trivially_destructible<TrivialDestructor10>::value);

  // Conversely, the opposite also holds.
  using NontrivialDestructor10 = NontrivialDestructor[10];
  EXPECT_FALSE(turbo::is_trivially_destructible<NontrivialDestructor10>::value);
}

TEST(TypeTraitsTest, TestTrivialDefaultCtor) {
  // arithmetic types and pointers have trivial default constructors.
  EXPECT_TRUE(turbo::is_trivially_default_constructible<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<char>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<int>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<float>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<double>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<Trivial*>::value);
  EXPECT_TRUE(
      turbo::is_trivially_default_constructible<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_default_constructible<Trivial**>::value);

  // types with compiler generated default ctors
  EXPECT_TRUE(turbo::is_trivially_default_constructible<Trivial>::value);
  EXPECT_TRUE(
      turbo::is_trivially_default_constructible<TrivialDefaultCtor>::value);

  // Verify that types without them are not.
  EXPECT_FALSE(
      turbo::is_trivially_default_constructible<NontrivialDefaultCtor>::value);
  EXPECT_FALSE(
      turbo::is_trivially_default_constructible<DeletedDefaultCtor>::value);

  // types with nontrivial destructor are nontrivial
  EXPECT_FALSE(
      turbo::is_trivially_default_constructible<NontrivialDestructor>::value);

  // types with vtables
  EXPECT_FALSE(turbo::is_trivially_default_constructible<Base>::value);

  // Verify that simple_pair has trivial constructors where applicable.
  EXPECT_TRUE((turbo::is_trivially_default_constructible<
               simple_pair<int, char*>>::value));
  EXPECT_TRUE((turbo::is_trivially_default_constructible<
               simple_pair<int, Trivial>>::value));
  EXPECT_TRUE((turbo::is_trivially_default_constructible<
               simple_pair<int, TrivialDefaultCtor>>::value));

  // Verify that types without trivial constructors are
  // correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_default_constructible<std::string>::value);
  EXPECT_FALSE(
      turbo::is_trivially_default_constructible<std::vector<int>>::value);

  // Verify that simple_pairs of types without trivial constructors
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_default_constructible<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_default_constructible<
                simple_pair<std::string, int>>::value));

  // Verify that arrays of such types are trivially default constructible
  using int10 = int[10];
  EXPECT_TRUE(turbo::is_trivially_default_constructible<int10>::value);
  using Trivial10 = Trivial[10];
  EXPECT_TRUE(turbo::is_trivially_default_constructible<Trivial10>::value);
  using TrivialDefaultCtor10 = TrivialDefaultCtor[10];
  EXPECT_TRUE(
      turbo::is_trivially_default_constructible<TrivialDefaultCtor10>::value);

  // Conversely, the opposite also holds.
#ifdef TURBO_GCC_BUG_TRIVIALLY_CONSTRUCTIBLE_ON_ARRAY_OF_NONTRIVIAL
  using NontrivialDefaultCtor10 = NontrivialDefaultCtor[10];
  EXPECT_FALSE(
      turbo::is_trivially_default_constructible<NontrivialDefaultCtor10>::value);
#endif
}

// GCC prior to 7.4 had a bug in its trivially-constructible traits
// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80654).
// This test makes sure that we do not depend on the trait in these cases when
// implementing turbo triviality traits.

template <class T>
struct BadConstructors {
  BadConstructors() { static_assert(T::value, ""); }

  BadConstructors(BadConstructors&&) { static_assert(T::value, ""); }

  BadConstructors(const BadConstructors&) { static_assert(T::value, ""); }
};

TEST(TypeTraitsTest, TestTrivialityBadConstructors) {
  using BadType = BadConstructors<int>;

  EXPECT_FALSE(turbo::is_trivially_default_constructible<BadType>::value);
  EXPECT_FALSE(turbo::is_trivially_move_constructible<BadType>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<BadType>::value);
}

TEST(TypeTraitsTest, TestTrivialMoveCtor) {
  // Verify that arithmetic types and pointers have trivial move
  // constructors.
  EXPECT_TRUE(turbo::is_trivially_move_constructible<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<int>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<float>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<double>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<Trivial**>::value);

  // Reference types
  EXPECT_TRUE(turbo::is_trivially_move_constructible<int&>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<int&&>::value);

  // types with compiler generated move ctors
  EXPECT_TRUE(turbo::is_trivially_move_constructible<Trivial>::value);
  EXPECT_TRUE(turbo::is_trivially_move_constructible<TrivialMoveCtor>::value);

  // Verify that types without them (i.e. nontrivial or deleted) are not.
  EXPECT_FALSE(
      turbo::is_trivially_move_constructible<NontrivialCopyCtor>::value);
  EXPECT_FALSE(turbo::is_trivially_move_constructible<DeletedCopyCtor>::value);
  EXPECT_FALSE(
      turbo::is_trivially_move_constructible<NonCopyableOrMovable>::value);

  // type with nontrivial destructor are nontrivial move construbtible
  EXPECT_FALSE(
      turbo::is_trivially_move_constructible<NontrivialDestructor>::value);

  // types with vtables
  EXPECT_FALSE(turbo::is_trivially_move_constructible<Base>::value);

  // Verify that simple_pair of such types is trivially move constructible
  EXPECT_TRUE(
      (turbo::is_trivially_move_constructible<simple_pair<int, char*>>::value));
  EXPECT_TRUE((
      turbo::is_trivially_move_constructible<simple_pair<int, Trivial>>::value));
  EXPECT_TRUE((turbo::is_trivially_move_constructible<
               simple_pair<int, TrivialMoveCtor>>::value));

  // Verify that types without trivial move constructors are
  // correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_move_constructible<std::string>::value);
  EXPECT_FALSE(turbo::is_trivially_move_constructible<std::vector<int>>::value);

  // Verify that simple_pairs of types without trivial move constructors
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_move_constructible<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_move_constructible<
                simple_pair<std::string, int>>::value));

  // Verify that arrays are not
  using int10 = int[10];
  EXPECT_FALSE(turbo::is_trivially_move_constructible<int10>::value);
}

TEST(TypeTraitsTest, TestTrivialCopyCtor) {
  // Verify that arithmetic types and pointers have trivial copy
  // constructors.
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<int>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<float>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<double>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<Trivial**>::value);

  // Reference types
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<int&>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<int&&>::value);

  // types with compiler generated copy ctors
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<Trivial>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_constructible<TrivialCopyCtor>::value);

  // Verify that types without them (i.e. nontrivial or deleted) are not.
  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<NontrivialCopyCtor>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<DeletedCopyCtor>::value);
  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<MovableNonCopyable>::value);
  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<NonCopyableOrMovable>::value);

  // type with nontrivial destructor are nontrivial copy construbtible
  EXPECT_FALSE(
      turbo::is_trivially_copy_constructible<NontrivialDestructor>::value);

  // types with vtables
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<Base>::value);

  // Verify that simple_pair of such types is trivially copy constructible
  EXPECT_TRUE(
      (turbo::is_trivially_copy_constructible<simple_pair<int, char*>>::value));
  EXPECT_TRUE((
      turbo::is_trivially_copy_constructible<simple_pair<int, Trivial>>::value));
  EXPECT_TRUE((turbo::is_trivially_copy_constructible<
               simple_pair<int, TrivialCopyCtor>>::value));

  // Verify that types without trivial copy constructors are
  // correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<std::string>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<std::vector<int>>::value);

  // Verify that simple_pairs of types without trivial copy constructors
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_copy_constructible<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_copy_constructible<
                simple_pair<std::string, int>>::value));

  // Verify that arrays are not
  using int10 = int[10];
  EXPECT_FALSE(turbo::is_trivially_copy_constructible<int10>::value);
}

TEST(TypeTraitsTest, TestTrivialMoveAssign) {
  // Verify that arithmetic types and pointers have trivial move
  // assignment operators.
  EXPECT_TRUE(turbo::is_trivially_move_assignable<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<int>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<float>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<double>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<Trivial**>::value);

  // const qualified types are not assignable
  EXPECT_FALSE(turbo::is_trivially_move_assignable<const int>::value);

  // types with compiler generated move assignment
  EXPECT_TRUE(turbo::is_trivially_move_assignable<Trivial>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<TrivialMoveAssign>::value);

  // Verify that types without them (i.e. nontrivial or deleted) are not.
  EXPECT_FALSE(turbo::is_trivially_move_assignable<NontrivialCopyAssign>::value);
  EXPECT_FALSE(turbo::is_trivially_move_assignable<DeletedCopyAssign>::value);
  EXPECT_FALSE(turbo::is_trivially_move_assignable<NonCopyableOrMovable>::value);

  // types with vtables
  EXPECT_FALSE(turbo::is_trivially_move_assignable<Base>::value);

  // Verify that simple_pair is trivially assignable
  EXPECT_TRUE(
      (turbo::is_trivially_move_assignable<simple_pair<int, char*>>::value));
  EXPECT_TRUE(
      (turbo::is_trivially_move_assignable<simple_pair<int, Trivial>>::value));
  EXPECT_TRUE((turbo::is_trivially_move_assignable<
               simple_pair<int, TrivialMoveAssign>>::value));

  // Verify that types not trivially move assignable are
  // correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_move_assignable<std::string>::value);
  EXPECT_FALSE(turbo::is_trivially_move_assignable<std::vector<int>>::value);

  // Verify that simple_pairs of types not trivially move assignable
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_move_assignable<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_move_assignable<
                simple_pair<std::string, int>>::value));

  // Verify that arrays are not trivially move assignable
  using int10 = int[10];
  EXPECT_FALSE(turbo::is_trivially_move_assignable<int10>::value);

  // Verify that references are handled correctly
  EXPECT_TRUE(turbo::is_trivially_move_assignable<Trivial&&>::value);
  EXPECT_TRUE(turbo::is_trivially_move_assignable<Trivial&>::value);
}

TEST(TypeTraitsTest, TestTrivialCopyAssign) {
  // Verify that arithmetic types and pointers have trivial copy
  // assignment operators.
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<bool>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<unsigned char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<signed char>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<wchar_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<int>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<unsigned int>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<int16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<uint16_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<int64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<uint64_t>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<float>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<double>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<long double>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<const std::string*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<const Trivial*>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<std::string**>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial**>::value);

  // const qualified types are not assignable
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<const int>::value);

  // types with compiler generated copy assignment
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<TrivialCopyAssign>::value);

  // Verify that types without them (i.e. nontrivial or deleted) are not.
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<NontrivialCopyAssign>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<DeletedCopyAssign>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<MovableNonCopyable>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<NonCopyableOrMovable>::value);

  // types with vtables
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<Base>::value);

  // Verify that simple_pair is trivially assignable
  EXPECT_TRUE(
      (turbo::is_trivially_copy_assignable<simple_pair<int, char*>>::value));
  EXPECT_TRUE(
      (turbo::is_trivially_copy_assignable<simple_pair<int, Trivial>>::value));
  EXPECT_TRUE((turbo::is_trivially_copy_assignable<
               simple_pair<int, TrivialCopyAssign>>::value));

  // Verify that types not trivially copy assignable are
  // correctly marked as such.
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<std::string>::value);
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<std::vector<int>>::value);

  // Verify that simple_pairs of types not trivially copy assignable
  // are not marked as trivial.
  EXPECT_FALSE((turbo::is_trivially_copy_assignable<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::is_trivially_copy_assignable<
                simple_pair<std::string, int>>::value));

  // Verify that arrays are not trivially copy assignable
  using int10 = int[10];
  EXPECT_FALSE(turbo::is_trivially_copy_assignable<int10>::value);

  // Verify that references are handled correctly
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial&&>::value);
  EXPECT_TRUE(turbo::is_trivially_copy_assignable<Trivial&>::value);
}

TEST(TypeTraitsTest, TestTriviallyCopyable) {
  // Verify that arithmetic types and pointers are trivially copyable.
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<bool>::value);
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<char>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<unsigned char>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<signed char>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<wchar_t>::value);
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<int>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<unsigned int>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<int16_t>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<uint16_t>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<int64_t>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<uint64_t>::value);
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<float>::value);
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<double>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<long double>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<std::string*>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<Trivial*>::value);
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<
              const std::string*>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<const Trivial*>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<std::string**>::value);
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<Trivial**>::value);

  // const qualified types are not assignable but are constructible
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<const int>::value);

  // Trivial copy constructor/assignment and destructor.
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<Trivial>::value);
  // Trivial copy assignment, but non-trivial copy constructor/destructor.
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               TrivialCopyAssign>::value);
  // Trivial copy constructor, but non-trivial assignment.
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               TrivialCopyCtor>::value);

  // Types with a non-trivial copy constructor/assignment
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               NontrivialCopyCtor>::value);
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               NontrivialCopyAssign>::value);

  // Types without copy constructor/assignment, but with move
  // MSVC disagrees with other compilers about this:
  // EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<
  //             MovableNonCopyable>::value);

  // Types without copy/move constructor/assignment
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               NonCopyableOrMovable>::value);

  // No copy assign, but has trivial copy constructor.
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<
              DeletedCopyAssign>::value);

  // types with vtables
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<Base>::value);

  // Verify that simple_pair is trivially copyable if members are
  EXPECT_TRUE((turbo::type_traits_internal::is_trivially_copyable<
               simple_pair<int, char*>>::value));
  EXPECT_TRUE((turbo::type_traits_internal::is_trivially_copyable<
               simple_pair<int, Trivial>>::value));

  // Verify that types not trivially copyable are
  // correctly marked as such.
  EXPECT_FALSE(
      turbo::type_traits_internal::is_trivially_copyable<std::string>::value);
  EXPECT_FALSE(turbo::type_traits_internal::is_trivially_copyable<
               std::vector<int>>::value);

  // Verify that simple_pairs of types not trivially copyable
  // are not marked as trivial.
  EXPECT_FALSE((turbo::type_traits_internal::is_trivially_copyable<
                simple_pair<int, std::string>>::value));
  EXPECT_FALSE((turbo::type_traits_internal::is_trivially_copyable<
                simple_pair<std::string, int>>::value));
  EXPECT_FALSE((turbo::type_traits_internal::is_trivially_copyable<
                simple_pair<int, TrivialCopyAssign>>::value));

  // Verify that arrays of trivially copyable types are trivially copyable
  using int10 = int[10];
  EXPECT_TRUE(turbo::type_traits_internal::is_trivially_copyable<int10>::value);
  using int10x10 = int[10][10];
  EXPECT_TRUE(
      turbo::type_traits_internal::is_trivially_copyable<int10x10>::value);

  // Verify that references are handled correctly
  EXPECT_FALSE(
      turbo::type_traits_internal::is_trivially_copyable<Trivial&&>::value);
  EXPECT_FALSE(
      turbo::type_traits_internal::is_trivially_copyable<Trivial&>::value);
}

TEST(TypeTraitsTest, TestRemoveCVRef) {
  EXPECT_TRUE(
      (std::is_same<typename turbo::remove_cvref<int>::type, int>::value));
  EXPECT_TRUE(
      (std::is_same<typename turbo::remove_cvref<int&>::type, int>::value));
  EXPECT_TRUE(
      (std::is_same<typename turbo::remove_cvref<int&&>::type, int>::value));
  EXPECT_TRUE((
      std::is_same<typename turbo::remove_cvref<const int&>::type, int>::value));
  EXPECT_TRUE(
      (std::is_same<typename turbo::remove_cvref<int*>::type, int*>::value));
  // Does not remove const in this case.
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int*>::type,
                            const int*>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int[2]>::type,
                            int[2]>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int(&)[2]>::type,
                            int[2]>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<int(&&)[2]>::type,
                            int[2]>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int[2]>::type,
                            int[2]>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int(&)[2]>::type,
                            int[2]>::value));
  EXPECT_TRUE((std::is_same<typename turbo::remove_cvref<const int(&&)[2]>::type,
                            int[2]>::value));
}

#define TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(trait_name, ...)          \
  EXPECT_TRUE((std::is_same<typename std::trait_name<__VA_ARGS__>::type, \
                            turbo::trait_name##_t<__VA_ARGS__>>::value))

TEST(TypeTraitsTest, TestRemoveCVAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_cv, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_cv, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_cv, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_cv, const volatile int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_const, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_const, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_const, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_const, const volatile int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_volatile, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_volatile, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_volatile, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_volatile, const volatile int);
}

TEST(TypeTraitsTest, TestAddCVAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_cv, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_cv, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_cv, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_cv, const volatile int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_const, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_const, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_const, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_const, const volatile int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_volatile, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_volatile, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_volatile, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_volatile, const volatile int);
}

TEST(TypeTraitsTest, TestReferenceAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, volatile int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, int&&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_reference, volatile int&&);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, volatile int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, int&&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_lvalue_reference, volatile int&&);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, volatile int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, int&&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_rvalue_reference, volatile int&&);
}

TEST(TypeTraitsTest, TestPointerAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_pointer, int*);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_pointer, volatile int*);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_pointer, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(add_pointer, volatile int);
}

TEST(TypeTraitsTest, TestSignednessAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_signed, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_signed, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_signed, unsigned);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_signed, volatile unsigned);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_unsigned, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_unsigned, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_unsigned, unsigned);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(make_unsigned, volatile unsigned);
}

TEST(TypeTraitsTest, TestExtentAliases) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_extent, int[]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_extent, int[1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_extent, int[1][1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_extent, int[][1]);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_all_extents, int[]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_all_extents, int[1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_all_extents, int[1][1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(remove_all_extents, int[][1]);
}

TEST(TypeTraitsTest, TestAlignedStorageAlias) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 1);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 2);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 3);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 4);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 5);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 6);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 7);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 8);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 9);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 10);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 11);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 12);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 13);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 14);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 15);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 16);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 17);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 18);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 19);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 20);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 21);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 22);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 23);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 24);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 25);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 26);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 27);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 28);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 29);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 30);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 31);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 32);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 33);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 1, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 2, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 3, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 4, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 5, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 6, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 7, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 8, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 9, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 10, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 11, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 12, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 13, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 14, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 15, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 16, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 17, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 18, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 19, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 20, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 21, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 22, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 23, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 24, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 25, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 26, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 27, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 28, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 29, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 30, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 31, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 32, 128);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(aligned_storage, 33, 128);
}

TEST(TypeTraitsTest, TestDecay) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, volatile int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const volatile int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, volatile int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const volatile int&);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, volatile int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, const volatile int&);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int[1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int[1][1]);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int[][1]);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int());
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int(float));  // NOLINT
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int(char, ...));  // NOLINT
}

struct TypeA {};
struct TypeB {};
struct TypeC {};
struct TypeD {};

template <typename T>
struct Wrap {};

enum class TypeEnum { A, B, C, D };

struct GetTypeT {
  template <typename T,
            turbo::enable_if_t<std::is_same<T, TypeA>::value, int> = 0>
  TypeEnum operator()(Wrap<T>) const {
    return TypeEnum::A;
  }

  template <typename T,
            turbo::enable_if_t<std::is_same<T, TypeB>::value, int> = 0>
  TypeEnum operator()(Wrap<T>) const {
    return TypeEnum::B;
  }

  template <typename T,
            turbo::enable_if_t<std::is_same<T, TypeC>::value, int> = 0>
  TypeEnum operator()(Wrap<T>) const {
    return TypeEnum::C;
  }

  // NOTE: TypeD is intentionally not handled
} constexpr GetType = {};

TEST(TypeTraitsTest, TestEnableIf) {
  EXPECT_EQ(TypeEnum::A, GetType(Wrap<TypeA>()));
  EXPECT_EQ(TypeEnum::B, GetType(Wrap<TypeB>()));
  EXPECT_EQ(TypeEnum::C, GetType(Wrap<TypeC>()));
}

TEST(TypeTraitsTest, TestConditional) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(conditional, true, int, char);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(conditional, false, int, char);
}

// TODO(calabrese) Check with specialized std::common_type
TEST(TypeTraitsTest, TestCommonType) {
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int, char);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int, char, int);

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int, char&);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(common_type, int, char, int&);
}

TEST(TypeTraitsTest, TestUnderlyingType) {
  enum class enum_char : char {};
  enum class enum_long_long : long long {};  // NOLINT(runtime/int)

  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(underlying_type, enum_char);
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(underlying_type, enum_long_long);
}

struct GetTypeExtT {
  template <typename T>
  turbo::result_of_t<const GetTypeT&(T)> operator()(T&& arg) const {
    return GetType(std::forward<T>(arg));
  }

  TypeEnum operator()(Wrap<TypeD>) const { return TypeEnum::D; }
} constexpr GetTypeExt = {};

TEST(TypeTraitsTest, TestResultOf) {
  EXPECT_EQ(TypeEnum::A, GetTypeExt(Wrap<TypeA>()));
  EXPECT_EQ(TypeEnum::B, GetTypeExt(Wrap<TypeB>()));
  EXPECT_EQ(TypeEnum::C, GetTypeExt(Wrap<TypeC>()));
  EXPECT_EQ(TypeEnum::D, GetTypeExt(Wrap<TypeD>()));
}

template <typename T>
bool TestCopyAssign() {
  return turbo::is_copy_assignable<T>::value ==
         std::is_copy_assignable<T>::value;
}

TEST(TypeTraitsTest, IsCopyAssignable) {
  EXPECT_TRUE(TestCopyAssign<int>());
  EXPECT_TRUE(TestCopyAssign<int&>());
  EXPECT_TRUE(TestCopyAssign<int&&>());

  struct S {};
  EXPECT_TRUE(TestCopyAssign<S>());
  EXPECT_TRUE(TestCopyAssign<S&>());
  EXPECT_TRUE(TestCopyAssign<S&&>());

  class C {
   public:
    explicit C(C* c) : c_(c) {}
    ~C() { delete c_; }

   private:
    C* c_;
  };
  EXPECT_TRUE(TestCopyAssign<C>());
  EXPECT_TRUE(TestCopyAssign<C&>());
  EXPECT_TRUE(TestCopyAssign<C&&>());

  // Reason for ifndef: add_lvalue_reference<T> in libc++ breaks for these cases
#ifndef _LIBCPP_VERSION
  EXPECT_TRUE(TestCopyAssign<int()>());
  EXPECT_TRUE(TestCopyAssign<int(int) const>());
  EXPECT_TRUE(TestCopyAssign<int(...) volatile&>());
  EXPECT_TRUE(TestCopyAssign<int(int, ...) const volatile&&>());
#endif  // _LIBCPP_VERSION
}

template <typename T>
bool TestMoveAssign() {
  return turbo::is_move_assignable<T>::value ==
         std::is_move_assignable<T>::value;
}

TEST(TypeTraitsTest, IsMoveAssignable) {
  EXPECT_TRUE(TestMoveAssign<int>());
  EXPECT_TRUE(TestMoveAssign<int&>());
  EXPECT_TRUE(TestMoveAssign<int&&>());

  struct S {};
  EXPECT_TRUE(TestMoveAssign<S>());
  EXPECT_TRUE(TestMoveAssign<S&>());
  EXPECT_TRUE(TestMoveAssign<S&&>());

  class C {
   public:
    explicit C(C* c) : c_(c) {}
    ~C() { delete c_; }
    void operator=(const C&) = delete;
    void operator=(C&&) = delete;

   private:
    C* c_;
  };
  EXPECT_TRUE(TestMoveAssign<C>());
  EXPECT_TRUE(TestMoveAssign<C&>());
  EXPECT_TRUE(TestMoveAssign<C&&>());

  // Reason for ifndef: add_lvalue_reference<T> in libc++ breaks for these cases
#ifndef _LIBCPP_VERSION
  EXPECT_TRUE(TestMoveAssign<int()>());
  EXPECT_TRUE(TestMoveAssign<int(int) const>());
  EXPECT_TRUE(TestMoveAssign<int(...) volatile&>());
  EXPECT_TRUE(TestMoveAssign<int(int, ...) const volatile&&>());
#endif  // _LIBCPP_VERSION
}

namespace adl_namespace {

struct DeletedSwap {
};

void swap(DeletedSwap&, DeletedSwap&) = delete;

struct SpecialNoexceptSwap {
  SpecialNoexceptSwap(SpecialNoexceptSwap&&) {}
  SpecialNoexceptSwap& operator=(SpecialNoexceptSwap&&) { return *this; }
  ~SpecialNoexceptSwap() = default;
};

void swap(SpecialNoexceptSwap&, SpecialNoexceptSwap&) noexcept {}

}  // namespace adl_namespace

TEST(TypeTraitsTest, IsSwappable) {
  using turbo::type_traits_internal::IsSwappable;
  using turbo::type_traits_internal::StdSwapIsUnconstrained;

  EXPECT_TRUE(IsSwappable<int>::value);

  struct S {};
  EXPECT_TRUE(IsSwappable<S>::value);

  struct NoConstruct {
    NoConstruct(NoConstruct&&) = delete;
    NoConstruct& operator=(NoConstruct&&) { return *this; }
    ~NoConstruct() = default;
  };

  EXPECT_EQ(IsSwappable<NoConstruct>::value, StdSwapIsUnconstrained::value);
  struct NoAssign {
    NoAssign(NoAssign&&) {}
    NoAssign& operator=(NoAssign&&) = delete;
    ~NoAssign() = default;
  };

  EXPECT_EQ(IsSwappable<NoAssign>::value, StdSwapIsUnconstrained::value);

  EXPECT_FALSE(IsSwappable<adl_namespace::DeletedSwap>::value);

  EXPECT_TRUE(IsSwappable<adl_namespace::SpecialNoexceptSwap>::value);
}

TEST(TypeTraitsTest, IsNothrowSwappable) {
  using turbo::type_traits_internal::IsNothrowSwappable;
  using turbo::type_traits_internal::StdSwapIsUnconstrained;

  EXPECT_TRUE(IsNothrowSwappable<int>::value);

  struct NonNoexceptMoves {
    NonNoexceptMoves(NonNoexceptMoves&&) {}
    NonNoexceptMoves& operator=(NonNoexceptMoves&&) { return *this; }
    ~NonNoexceptMoves() = default;
  };

  EXPECT_FALSE(IsNothrowSwappable<NonNoexceptMoves>::value);

  struct NoConstruct {
    NoConstruct(NoConstruct&&) = delete;
    NoConstruct& operator=(NoConstruct&&) { return *this; }
    ~NoConstruct() = default;
  };

  EXPECT_FALSE(IsNothrowSwappable<NoConstruct>::value);

  struct NoAssign {
    NoAssign(NoAssign&&) {}
    NoAssign& operator=(NoAssign&&) = delete;
    ~NoAssign() = default;
  };

  EXPECT_FALSE(IsNothrowSwappable<NoAssign>::value);

  EXPECT_FALSE(IsNothrowSwappable<adl_namespace::DeletedSwap>::value);

  EXPECT_TRUE(IsNothrowSwappable<adl_namespace::SpecialNoexceptSwap>::value);
}

TEST(TrivallyRelocatable, Sanity) {
#if !defined(TURBO_HAVE_ATTRIBUTE_TRIVIAL_ABI) || \
    !TURBO_HAVE_BUILTIN(__is_trivially_relocatable)
  GTEST_SKIP() << "No trivial ABI support.";
#endif

  struct Trivial {};
  struct NonTrivial {
    NonTrivial(const NonTrivial&) {}  // NOLINT
  };
  struct TURBO_ATTRIBUTE_TRIVIAL_ABI TrivialAbi {
    TrivialAbi(const TrivialAbi&) {}  // NOLINT
  };
  EXPECT_TRUE(turbo::is_trivially_relocatable<Trivial>::value);
  EXPECT_FALSE(turbo::is_trivially_relocatable<NonTrivial>::value);
  EXPECT_TRUE(turbo::is_trivially_relocatable<TrivialAbi>::value);
}

#ifdef TURBO_HAVE_CONSTANT_EVALUATED

constexpr int64_t NegateIfConstantEvaluated(int64_t i) {
  if (turbo::is_constant_evaluated()) {
    return -i;
  } else {
    return i;
  }
}

#endif  // TURBO_HAVE_CONSTANT_EVALUATED

TEST(TrivallyRelocatable, is_constant_evaluated) {
#ifdef TURBO_HAVE_CONSTANT_EVALUATED
  constexpr int64_t constant = NegateIfConstantEvaluated(42);
  EXPECT_EQ(constant, -42);

  int64_t now = turbo::ToUnixSeconds(turbo::Now());
  int64_t not_constant = NegateIfConstantEvaluated(now);
  EXPECT_EQ(not_constant, now);

  static int64_t const_init = NegateIfConstantEvaluated(42);
  EXPECT_EQ(const_init, -42);
#else
  GTEST_SKIP() << "turbo::is_constant_evaluated is not defined";
#endif  // TURBO_HAVE_CONSTANT_EVALUATED
}


}  // namespace
