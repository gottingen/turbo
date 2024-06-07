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

#include <turbo/meta/type_traits.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/times/clock.h>
#include <turbo/times/time.h>
#include <string_view>

namespace {

using ::testing::StaticAssertTypeEq;

template <typename T>
using IsOwnerAndNotView =
    turbo::conjunction<turbo::type_traits_internal::IsOwner<T>,
                      turbo::negation<turbo::type_traits_internal::IsView<T>>>;

static_assert(IsOwnerAndNotView<std::vector<int>>::value,
              "vector is an owner, not a view");
static_assert(IsOwnerAndNotView<std::string>::value,
              "string is an owner, not a view");
static_assert(IsOwnerAndNotView<std::wstring>::value,
              "wstring is an owner, not a view");
static_assert(!IsOwnerAndNotView<std::string_view>::value,
              "string_view is a view, not an owner");
static_assert(!IsOwnerAndNotView<std::wstring_view>::value,
              "wstring_view is a view, not an owner");

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

TEST(TypeTraitsTest, TestIsFunction) {
  struct Callable {
    void operator()() {}
  };
  EXPECT_TRUE(turbo::is_function<void()>::value);
  EXPECT_TRUE(turbo::is_function<void()&>::value);
  EXPECT_TRUE(turbo::is_function<void() const>::value);
  EXPECT_TRUE(turbo::is_function<void() noexcept>::value);
  EXPECT_TRUE(turbo::is_function<void(...) noexcept>::value);

  EXPECT_FALSE(turbo::is_function<void (*)()>::value);
  EXPECT_FALSE(turbo::is_function<void (&)()>::value);
  EXPECT_FALSE(turbo::is_function<int>::value);
  EXPECT_FALSE(turbo::is_function<Callable>::value);
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
  EXPECT_TRUE(
      (std::is_same<typename turbo::remove_cvref<int[2]>::type, int[2]>::value));
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
  TURBO_INTERNAL_EXPECT_ALIAS_EQUIVALENCE(decay, int(float));      // NOLINT
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

namespace adl_namespace {

struct DeletedSwap {};

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

TEST(TriviallyRelocatable, PrimitiveTypes) {
  static_assert(turbo::is_trivially_relocatable<int>::value, "");
  static_assert(turbo::is_trivially_relocatable<char>::value, "");
  static_assert(turbo::is_trivially_relocatable<void*>::value, "");
}

// User-defined types can be trivially relocatable as long as they don't have a
// user-provided move constructor or destructor.
TEST(TriviallyRelocatable, UserDefinedTriviallyRelocatable) {
  struct S {
    int x;
    int y;
  };

  static_assert(turbo::is_trivially_relocatable<S>::value, "");
}

// A user-provided move constructor disqualifies a type from being trivially
// relocatable.
TEST(TriviallyRelocatable, UserProvidedMoveConstructor) {
  struct S {
    S(S&&) {}  // NOLINT(modernize-use-equals-default)
  };

  static_assert(!turbo::is_trivially_relocatable<S>::value, "");
}

// A user-provided copy constructor disqualifies a type from being trivially
// relocatable.
TEST(TriviallyRelocatable, UserProvidedCopyConstructor) {
  struct S {
    S(const S&) {}  // NOLINT(modernize-use-equals-default)
  };

  static_assert(!turbo::is_trivially_relocatable<S>::value, "");
}

// A user-provided copy assignment operator disqualifies a type from
// being trivially relocatable.
TEST(TriviallyRelocatable, UserProvidedCopyAssignment) {
  struct S {
    S(const S&) = default;
    S& operator=(const S&) {  // NOLINT(modernize-use-equals-default)
      return *this;
    }
  };

  static_assert(!turbo::is_trivially_relocatable<S>::value, "");
}

// A user-provided move assignment operator disqualifies a type from
// being trivially relocatable.
TEST(TriviallyRelocatable, UserProvidedMoveAssignment) {
  struct S {
    S(S&&) = default;
    S& operator=(S&&) { return *this; }  // NOLINT(modernize-use-equals-default)
  };

  static_assert(!turbo::is_trivially_relocatable<S>::value, "");
}

// A user-provided destructor disqualifies a type from being trivially
// relocatable.
TEST(TriviallyRelocatable, UserProvidedDestructor) {
  struct S {
    ~S() {}  // NOLINT(modernize-use-equals-default)
  };

  static_assert(!turbo::is_trivially_relocatable<S>::value, "");
}

// TODO(b/275003464): remove the opt-out for Clang on Windows once
// __is_trivially_relocatable is used there again.
// TODO(b/324278148): remove the opt-out for Apple once
// __is_trivially_relocatable is fixed there.
#if defined(TURBO_HAVE_ATTRIBUTE_TRIVIAL_ABI) &&      \
    TURBO_HAVE_BUILTIN(__is_trivially_relocatable) && \
    (defined(__cpp_impl_trivially_relocatable) ||    \
     (!defined(__clang__) && !defined(__APPLE__) && !defined(__NVCC__)))
// A type marked with the "trivial ABI" attribute is trivially relocatable even
// if it has user-provided special members.
TEST(TriviallyRelocatable, TrivialAbi) {
  struct TURBO_ATTRIBUTE_TRIVIAL_ABI S {
    S(S&&) {}       // NOLINT(modernize-use-equals-default)
    S(const S&) {}  // NOLINT(modernize-use-equals-default)
    void operator=(S&&) {}
    void operator=(const S&) {}
    ~S() {}  // NOLINT(modernize-use-equals-default)
  };

  static_assert(turbo::is_trivially_relocatable<S>::value, "");
}
#endif

#ifdef TURBO_HAVE_CONSTANT_EVALUATED

constexpr int64_t NegateIfConstantEvaluated(int64_t i) {
  if (turbo::is_constant_evaluated()) {
    return -i;
  } else {
    return i;
  }
}

#endif  // TURBO_HAVE_CONSTANT_EVALUATED

TEST(IsConstantEvaluated, is_constant_evaluated) {
#ifdef TURBO_HAVE_CONSTANT_EVALUATED
  constexpr int64_t constant = NegateIfConstantEvaluated(42);
  EXPECT_EQ(constant, -42);

  int64_t now = turbo::Time::to_seconds(turbo::Time::current_time());
  int64_t not_constant = NegateIfConstantEvaluated(now);
  EXPECT_EQ(not_constant, now);

  static int64_t const_init = NegateIfConstantEvaluated(42);
  EXPECT_EQ(const_init, -42);
#else
  GTEST_SKIP() << "turbo::is_constant_evaluated is not defined";
#endif  // TURBO_HAVE_CONSTANT_EVALUATED
}

}  // namespace
