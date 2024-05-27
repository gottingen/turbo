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

// Unit tests for all join.h functions

#include <turbo/strings/str_join.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/base/macros.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>

namespace {

TEST(StrJoin, APIExamples) {
  {
    // Collection of strings
    std::vector<std::string> v = {"foo", "bar", "baz"};
    EXPECT_EQ("foo-bar-baz", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of turbo::string_view
    std::vector<turbo::string_view> v = {"foo", "bar", "baz"};
    EXPECT_EQ("foo-bar-baz", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of const char*
    std::vector<const char*> v = {"foo", "bar", "baz"};
    EXPECT_EQ("foo-bar-baz", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of non-const char*
    std::string a = "foo", b = "bar", c = "baz";
    std::vector<char*> v = {&a[0], &b[0], &c[0]};
    EXPECT_EQ("foo-bar-baz", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of ints
    std::vector<int> v = {1, 2, 3, -4};
    EXPECT_EQ("1-2-3--4", turbo::StrJoin(v, "-"));
  }

  {
    // Literals passed as a std::initializer_list
    std::string s = turbo::StrJoin({"a", "b", "c"}, "-");
    EXPECT_EQ("a-b-c", s);
  }
  {
    // Join a std::tuple<T...>.
    std::string s = turbo::StrJoin(std::make_tuple(123, "abc", 0.456), "-");
    EXPECT_EQ("123-abc-0.456", s);
  }

  {
    // Collection of unique_ptrs
    std::vector<std::unique_ptr<int>> v;
    v.emplace_back(new int(1));
    v.emplace_back(new int(2));
    v.emplace_back(new int(3));
    EXPECT_EQ("1-2-3", turbo::StrJoin(v, "-"));
  }

  {
    // Array of ints
    const int a[] = {1, 2, 3, -4};
    EXPECT_EQ("1-2-3--4", turbo::StrJoin(a, a + TURBO_ARRAYSIZE(a), "-"));
  }

  {
    // Collection of pointers
    int x = 1, y = 2, z = 3;
    std::vector<int*> v = {&x, &y, &z};
    EXPECT_EQ("1-2-3", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of pointers to pointers
    int x = 1, y = 2, z = 3;
    int *px = &x, *py = &y, *pz = &z;
    std::vector<int**> v = {&px, &py, &pz};
    EXPECT_EQ("1-2-3", turbo::StrJoin(v, "-"));
  }

  {
    // Collection of pointers to std::string
    std::string a("a"), b("b");
    std::vector<std::string*> v = {&a, &b};
    EXPECT_EQ("a-b", turbo::StrJoin(v, "-"));
  }

  {
    // A std::map, which is a collection of std::pair<>s.
    std::map<std::string, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ("a=1,b=2,c=3", turbo::StrJoin(m, ",", turbo::PairFormatter("=")));
  }

  {
    // Shows turbo::StrSplit and turbo::StrJoin working together. This example is
    // equivalent to s/=/-/g.
    const std::string s = "a=b=c=d";
    EXPECT_EQ("a-b-c-d", turbo::StrJoin(turbo::StrSplit(s, "="), "-"));
  }

  //
  // A few examples of edge cases
  //

  {
    // Empty range yields an empty string.
    std::vector<std::string> v;
    EXPECT_EQ("", turbo::StrJoin(v, "-"));
  }

  {
    // A range of 1 element gives a string with that element but no
    // separator.
    std::vector<std::string> v = {"foo"};
    EXPECT_EQ("foo", turbo::StrJoin(v, "-"));
  }

  {
    // A range with a single empty string element
    std::vector<std::string> v = {""};
    EXPECT_EQ("", turbo::StrJoin(v, "-"));
  }

  {
    // A range with 2 elements, one of which is an empty string
    std::vector<std::string> v = {"a", ""};
    EXPECT_EQ("a-", turbo::StrJoin(v, "-"));
  }

  {
    // A range with 2 empty elements.
    std::vector<std::string> v = {"", ""};
    EXPECT_EQ("-", turbo::StrJoin(v, "-"));
  }

  {
    // A std::vector of bool.
    std::vector<bool> v = {true, false, true};
    EXPECT_EQ("1-0-1", turbo::StrJoin(v, "-"));
  }
}

TEST(StrJoin, CustomFormatter) {
  std::vector<std::string> v{"One", "Two", "Three"};
  {
    std::string joined =
        turbo::StrJoin(v, "", [](std::string* out, const std::string& in) {
          turbo::StrAppend(out, "(", in, ")");
        });
    EXPECT_EQ("(One)(Two)(Three)", joined);
  }
  {
    class ImmovableFormatter {
     public:
      void operator()(std::string* out, const std::string& in) {
        turbo::StrAppend(out, "(", in, ")");
      }
      ImmovableFormatter() {}
      ImmovableFormatter(const ImmovableFormatter&) = delete;
    };
    EXPECT_EQ("(One)(Two)(Three)", turbo::StrJoin(v, "", ImmovableFormatter()));
  }
  {
    class OverloadedFormatter {
     public:
      void operator()(std::string* out, const std::string& in) {
        turbo::StrAppend(out, "(", in, ")");
      }
      void operator()(std::string* out, const std::string& in) const {
        turbo::StrAppend(out, "[", in, "]");
      }
    };
    EXPECT_EQ("(One)(Two)(Three)", turbo::StrJoin(v, "", OverloadedFormatter()));
    const OverloadedFormatter fmt = {};
    EXPECT_EQ("[One][Two][Three]", turbo::StrJoin(v, "", fmt));
  }
}

//
// Tests the Formatters
//

TEST(AlphaNumFormatter, FormatterAPI) {
  // Not an exhaustive test. See strings/strcat_test.h for the exhaustive test
  // of what AlphaNum can convert.
  auto f = turbo::AlphaNumFormatter();
  std::string s;
  f(&s, "Testing: ");
  f(&s, static_cast<int>(1));
  f(&s, static_cast<int16_t>(2));
  f(&s, static_cast<int64_t>(3));
  f(&s, static_cast<float>(4));
  f(&s, static_cast<double>(5));
  f(&s, static_cast<unsigned>(6));
  f(&s, static_cast<size_t>(7));
  f(&s, turbo::string_view(" OK"));
  EXPECT_EQ("Testing: 1234567 OK", s);
}

// Make sure people who are mistakenly using std::vector<bool> even though
// they're not memory-constrained can use turbo::AlphaNumFormatter().
TEST(AlphaNumFormatter, VectorOfBool) {
  auto f = turbo::AlphaNumFormatter();
  std::string s;
  std::vector<bool> v = {true, false, true};
  f(&s, *v.cbegin());
  f(&s, *v.begin());
  f(&s, v[1]);
  EXPECT_EQ("110", s);
}

TEST(AlphaNumFormatter, AlphaNum) {
  auto f = turbo::AlphaNumFormatter();
  std::string s;
  f(&s, turbo::AlphaNum("hello"));
  EXPECT_EQ("hello", s);
}

struct StreamableType {
  std::string contents;
};
inline std::ostream& operator<<(std::ostream& os, const StreamableType& t) {
  os << "Streamable:" << t.contents;
  return os;
}

TEST(StreamFormatter, FormatterAPI) {
  auto f = turbo::StreamFormatter();
  std::string s;
  f(&s, "Testing: ");
  f(&s, static_cast<int>(1));
  f(&s, static_cast<int16_t>(2));
  f(&s, static_cast<int64_t>(3));
  f(&s, static_cast<float>(4));
  f(&s, static_cast<double>(5));
  f(&s, static_cast<unsigned>(6));
  f(&s, static_cast<size_t>(7));
  f(&s, turbo::string_view(" OK "));
  StreamableType streamable = {"object"};
  f(&s, streamable);
  EXPECT_EQ("Testing: 1234567 OK Streamable:object", s);
}

// A dummy formatter that wraps each element in parens. Used in some tests
// below.
struct TestingParenFormatter {
  template <typename T>
  void operator()(std::string* s, const T& t) {
    turbo::StrAppend(s, "(", t, ")");
  }
};

TEST(PairFormatter, FormatterAPI) {
  {
    // Tests default PairFormatter(sep) that uses AlphaNumFormatter for the
    // 'first' and 'second' members.
    const auto f = turbo::PairFormatter("=");
    std::string s;
    f(&s, std::make_pair("a", "b"));
    f(&s, std::make_pair(1, 2));
    EXPECT_EQ("a=b1=2", s);
  }

  {
    // Tests using a custom formatter for the 'first' and 'second' members.
    auto f = turbo::PairFormatter(TestingParenFormatter(), "=",
                                 TestingParenFormatter());
    std::string s;
    f(&s, std::make_pair("a", "b"));
    f(&s, std::make_pair(1, 2));
    EXPECT_EQ("(a)=(b)(1)=(2)", s);
  }
}

TEST(DereferenceFormatter, FormatterAPI) {
  {
    // Tests wrapping the default AlphaNumFormatter.
    const turbo::strings_internal::DereferenceFormatterImpl<
        turbo::strings_internal::AlphaNumFormatterImpl>
        f;
    int x = 1, y = 2, z = 3;
    std::string s;
    f(&s, &x);
    f(&s, &y);
    f(&s, &z);
    EXPECT_EQ("123", s);
  }

  {
    // Tests wrapping std::string's default formatter.
    turbo::strings_internal::DereferenceFormatterImpl<
        turbo::strings_internal::DefaultFormatter<std::string>::Type>
        f;

    std::string x = "x";
    std::string y = "y";
    std::string z = "z";
    std::string s;
    f(&s, &x);
    f(&s, &y);
    f(&s, &z);
    EXPECT_EQ(s, "xyz");
  }

  {
    // Tests wrapping a custom formatter.
    auto f = turbo::DereferenceFormatter(TestingParenFormatter());
    int x = 1, y = 2, z = 3;
    std::string s;
    f(&s, &x);
    f(&s, &y);
    f(&s, &z);
    EXPECT_EQ("(1)(2)(3)", s);
  }

  {
    turbo::strings_internal::DereferenceFormatterImpl<
        turbo::strings_internal::AlphaNumFormatterImpl>
        f;
    auto x = std::unique_ptr<int>(new int(1));
    auto y = std::unique_ptr<int>(new int(2));
    auto z = std::unique_ptr<int>(new int(3));
    std::string s;
    f(&s, x);
    f(&s, y);
    f(&s, z);
    EXPECT_EQ("123", s);
  }
}

//
// Tests the interfaces for the 4 public Join function overloads. The semantics
// of the algorithm is covered in the above APIExamples test.
//
TEST(StrJoin, PublicAPIOverloads) {
  std::vector<std::string> v = {"a", "b", "c"};

  // Iterators + formatter
  EXPECT_EQ("a-b-c",
            turbo::StrJoin(v.begin(), v.end(), "-", turbo::AlphaNumFormatter()));
  // Range + formatter
  EXPECT_EQ("a-b-c", turbo::StrJoin(v, "-", turbo::AlphaNumFormatter()));
  // Iterators, no formatter
  EXPECT_EQ("a-b-c", turbo::StrJoin(v.begin(), v.end(), "-"));
  // Range, no formatter
  EXPECT_EQ("a-b-c", turbo::StrJoin(v, "-"));
}

TEST(StrJoin, Array) {
  const turbo::string_view a[] = {"a", "b", "c"};
  EXPECT_EQ("a-b-c", turbo::StrJoin(a, "-"));
}

TEST(StrJoin, InitializerList) {
  { EXPECT_EQ("a-b-c", turbo::StrJoin({"a", "b", "c"}, "-")); }

  {
    auto a = {"a", "b", "c"};
    EXPECT_EQ("a-b-c", turbo::StrJoin(a, "-"));
  }

  {
    std::initializer_list<const char*> a = {"a", "b", "c"};
    EXPECT_EQ("a-b-c", turbo::StrJoin(a, "-"));
  }

  {
    std::initializer_list<std::string> a = {"a", "b", "c"};
    EXPECT_EQ("a-b-c", turbo::StrJoin(a, "-"));
  }

  {
    std::initializer_list<turbo::string_view> a = {"a", "b", "c"};
    EXPECT_EQ("a-b-c", turbo::StrJoin(a, "-"));
  }

  {
    // Tests initializer_list with a non-default formatter
    auto a = {"a", "b", "c"};
    TestingParenFormatter f;
    EXPECT_EQ("(a)-(b)-(c)", turbo::StrJoin(a, "-", f));
  }

  {
    // initializer_list of ints
    EXPECT_EQ("1-2-3", turbo::StrJoin({1, 2, 3}, "-"));
  }

  {
    // Tests initializer_list of ints with a non-default formatter
    auto a = {1, 2, 3};
    TestingParenFormatter f;
    EXPECT_EQ("(1)-(2)-(3)", turbo::StrJoin(a, "-", f));
  }
}

TEST(StrJoin, StringViewInitializerList) {
  {
    // Tests initializer_list of string_views
    std::string b = "b";
    EXPECT_EQ("a-b-c", turbo::StrJoin({"a", b, "c"}, "-"));
  }
  {
    // Tests initializer_list of string_views with a non-default formatter
    TestingParenFormatter f;
    std::string b = "b";
    EXPECT_EQ("(a)-(b)-(c)", turbo::StrJoin({"a", b, "c"}, "-", f));
  }

  class NoCopy {
   public:
    explicit NoCopy(turbo::string_view view) : view_(view) {}
    NoCopy(const NoCopy&) = delete;
    operator turbo::string_view() { return view_; }  // NOLINT
   private:
    turbo::string_view view_;
  };
  {
    // Tests initializer_list of string_views preferred over initializer_list<T>
    // for T that is implicitly convertible to string_view
    EXPECT_EQ("a-b-c",
              turbo::StrJoin({NoCopy("a"), NoCopy("b"), NoCopy("c")}, "-"));
  }
  {
    // Tests initializer_list of string_views preferred over initializer_list<T>
    // for T that is implicitly convertible to string_view
    TestingParenFormatter f;
    EXPECT_EQ("(a)-(b)-(c)",
              turbo::StrJoin({NoCopy("a"), NoCopy("b"), NoCopy("c")}, "-", f));
  }
}

TEST(StrJoin, Tuple) {
  EXPECT_EQ("", turbo::StrJoin(std::make_tuple(), "-"));
  EXPECT_EQ("hello", turbo::StrJoin(std::make_tuple("hello"), "-"));

  int x(10);
  std::string y("hello");
  double z(3.14);
  EXPECT_EQ("10-hello-3.14", turbo::StrJoin(std::make_tuple(x, y, z), "-"));

  // Faster! Faster!!
  EXPECT_EQ("10-hello-3.14",
            turbo::StrJoin(std::make_tuple(x, std::cref(y), z), "-"));

  struct TestFormatter {
    char buffer[128];
    void operator()(std::string* out, int v) {
      snprintf(buffer, sizeof(buffer), "%#.8x", v);
      out->append(buffer);
    }
    void operator()(std::string* out, double v) {
      snprintf(buffer, sizeof(buffer), "%#.0f", v);
      out->append(buffer);
    }
    void operator()(std::string* out, const std::string& v) {
      snprintf(buffer, sizeof(buffer), "%.4s", v.c_str());
      out->append(buffer);
    }
  };
  EXPECT_EQ("0x0000000a-hell-3.",
            turbo::StrJoin(std::make_tuple(x, y, z), "-", TestFormatter()));
  EXPECT_EQ(
      "0x0000000a-hell-3.",
      turbo::StrJoin(std::make_tuple(x, std::cref(y), z), "-", TestFormatter()));
  EXPECT_EQ("0x0000000a-hell-3.",
            turbo::StrJoin(std::make_tuple(&x, &y, &z), "-",
                          turbo::DereferenceFormatter(TestFormatter())));
  EXPECT_EQ("0x0000000a-hell-3.",
            turbo::StrJoin(std::make_tuple(turbo::make_unique<int>(x),
                                          turbo::make_unique<std::string>(y),
                                          turbo::make_unique<double>(z)),
                          "-", turbo::DereferenceFormatter(TestFormatter())));
  EXPECT_EQ("0x0000000a-hell-3.",
            turbo::StrJoin(std::make_tuple(turbo::make_unique<int>(x), &y, &z),
                          "-", turbo::DereferenceFormatter(TestFormatter())));
}

// A minimal value type for `StrJoin` inputs.
// Used to ensure we do not excessively require more a specific type, such as a
// `string_view`.
//
// Anything that can be  `data()` and `size()` is OK.
class TestValue {
 public:
  TestValue(const char* data, size_t size) : data_(data), size_(size) {}
  const char* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  const char* data_;
  size_t size_;
};

// A minimal C++20 forward iterator, used to test that we do not impose
// excessive requirements on StrJoin inputs.
//
// The 2 main differences between pre-C++20 LegacyForwardIterator and the
// C++20 ForwardIterator are:
// 1. `operator->` is not required in C++20.
// 2. `operator*` result does not need to be an lvalue (a reference).
//
// The `operator->` requirement was removed on page 17 in:
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1037r0.pdf
//
// See the `[iterator.requirements]` section of the C++ standard.
//
// The value type is a template parameter so that we can test the behaviour
// of `StrJoin` specializations, e.g. the NoFormatter specialization for
// `string_view`.
template <typename ValueT>
class TestIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = ValueT;
  using pointer = void;
  using reference = const value_type&;
  using difference_type = int;

  // `data` must outlive the result.
  static TestIterator begin(const std::vector<turbo::string_view>& data) {
    return TestIterator(&data, 0);
  }

  static TestIterator end(const std::vector<turbo::string_view>& data) {
    return TestIterator(nullptr, data.size());
  }

  bool operator==(const TestIterator& other) const {
    return pos_ == other.pos_;
  }
  bool operator!=(const TestIterator& other) const {
    return pos_ != other.pos_;
  }

  // This deliberately returns a `prvalue`.
  // The requirement to return a reference was removed in C++20.
  value_type operator*() const {
    return ValueT((*data_)[pos_].data(), (*data_)[pos_].size());
  }

  // `operator->()` is deliberately omitted.
  // The requirement to provide it was removed in C++20.

  TestIterator& operator++() {
    ++pos_;
    return *this;
  }

  TestIterator operator++(int) {
    TestIterator result = *this;
    ++(*this);
    return result;
  }

  TestIterator& operator--() {
    --pos_;
    return *this;
  }

  TestIterator operator--(int) {
    TestIterator result = *this;
    --(*this);
    return result;
  }

 private:
  TestIterator(const std::vector<turbo::string_view>* data, size_t pos)
      : data_(data), pos_(pos) {}

  const std::vector<turbo::string_view>* data_;
  size_t pos_;
};

template <typename ValueT>
class TestIteratorRange {
 public:
  // `data` must be non-null and must outlive the result.
  explicit TestIteratorRange(const std::vector<turbo::string_view>& data)
      : begin_(TestIterator<ValueT>::begin(data)),
        end_(TestIterator<ValueT>::end(data)) {}

  const TestIterator<ValueT>& begin() const { return begin_; }
  const TestIterator<ValueT>& end() const { return end_; }

 private:
  TestIterator<ValueT> begin_;
  TestIterator<ValueT> end_;
};

TEST(StrJoin, TestIteratorRequirementsNoFormatter) {
  const std::vector<turbo::string_view> a = {"a", "b", "c"};

  // When the value type is string-like (`std::string` or `string_view`),
  // the NoFormatter template specialization is used internally.
  EXPECT_EQ("a-b-c",
            turbo::StrJoin(TestIteratorRange<turbo::string_view>(a), "-"));
}

TEST(StrJoin, TestIteratorRequirementsCustomFormatter) {
  const std::vector<turbo::string_view> a = {"a", "b", "c"};
  EXPECT_EQ("a-b-c",
            turbo::StrJoin(TestIteratorRange<TestValue>(a), "-",
                          [](std::string* out, const TestValue& value) {
                            turbo::StrAppend(
                                out,
                                turbo::string_view(value.data(), value.size()));
                          }));
}

}  // namespace
