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

#include <turbo/types/span.h>

#include <array>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <tests/base/exception_testing.h>
#include <turbo/base/options.h>
#include <turbo/container/fixed_array.h>
#include <turbo/container/inlined_vector.h>
#include <tests/hash/hash_testing.h>
#include <turbo/meta/type_traits.h>
#include <turbo/strings/str_cat.h>

namespace {

static_assert(!turbo::type_traits_internal::IsOwner<turbo::Span<int>>::value &&
                  turbo::type_traits_internal::IsView<turbo::Span<int>>::value,
              "Span is a view, not an owner");

MATCHER_P(DataIs, data,
          turbo::StrCat("data() ", negation ? "isn't " : "is ",
                       testing::PrintToString(data))) {
  return arg.data() == data;
}

template <typename T>
auto SpanIs(T data, size_t size)
    -> decltype(testing::AllOf(DataIs(data), testing::SizeIs(size))) {
  return testing::AllOf(DataIs(data), testing::SizeIs(size));
}

template <typename Container>
auto SpanIs(const Container& c) -> decltype(SpanIs(c.data(), c.size())) {
  return SpanIs(c.data(), c.size());
}

std::vector<int> MakeRamp(int len, int offset = 0) {
  std::vector<int> v(len);
  std::iota(v.begin(), v.end(), offset);
  return v;
}

TEST(IntSpan, EmptyCtors) {
  turbo::Span<int> s;
  EXPECT_THAT(s, SpanIs(nullptr, 0));
}

TEST(IntSpan, PtrLenCtor) {
  int a[] = {1, 2, 3};
  turbo::Span<int> s(&a[0], 2);
  EXPECT_THAT(s, SpanIs(a, 2));
}

TEST(IntSpan, ArrayCtor) {
  int a[] = {1, 2, 3};
  turbo::Span<int> s(a);
  EXPECT_THAT(s, SpanIs(a, 3));

  EXPECT_TRUE((std::is_constructible<turbo::Span<const int>, int[3]>::value));
  EXPECT_TRUE(
      (std::is_constructible<turbo::Span<const int>, const int[3]>::value));
  EXPECT_FALSE((std::is_constructible<turbo::Span<int>, const int[3]>::value));
  EXPECT_TRUE((std::is_convertible<int[3], turbo::Span<const int>>::value));
  EXPECT_TRUE(
      (std::is_convertible<const int[3], turbo::Span<const int>>::value));
}

template <typename T>
void TakesGenericSpan(turbo::Span<T>) {}

TEST(IntSpan, ContainerCtor) {
  std::vector<int> empty;
  turbo::Span<int> s_empty(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> filled{1, 2, 3};
  turbo::Span<int> s_filled(filled);
  EXPECT_THAT(s_filled, SpanIs(filled));

  turbo::Span<int> s_from_span(filled);
  EXPECT_THAT(s_from_span, SpanIs(s_filled));

  turbo::Span<const int> const_filled = filled;
  EXPECT_THAT(const_filled, SpanIs(filled));

  turbo::Span<const int> const_from_span = s_filled;
  EXPECT_THAT(const_from_span, SpanIs(s_filled));

  EXPECT_TRUE(
      (std::is_convertible<std::vector<int>&, turbo::Span<const int>>::value));
  EXPECT_TRUE(
      (std::is_convertible<turbo::Span<int>&, turbo::Span<const int>>::value));

  TakesGenericSpan(turbo::Span<int>(filled));
}

// A struct supplying shallow data() const.
struct ContainerWithShallowConstData {
  std::vector<int> storage;
  int* data() const { return const_cast<int*>(storage.data()); }
  int size() const { return storage.size(); }
};

TEST(IntSpan, ShallowConstness) {
  const ContainerWithShallowConstData c{MakeRamp(20)};
  turbo::Span<int> s(
      c);  // We should be able to do this even though data() is const.
  s[0] = -1;
  EXPECT_EQ(c.storage[0], -1);
}

TEST(CharSpan, StringCtor) {
  std::string empty = "";
  turbo::Span<char> s_empty(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::string abc = "abc";
  turbo::Span<char> s_abc(abc);
  EXPECT_THAT(s_abc, SpanIs(abc));

  turbo::Span<const char> s_const_abc = abc;
  EXPECT_THAT(s_const_abc, SpanIs(abc));

  EXPECT_FALSE((std::is_constructible<turbo::Span<int>, std::string>::value));
  EXPECT_FALSE(
      (std::is_constructible<turbo::Span<const int>, std::string>::value));
  EXPECT_TRUE(
      (std::is_convertible<std::string, turbo::Span<const char>>::value));
}

TEST(IntSpan, FromConstPointer) {
  EXPECT_TRUE((std::is_constructible<turbo::Span<const int* const>,
                                     std::vector<int*>>::value));
  EXPECT_TRUE((std::is_constructible<turbo::Span<const int* const>,
                                     std::vector<const int*>>::value));
  EXPECT_FALSE((
      std::is_constructible<turbo::Span<const int*>, std::vector<int*>>::value));
  EXPECT_FALSE((
      std::is_constructible<turbo::Span<int*>, std::vector<const int*>>::value));
}

struct TypeWithMisleadingData {
  int& data() { return i; }
  int size() { return 1; }
  int i;
};

struct TypeWithMisleadingSize {
  int* data() { return &i; }
  const char* size() { return "1"; }
  int i;
};

TEST(IntSpan, EvilTypes) {
  EXPECT_FALSE(
      (std::is_constructible<turbo::Span<int>, TypeWithMisleadingData&>::value));
  EXPECT_FALSE(
      (std::is_constructible<turbo::Span<int>, TypeWithMisleadingSize&>::value));
}

struct Base {
  int* data() { return &i; }
  int size() { return 1; }
  int i;
};
struct Derived : Base {};

TEST(IntSpan, SpanOfDerived) {
  EXPECT_TRUE((std::is_constructible<turbo::Span<int>, Base&>::value));
  EXPECT_TRUE((std::is_constructible<turbo::Span<int>, Derived&>::value));
  EXPECT_FALSE(
      (std::is_constructible<turbo::Span<Base>, std::vector<Derived>>::value));
}

void TestInitializerList(turbo::Span<const int> s, const std::vector<int>& v) {
  EXPECT_TRUE(std::equal(s.begin(), s.end(), v.begin(), v.end()));
}

TEST(ConstIntSpan, InitializerListConversion) {
  TestInitializerList({}, {});
  TestInitializerList({1}, {1});
  TestInitializerList({1, 2, 3}, {1, 2, 3});

  EXPECT_FALSE((std::is_constructible<turbo::Span<int>,
                                      std::initializer_list<int>>::value));
  EXPECT_FALSE((
      std::is_convertible<turbo::Span<int>, std::initializer_list<int>>::value));
}

TEST(IntSpan, Data) {
  int i;
  turbo::Span<int> s(&i, 1);
  EXPECT_EQ(&i, s.data());
}

TEST(IntSpan, SizeLengthEmpty) {
  turbo::Span<int> empty;
  EXPECT_EQ(empty.size(), 0);
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.size(), empty.length());

  auto v = MakeRamp(10);
  turbo::Span<int> s(v);
  EXPECT_EQ(s.size(), 10);
  EXPECT_FALSE(s.empty());
  EXPECT_EQ(s.size(), s.length());
}

TEST(IntSpan, ElementAccess) {
  auto v = MakeRamp(10);
  turbo::Span<int> s(v);
  for (int i = 0; i < s.size(); ++i) {
    EXPECT_EQ(s[i], s.at(i));
  }

  EXPECT_EQ(s.front(), s[0]);
  EXPECT_EQ(s.back(), s[9]);

#if !defined(NDEBUG) || TURBO_OPTION_HARDENED
  EXPECT_DEATH_IF_SUPPORTED(s[-1], "");
  EXPECT_DEATH_IF_SUPPORTED(s[10], "");
#endif
}

TEST(IntSpan, AtThrows) {
  auto v = MakeRamp(10);
  turbo::Span<int> s(v);

  EXPECT_EQ(s.at(9), 9);
  TURBO_BASE_INTERNAL_EXPECT_FAIL(s.at(10), std::out_of_range,
                                 "failed bounds check");
}

TEST(IntSpan, RemovePrefixAndSuffix) {
  auto v = MakeRamp(20, 1);
  turbo::Span<int> s(v);
  EXPECT_EQ(s.size(), 20);

  s.remove_suffix(0);
  s.remove_prefix(0);
  EXPECT_EQ(s.size(), 20);

  s.remove_prefix(1);
  EXPECT_EQ(s.size(), 19);
  EXPECT_EQ(s[0], 2);

  s.remove_suffix(1);
  EXPECT_EQ(s.size(), 18);
  EXPECT_EQ(s.back(), 19);

  s.remove_prefix(7);
  EXPECT_EQ(s.size(), 11);
  EXPECT_EQ(s[0], 9);

  s.remove_suffix(11);
  EXPECT_EQ(s.size(), 0);

  EXPECT_EQ(v, MakeRamp(20, 1));

#if !defined(NDEBUG) || TURBO_OPTION_HARDENED
  turbo::Span<int> prefix_death(v);
  EXPECT_DEATH_IF_SUPPORTED(prefix_death.remove_prefix(21), "");
  turbo::Span<int> suffix_death(v);
  EXPECT_DEATH_IF_SUPPORTED(suffix_death.remove_suffix(21), "");
#endif
}

TEST(IntSpan, Subspan) {
  std::vector<int> empty;
  EXPECT_EQ(turbo::MakeSpan(empty).subspan(), empty);
  EXPECT_THAT(turbo::MakeSpan(empty).subspan(0, 0), SpanIs(empty));
  EXPECT_THAT(turbo::MakeSpan(empty).subspan(0, turbo::Span<const int>::npos),
              SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(), SpanIs(ramp));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(0, 10), SpanIs(ramp));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(0, turbo::Span<const int>::npos),
              SpanIs(ramp));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(0, 3), SpanIs(ramp.data(), 3));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(5, turbo::Span<const int>::npos),
              SpanIs(ramp.data() + 5, 5));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(3, 3), SpanIs(ramp.data() + 3, 3));
  EXPECT_THAT(turbo::MakeSpan(ramp).subspan(10, 5), SpanIs(ramp.data() + 10, 0));

#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW(turbo::MakeSpan(ramp).subspan(11, 5), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(turbo::MakeSpan(ramp).subspan(11, 5), "");
#endif
}

TEST(IntSpan, First) {
  std::vector<int> empty;
  EXPECT_THAT(turbo::MakeSpan(empty).first(0), SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(turbo::MakeSpan(ramp).first(0), SpanIs(ramp.data(), 0));
  EXPECT_THAT(turbo::MakeSpan(ramp).first(10), SpanIs(ramp));
  EXPECT_THAT(turbo::MakeSpan(ramp).first(3), SpanIs(ramp.data(), 3));

#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW(turbo::MakeSpan(ramp).first(11), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(turbo::MakeSpan(ramp).first(11), "");
#endif
}

TEST(IntSpan, Last) {
  std::vector<int> empty;
  EXPECT_THAT(turbo::MakeSpan(empty).last(0), SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(turbo::MakeSpan(ramp).last(0), SpanIs(ramp.data() + 10, 0));
  EXPECT_THAT(turbo::MakeSpan(ramp).last(10), SpanIs(ramp));
  EXPECT_THAT(turbo::MakeSpan(ramp).last(3), SpanIs(ramp.data() + 7, 3));

#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW(turbo::MakeSpan(ramp).last(11), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(turbo::MakeSpan(ramp).last(11), "");
#endif
}

TEST(IntSpan, MakeSpanPtrLength) {
  std::vector<int> empty;
  auto s_empty = turbo::MakeSpan(empty.data(), empty.size());
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::array<int, 3> a{{1, 2, 3}};
  auto s = turbo::MakeSpan(a.data(), a.size());
  EXPECT_THAT(s, SpanIs(a));

  EXPECT_THAT(turbo::MakeConstSpan(empty.data(), empty.size()), SpanIs(s_empty));
  EXPECT_THAT(turbo::MakeConstSpan(a.data(), a.size()), SpanIs(s));
}

TEST(IntSpan, MakeSpanTwoPtrs) {
  std::vector<int> empty;
  auto s_empty = turbo::MakeSpan(empty.data(), empty.data());
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> v{1, 2, 3};
  auto s = turbo::MakeSpan(v.data(), v.data() + 1);
  EXPECT_THAT(s, SpanIs(v.data(), 1));

  EXPECT_THAT(turbo::MakeConstSpan(empty.data(), empty.data()), SpanIs(s_empty));
  EXPECT_THAT(turbo::MakeConstSpan(v.data(), v.data() + 1), SpanIs(s));
}

TEST(IntSpan, MakeSpanContainer) {
  std::vector<int> empty;
  auto s_empty = turbo::MakeSpan(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> v{1, 2, 3};
  auto s = turbo::MakeSpan(v);
  EXPECT_THAT(s, SpanIs(v));

  EXPECT_THAT(turbo::MakeConstSpan(empty), SpanIs(s_empty));
  EXPECT_THAT(turbo::MakeConstSpan(v), SpanIs(s));

  EXPECT_THAT(turbo::MakeSpan(s), SpanIs(s));
  EXPECT_THAT(turbo::MakeConstSpan(s), SpanIs(s));
}

TEST(CharSpan, MakeSpanString) {
  std::string empty = "";
  auto s_empty = turbo::MakeSpan(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::string str = "abc";
  auto s_str = turbo::MakeSpan(str);
  EXPECT_THAT(s_str, SpanIs(str));

  EXPECT_THAT(turbo::MakeConstSpan(empty), SpanIs(s_empty));
  EXPECT_THAT(turbo::MakeConstSpan(str), SpanIs(s_str));
}

TEST(IntSpan, MakeSpanArray) {
  int a[] = {1, 2, 3};
  auto s = turbo::MakeSpan(a);
  EXPECT_THAT(s, SpanIs(a, 3));

  const int ca[] = {1, 2, 3};
  auto s_ca = turbo::MakeSpan(ca);
  EXPECT_THAT(s_ca, SpanIs(ca, 3));

  EXPECT_THAT(turbo::MakeConstSpan(a), SpanIs(s));
  EXPECT_THAT(turbo::MakeConstSpan(ca), SpanIs(s_ca));
}

// Compile-asserts that the argument has the expected decayed type.
template <typename Expected, typename T>
void CheckType(const T& /* value */) {
  testing::StaticAssertTypeEq<Expected, T>();
}

TEST(IntSpan, MakeSpanTypes) {
  std::vector<int> vec;
  const std::vector<int> cvec;
  int a[1];
  const int ca[] = {1};
  int* ip = a;
  const int* cip = ca;
  std::string s = "";
  const std::string cs = "";
  CheckType<turbo::Span<int>>(turbo::MakeSpan(vec));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(cvec));
  CheckType<turbo::Span<int>>(turbo::MakeSpan(ip, ip + 1));
  CheckType<turbo::Span<int>>(turbo::MakeSpan(ip, 1));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(cip, cip + 1));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(cip, 1));
  CheckType<turbo::Span<int>>(turbo::MakeSpan(a));
  CheckType<turbo::Span<int>>(turbo::MakeSpan(a, a + 1));
  CheckType<turbo::Span<int>>(turbo::MakeSpan(a, 1));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(ca));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(ca, ca + 1));
  CheckType<turbo::Span<const int>>(turbo::MakeSpan(ca, 1));
  CheckType<turbo::Span<char>>(turbo::MakeSpan(s));
  CheckType<turbo::Span<const char>>(turbo::MakeSpan(cs));
}

TEST(ConstIntSpan, MakeConstSpanTypes) {
  std::vector<int> vec;
  const std::vector<int> cvec;
  int array[1];
  const int carray[] = {0};
  int* ptr = array;
  const int* cptr = carray;
  std::string s = "";
  std::string cs = "";
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(vec));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(cvec));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(ptr, ptr + 1));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(ptr, 1));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(cptr, cptr + 1));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(cptr, 1));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(array));
  CheckType<turbo::Span<const int>>(turbo::MakeConstSpan(carray));
  CheckType<turbo::Span<const char>>(turbo::MakeConstSpan(s));
  CheckType<turbo::Span<const char>>(turbo::MakeConstSpan(cs));
}

TEST(IntSpan, Equality) {
  const int arr1[] = {1, 2, 3, 4, 5};
  int arr2[] = {1, 2, 3, 4, 5};
  std::vector<int> vec1(std::begin(arr1), std::end(arr1));
  std::vector<int> vec2 = vec1;
  std::vector<int> other_vec = {2, 4, 6, 8, 10};
  // These two slices are from different vectors, but have the same size and
  // have the same elements (right now).  They should compare equal. Test both
  // == and !=.
  const turbo::Span<const int> from1 = vec1;
  const turbo::Span<const int> from2 = vec2;
  EXPECT_EQ(from1, from1);
  EXPECT_FALSE(from1 != from1);
  EXPECT_EQ(from1, from2);
  EXPECT_FALSE(from1 != from2);

  // These two slices have different underlying vector values. They should be
  // considered not equal. Test both == and !=.
  const turbo::Span<const int> from_other = other_vec;
  EXPECT_NE(from1, from_other);
  EXPECT_FALSE(from1 == from_other);

  // Comparison between a vector and its slice should be equal. And vice-versa.
  // This ensures implicit conversion to Span works on both sides of ==.
  EXPECT_EQ(vec1, from1);
  EXPECT_FALSE(vec1 != from1);
  EXPECT_EQ(from1, vec1);
  EXPECT_FALSE(from1 != vec1);

  // This verifies that turbo::Span<T> can be compared freely with
  // turbo::Span<const T>.
  const turbo::Span<int> mutable_from1(vec1);
  const turbo::Span<int> mutable_from2(vec2);
  EXPECT_EQ(from1, mutable_from1);
  EXPECT_EQ(mutable_from1, from1);
  EXPECT_EQ(mutable_from1, mutable_from2);
  EXPECT_EQ(mutable_from2, mutable_from1);

  // Comparison between a vector and its slice should be equal for mutable
  // Spans as well.
  EXPECT_EQ(vec1, mutable_from1);
  EXPECT_FALSE(vec1 != mutable_from1);
  EXPECT_EQ(mutable_from1, vec1);
  EXPECT_FALSE(mutable_from1 != vec1);

  // Comparison between convertible-to-Span-of-const and Span-of-mutable. Arrays
  // are used because they're the only value type which converts to a
  // Span-of-mutable. EXPECT_TRUE is used instead of EXPECT_EQ to avoid
  // array-to-pointer decay.
  EXPECT_TRUE(arr1 == mutable_from1);
  EXPECT_FALSE(arr1 != mutable_from1);
  EXPECT_TRUE(mutable_from1 == arr1);
  EXPECT_FALSE(mutable_from1 != arr1);

  // Comparison between convertible-to-Span-of-mutable and Span-of-const
  EXPECT_TRUE(arr2 == from1);
  EXPECT_FALSE(arr2 != from1);
  EXPECT_TRUE(from1 == arr2);
  EXPECT_FALSE(from1 != arr2);

  // With a different size, the array slices should not be equal.
  EXPECT_NE(from1, turbo::Span<const int>(from1).subspan(0, from1.size() - 1));

  // With different contents, the array slices should not be equal.
  ++vec2.back();
  EXPECT_NE(from1, from2);
}

class IntSpanOrderComparisonTest : public testing::Test {
 public:
  IntSpanOrderComparisonTest()
      : arr_before_{1, 2, 3},
        arr_after_{1, 2, 4},
        carr_after_{1, 2, 4},
        vec_before_(std::begin(arr_before_), std::end(arr_before_)),
        vec_after_(std::begin(arr_after_), std::end(arr_after_)),
        before_(vec_before_),
        after_(vec_after_),
        cbefore_(vec_before_),
        cafter_(vec_after_) {}

 protected:
  int arr_before_[3], arr_after_[3];
  const int carr_after_[3];
  std::vector<int> vec_before_, vec_after_;
  turbo::Span<int> before_, after_;
  turbo::Span<const int> cbefore_, cafter_;
};

TEST_F(IntSpanOrderComparisonTest, CompareSpans) {
  EXPECT_TRUE(cbefore_ < cafter_);
  EXPECT_TRUE(cbefore_ <= cafter_);
  EXPECT_TRUE(cafter_ > cbefore_);
  EXPECT_TRUE(cafter_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > cafter_);
  EXPECT_FALSE(cafter_ < cbefore_);

  EXPECT_TRUE(before_ < after_);
  EXPECT_TRUE(before_ <= after_);
  EXPECT_TRUE(after_ > before_);
  EXPECT_TRUE(after_ >= before_);

  EXPECT_FALSE(before_ > after_);
  EXPECT_FALSE(after_ < before_);

  EXPECT_TRUE(cbefore_ < after_);
  EXPECT_TRUE(cbefore_ <= after_);
  EXPECT_TRUE(after_ > cbefore_);
  EXPECT_TRUE(after_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > after_);
  EXPECT_FALSE(after_ < cbefore_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfConstAndContainer) {
  EXPECT_TRUE(cbefore_ < vec_after_);
  EXPECT_TRUE(cbefore_ <= vec_after_);
  EXPECT_TRUE(vec_after_ > cbefore_);
  EXPECT_TRUE(vec_after_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > vec_after_);
  EXPECT_FALSE(vec_after_ < cbefore_);

  EXPECT_TRUE(arr_before_ < cafter_);
  EXPECT_TRUE(arr_before_ <= cafter_);
  EXPECT_TRUE(cafter_ > arr_before_);
  EXPECT_TRUE(cafter_ >= arr_before_);

  EXPECT_FALSE(arr_before_ > cafter_);
  EXPECT_FALSE(cafter_ < arr_before_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfMutableAndContainer) {
  EXPECT_TRUE(vec_before_ < after_);
  EXPECT_TRUE(vec_before_ <= after_);
  EXPECT_TRUE(after_ > vec_before_);
  EXPECT_TRUE(after_ >= vec_before_);

  EXPECT_FALSE(vec_before_ > after_);
  EXPECT_FALSE(after_ < vec_before_);

  EXPECT_TRUE(before_ < carr_after_);
  EXPECT_TRUE(before_ <= carr_after_);
  EXPECT_TRUE(carr_after_ > before_);
  EXPECT_TRUE(carr_after_ >= before_);

  EXPECT_FALSE(before_ > carr_after_);
  EXPECT_FALSE(carr_after_ < before_);
}

TEST_F(IntSpanOrderComparisonTest, EqualSpans) {
  EXPECT_FALSE(before_ < before_);
  EXPECT_TRUE(before_ <= before_);
  EXPECT_FALSE(before_ > before_);
  EXPECT_TRUE(before_ >= before_);
}

TEST_F(IntSpanOrderComparisonTest, Subspans) {
  auto subspan = before_.subspan(0, 1);
  EXPECT_TRUE(subspan < before_);
  EXPECT_TRUE(subspan <= before_);
  EXPECT_TRUE(before_ > subspan);
  EXPECT_TRUE(before_ >= subspan);

  EXPECT_FALSE(subspan > before_);
  EXPECT_FALSE(before_ < subspan);
}

TEST_F(IntSpanOrderComparisonTest, EmptySpans) {
  turbo::Span<int> empty;
  EXPECT_FALSE(empty < empty);
  EXPECT_TRUE(empty <= empty);
  EXPECT_FALSE(empty > empty);
  EXPECT_TRUE(empty >= empty);

  EXPECT_TRUE(empty < before_);
  EXPECT_TRUE(empty <= before_);
  EXPECT_TRUE(before_ > empty);
  EXPECT_TRUE(before_ >= empty);

  EXPECT_FALSE(empty > before_);
  EXPECT_FALSE(before_ < empty);
}

TEST(IntSpan, ExposesContainerTypesAndConsts) {
  turbo::Span<int> slice;
  CheckType<turbo::Span<int>::iterator>(slice.begin());
  EXPECT_TRUE((std::is_convertible<decltype(slice.begin()),
                                   turbo::Span<int>::const_iterator>::value));
  CheckType<turbo::Span<int>::const_iterator>(slice.cbegin());
  EXPECT_TRUE((std::is_convertible<decltype(slice.end()),
                                   turbo::Span<int>::const_iterator>::value));
  CheckType<turbo::Span<int>::const_iterator>(slice.cend());
  CheckType<turbo::Span<int>::reverse_iterator>(slice.rend());
  EXPECT_TRUE(
      (std::is_convertible<decltype(slice.rend()),
                           turbo::Span<int>::const_reverse_iterator>::value));
  CheckType<turbo::Span<int>::const_reverse_iterator>(slice.crend());
  testing::StaticAssertTypeEq<int, turbo::Span<int>::value_type>();
  testing::StaticAssertTypeEq<int, turbo::Span<const int>::value_type>();
  testing::StaticAssertTypeEq<int, turbo::Span<int>::element_type>();
  testing::StaticAssertTypeEq<const int, turbo::Span<const int>::element_type>();
  testing::StaticAssertTypeEq<int*, turbo::Span<int>::pointer>();
  testing::StaticAssertTypeEq<const int*, turbo::Span<const int>::pointer>();
  testing::StaticAssertTypeEq<int&, turbo::Span<int>::reference>();
  testing::StaticAssertTypeEq<const int&, turbo::Span<const int>::reference>();
  testing::StaticAssertTypeEq<const int&, turbo::Span<int>::const_reference>();
  testing::StaticAssertTypeEq<const int&,
                              turbo::Span<const int>::const_reference>();
  EXPECT_EQ(static_cast<turbo::Span<int>::size_type>(-1), turbo::Span<int>::npos);
}

TEST(IntSpan, IteratorsAndReferences) {
  auto accept_pointer = [](int*) {};
  auto accept_reference = [](int&) {};
  auto accept_iterator = [](turbo::Span<int>::iterator) {};
  auto accept_const_iterator = [](turbo::Span<int>::const_iterator) {};
  auto accept_reverse_iterator = [](turbo::Span<int>::reverse_iterator) {};
  auto accept_const_reverse_iterator =
      [](turbo::Span<int>::const_reverse_iterator) {};

  int a[1];
  turbo::Span<int> s = a;

  accept_pointer(s.data());
  accept_iterator(s.begin());
  accept_const_iterator(s.begin());
  accept_const_iterator(s.cbegin());
  accept_iterator(s.end());
  accept_const_iterator(s.end());
  accept_const_iterator(s.cend());
  accept_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.crbegin());
  accept_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.crend());

  accept_reference(s[0]);
  accept_reference(s.at(0));
  accept_reference(s.front());
  accept_reference(s.back());
}

TEST(IntSpan, IteratorsAndReferences_Const) {
  auto accept_pointer = [](int*) {};
  auto accept_reference = [](int&) {};
  auto accept_iterator = [](turbo::Span<int>::iterator) {};
  auto accept_const_iterator = [](turbo::Span<int>::const_iterator) {};
  auto accept_reverse_iterator = [](turbo::Span<int>::reverse_iterator) {};
  auto accept_const_reverse_iterator =
      [](turbo::Span<int>::const_reverse_iterator) {};

  int a[1];
  const turbo::Span<int> s = a;

  accept_pointer(s.data());
  accept_iterator(s.begin());
  accept_const_iterator(s.begin());
  accept_const_iterator(s.cbegin());
  accept_iterator(s.end());
  accept_const_iterator(s.end());
  accept_const_iterator(s.cend());
  accept_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.crbegin());
  accept_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.crend());

  accept_reference(s[0]);
  accept_reference(s.at(0));
  accept_reference(s.front());
  accept_reference(s.back());
}

TEST(IntSpan, NoexceptTest) {
  int a[] = {1, 2, 3};
  std::vector<int> v;
  EXPECT_TRUE(noexcept(turbo::Span<const int>()));
  EXPECT_TRUE(noexcept(turbo::Span<const int>(a, 2)));
  EXPECT_TRUE(noexcept(turbo::Span<const int>(a)));
  EXPECT_TRUE(noexcept(turbo::Span<const int>(v)));
  EXPECT_TRUE(noexcept(turbo::Span<int>(v)));
  EXPECT_TRUE(noexcept(turbo::Span<const int>({1, 2, 3})));
  EXPECT_TRUE(noexcept(turbo::MakeSpan(v)));
  EXPECT_TRUE(noexcept(turbo::MakeSpan(a)));
  EXPECT_TRUE(noexcept(turbo::MakeSpan(a, 2)));
  EXPECT_TRUE(noexcept(turbo::MakeSpan(a, a + 1)));
  EXPECT_TRUE(noexcept(turbo::MakeConstSpan(v)));
  EXPECT_TRUE(noexcept(turbo::MakeConstSpan(a)));
  EXPECT_TRUE(noexcept(turbo::MakeConstSpan(a, 2)));
  EXPECT_TRUE(noexcept(turbo::MakeConstSpan(a, a + 1)));

  turbo::Span<int> s(v);
  EXPECT_TRUE(noexcept(s.data()));
  EXPECT_TRUE(noexcept(s.size()));
  EXPECT_TRUE(noexcept(s.length()));
  EXPECT_TRUE(noexcept(s.empty()));
  EXPECT_TRUE(noexcept(s[0]));
  EXPECT_TRUE(noexcept(s.front()));
  EXPECT_TRUE(noexcept(s.back()));
  EXPECT_TRUE(noexcept(s.begin()));
  EXPECT_TRUE(noexcept(s.cbegin()));
  EXPECT_TRUE(noexcept(s.end()));
  EXPECT_TRUE(noexcept(s.cend()));
  EXPECT_TRUE(noexcept(s.rbegin()));
  EXPECT_TRUE(noexcept(s.crbegin()));
  EXPECT_TRUE(noexcept(s.rend()));
  EXPECT_TRUE(noexcept(s.crend()));
  EXPECT_TRUE(noexcept(s.remove_prefix(0)));
  EXPECT_TRUE(noexcept(s.remove_suffix(0)));
}

// ConstexprTester exercises expressions in a constexpr context. Simply placing
// the expression in a constexpr function is not enough, as some compilers will
// simply compile the constexpr function as runtime code. Using template
// parameters forces compile-time execution.
template <int i>
struct ConstexprTester {};

#define TURBO_TEST_CONSTEXPR(expr)                       \
  do {                                                  \
    TURBO_ATTRIBUTE_UNUSED ConstexprTester<(expr, 1)> t; \
  } while (0)

struct ContainerWithConstexprMethods {
  constexpr int size() const { return 1; }
  constexpr const int* data() const { return &i; }
  const int i;
};

TEST(ConstIntSpan, ConstexprTest) {
  static constexpr int a[] = {1, 2, 3};
  static constexpr int sized_arr[2] = {1, 2};
  static constexpr ContainerWithConstexprMethods c{1};
  TURBO_TEST_CONSTEXPR(turbo::Span<const int>());
  TURBO_TEST_CONSTEXPR(turbo::Span<const int>(a, 2));
  TURBO_TEST_CONSTEXPR(turbo::Span<const int>(sized_arr));
  TURBO_TEST_CONSTEXPR(turbo::Span<const int>(c));
  TURBO_TEST_CONSTEXPR(turbo::MakeSpan(&a[0], 1));
  TURBO_TEST_CONSTEXPR(turbo::MakeSpan(c));
  TURBO_TEST_CONSTEXPR(turbo::MakeSpan(a));
  TURBO_TEST_CONSTEXPR(turbo::MakeConstSpan(&a[0], 1));
  TURBO_TEST_CONSTEXPR(turbo::MakeConstSpan(c));
  TURBO_TEST_CONSTEXPR(turbo::MakeConstSpan(a));

  constexpr turbo::Span<const int> span = c;
  TURBO_TEST_CONSTEXPR(span.data());
  TURBO_TEST_CONSTEXPR(span.size());
  TURBO_TEST_CONSTEXPR(span.length());
  TURBO_TEST_CONSTEXPR(span.empty());
  TURBO_TEST_CONSTEXPR(span.begin());
  TURBO_TEST_CONSTEXPR(span.cbegin());
  TURBO_TEST_CONSTEXPR(span.subspan(0, 0));
  TURBO_TEST_CONSTEXPR(span.first(1));
  TURBO_TEST_CONSTEXPR(span.last(1));
  TURBO_TEST_CONSTEXPR(span[0]);
}

struct BigStruct {
  char bytes[10000];
};

TEST(Span, SpanSize) {
  EXPECT_LE(sizeof(turbo::Span<int>), 2 * sizeof(void*));
  EXPECT_LE(sizeof(turbo::Span<BigStruct>), 2 * sizeof(void*));
}

TEST(Span, Hash) {
  int array[] = {1, 2, 3, 4};
  int array2[] = {1, 2, 3};
  using T = turbo::Span<const int>;
  EXPECT_TRUE(turbo::VerifyTypeImplementsTurboHashCorrectly(
      {// Empties
       T(), T(nullptr, 0), T(array, 0), T(array2, 0),
       // Different array with same value
       T(array, 3), T(array2), T({1, 2, 3}),
       // Same array, but different length
       T(array, 1), T(array, 2),
       // Same length, but different array
       T(array + 1, 2), T(array + 2, 2)}));
}

}  // namespace
