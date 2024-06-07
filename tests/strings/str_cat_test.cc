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

// Unit tests for all str_cat.h functions

#include <turbo/strings/str_cat.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/strings/str_format.h>
#include <turbo/strings/string_view.h>

#ifdef __ANDROID__
// Android assert messages only go to system log, so death tests cannot inspect
// the message for matching.
#define TURBO_EXPECT_DEBUG_DEATH(statement, regex) \
  EXPECT_DEBUG_DEATH(statement, ".*")
#else
#define TURBO_EXPECT_DEBUG_DEATH(statement, regex) \
  EXPECT_DEBUG_DEATH(statement, regex)
#endif

namespace {

// Test turbo::str_cat of ints and longs of various sizes and signdedness.
TEST(str_cat, Ints) {
  const short s = -1;  // NOLINT(runtime/int)
  const uint16_t us = 2;
  const int i = -3;
  const unsigned int ui = 4;
  const long l = -5;                 // NOLINT(runtime/int)
  const unsigned long ul = 6;        // NOLINT(runtime/int)
  const long long ll = -7;           // NOLINT(runtime/int)
  const unsigned long long ull = 8;  // NOLINT(runtime/int)
  const ptrdiff_t ptrdiff = -9;
  const size_t size = 10;
  const intptr_t intptr = -12;
  const uintptr_t uintptr = 13;
  std::string answer;
  answer = turbo::str_cat(s, us);
  EXPECT_EQ(answer, "-12");
  answer = turbo::str_cat(i, ui);
  EXPECT_EQ(answer, "-34");
  answer = turbo::str_cat(l, ul);
  EXPECT_EQ(answer, "-56");
  answer = turbo::str_cat(ll, ull);
  EXPECT_EQ(answer, "-78");
  answer = turbo::str_cat(ptrdiff, size);
  EXPECT_EQ(answer, "-910");
  answer = turbo::str_cat(ptrdiff, intptr);
  EXPECT_EQ(answer, "-9-12");
  answer = turbo::str_cat(uintptr, 0);
  EXPECT_EQ(answer, "130");
}

TEST(str_cat, Enums) {
  enum SmallNumbers { One = 1, Ten = 10 } e = Ten;
  EXPECT_EQ("10", turbo::str_cat(e));
  EXPECT_EQ("-5", turbo::str_cat(SmallNumbers(-5)));

  enum class Option { Boxers = 1, Briefs = -1 };

  EXPECT_EQ("-1", turbo::str_cat(Option::Briefs));

  enum class Airplane : uint64_t {
    Airbus = 1,
    Boeing = 1000,
    Canary = 10000000000  // too big for "int"
  };

  EXPECT_EQ("10000000000", turbo::str_cat(Airplane::Canary));

  enum class TwoGig : int32_t {
    TwoToTheZero = 1,
    TwoToTheSixteenth = 1 << 16,
    TwoToTheThirtyFirst = INT32_MIN
  };
  EXPECT_EQ("65536", turbo::str_cat(TwoGig::TwoToTheSixteenth));
  EXPECT_EQ("-2147483648", turbo::str_cat(TwoGig::TwoToTheThirtyFirst));
  EXPECT_EQ("-1", turbo::str_cat(static_cast<TwoGig>(-1)));

  enum class FourGig : uint32_t {
    TwoToTheZero = 1,
    TwoToTheSixteenth = 1 << 16,
    TwoToTheThirtyFirst = 1U << 31  // too big for "int"
  };
  EXPECT_EQ("65536", turbo::str_cat(FourGig::TwoToTheSixteenth));
  EXPECT_EQ("2147483648", turbo::str_cat(FourGig::TwoToTheThirtyFirst));
  EXPECT_EQ("4294967295", turbo::str_cat(static_cast<FourGig>(-1)));

  EXPECT_EQ("10000000000", turbo::str_cat(Airplane::Canary));
}

TEST(str_cat, Basics) {
  std::string result;

  std::string strs[] = {"Hello", "Cruel", "World"};

  std::string stdstrs[] = {
    "std::Hello",
    "std::Cruel",
    "std::World"
  };

  std::string_view pieces[] = {"Hello", "Cruel", "World"};

  const char* c_strs[] = {
    "Hello",
    "Cruel",
    "World"
  };

  int32_t i32s[] = {'H', 'C', 'W'};
  uint64_t ui64s[] = {12345678910LL, 10987654321LL};

  EXPECT_EQ(turbo::str_cat(), "");

  result = turbo::str_cat(false, true, 2, 3);
  EXPECT_EQ(result, "0123");

  result = turbo::str_cat(-1);
  EXPECT_EQ(result, "-1");

  result = turbo::str_cat(turbo::six_digits(0.5));
  EXPECT_EQ(result, "0.5");

  result = turbo::str_cat(strs[1], pieces[2]);
  EXPECT_EQ(result, "CruelWorld");

  result = turbo::str_cat(stdstrs[1], " ", stdstrs[2]);
  EXPECT_EQ(result, "std::Cruel std::World");

  result = turbo::str_cat(strs[0], ", ", pieces[2]);
  EXPECT_EQ(result, "Hello, World");

  result = turbo::str_cat(strs[0], ", ", strs[1], " ", strs[2], "!");
  EXPECT_EQ(result, "Hello, Cruel World!");

  result = turbo::str_cat(pieces[0], ", ", pieces[1], " ", pieces[2]);
  EXPECT_EQ(result, "Hello, Cruel World");

  result = turbo::str_cat(c_strs[0], ", ", c_strs[1], " ", c_strs[2]);
  EXPECT_EQ(result, "Hello, Cruel World");

  result = turbo::str_cat("ASCII ", i32s[0], ", ", i32s[1], " ", i32s[2], "!");
  EXPECT_EQ(result, "ASCII 72, 67 87!");

  result = turbo::str_cat(ui64s[0], ", ", ui64s[1], "!");
  EXPECT_EQ(result, "12345678910, 10987654321!");

  std::string one =
      "1";  // Actually, it's the size of this string that we want; a
            // 64-bit build distinguishes between size_t and uint64_t,
            // even though they're both unsigned 64-bit values.
  result = turbo::str_cat("And a ", one.size(), " and a ",
                        &result[2] - &result[0], " and a ", one, " 2 3 4", "!");
  EXPECT_EQ(result, "And a 1 and a 2 and a 1 2 3 4!");

  // result = turbo::str_cat("Single chars won't compile", '!');
  // result = turbo::str_cat("Neither will nullptrs", nullptr);
  result =
      turbo::str_cat("To output a char by ASCII/numeric value, use +: ", '!' + 0);
  EXPECT_EQ(result, "To output a char by ASCII/numeric value, use +: 33");

  float f = 100000.5;
  result = turbo::str_cat("A hundred K and a half is ", turbo::six_digits(f));
  EXPECT_EQ(result, "A hundred K and a half is 100000");

  f = 100001.5;
  result =
      turbo::str_cat("A hundred K and one and a half is ", turbo::six_digits(f));
  EXPECT_EQ(result, "A hundred K and one and a half is 100002");

  double d = 100000.5;
  d *= d;
  result =
      turbo::str_cat("A hundred K and a half squared is ", turbo::six_digits(d));
  EXPECT_EQ(result, "A hundred K and a half squared is 1.00001e+10");

  result = turbo::str_cat(1, 2, 333, 4444, 55555, 666666, 7777777, 88888888,
                        999999999);
  EXPECT_EQ(result, "12333444455555666666777777788888888999999999");
}

TEST(str_cat, CornerCases) {
  std::string result;

  result = turbo::str_cat("");  // NOLINT
  EXPECT_EQ(result, "");
  result = turbo::str_cat("", "");
  EXPECT_EQ(result, "");
  result = turbo::str_cat("", "", "");
  EXPECT_EQ(result, "");
  result = turbo::str_cat("", "", "", "");
  EXPECT_EQ(result, "");
  result = turbo::str_cat("", "", "", "", "");
  EXPECT_EQ(result, "");
}

TEST(str_cat, NullConstCharPtr) {
  const char* null = nullptr;
  EXPECT_EQ(turbo::str_cat("mon", null, "key"), "monkey");
}

// A minimal allocator that uses malloc().
template <typename T>
struct Mallocator {
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;

  size_type max_size() const {
    return size_t(std::numeric_limits<size_type>::max()) / sizeof(value_type);
  }
  template <typename U>
  struct rebind {
    typedef Mallocator<U> other;
  };
  Mallocator() = default;
  template <class U>
  Mallocator(const Mallocator<U>&) {}  // NOLINT(runtime/explicit)

  T* allocate(size_t n) { return static_cast<T*>(std::malloc(n * sizeof(T))); }
  void deallocate(T* p, size_t) { std::free(p); }
};
template <typename T, typename U>
bool operator==(const Mallocator<T>&, const Mallocator<U>&) {
  return true;
}
template <typename T, typename U>
bool operator!=(const Mallocator<T>&, const Mallocator<U>&) {
  return false;
}

TEST(str_cat, CustomAllocator) {
  using mstring =
      std::basic_string<char, std::char_traits<char>, Mallocator<char>>;
  const mstring str1("PARACHUTE OFF A BLIMP INTO MOSCONE!!");

  const mstring str2("Read this book about coffee tables");

  std::string result = turbo::str_cat(str1, str2);
  EXPECT_EQ(result,
            "PARACHUTE OFF A BLIMP INTO MOSCONE!!"
            "Read this book about coffee tables");
}

TEST(str_cat, MaxArgs) {
  std::string result;
  // Test 10 up to 26 arguments, the old maximum
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a");
  EXPECT_EQ(result, "123456789a");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b");
  EXPECT_EQ(result, "123456789ab");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c");
  EXPECT_EQ(result, "123456789abc");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d");
  EXPECT_EQ(result, "123456789abcd");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e");
  EXPECT_EQ(result, "123456789abcde");
  result =
      turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f");
  EXPECT_EQ(result, "123456789abcdef");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g");
  EXPECT_EQ(result, "123456789abcdefg");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h");
  EXPECT_EQ(result, "123456789abcdefgh");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i");
  EXPECT_EQ(result, "123456789abcdefghi");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j");
  EXPECT_EQ(result, "123456789abcdefghij");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k");
  EXPECT_EQ(result, "123456789abcdefghijk");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l");
  EXPECT_EQ(result, "123456789abcdefghijkl");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l", "m");
  EXPECT_EQ(result, "123456789abcdefghijklm");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l", "m", "n");
  EXPECT_EQ(result, "123456789abcdefghijklmn");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l", "m", "n", "o");
  EXPECT_EQ(result, "123456789abcdefghijklmno");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l", "m", "n", "o", "p");
  EXPECT_EQ(result, "123456789abcdefghijklmnop");
  result = turbo::str_cat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                        "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q");
  EXPECT_EQ(result, "123456789abcdefghijklmnopq");
  // No limit thanks to C++11's variadic templates
  result = turbo::str_cat(
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "a", "b", "c", "d", "e", "f", "g", "h",
      "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
      "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
      "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z");
  EXPECT_EQ(result,
            "12345678910abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

TEST(str_append, Basics) {
  std::string result = "existing text";

  std::string strs[] = {"Hello", "Cruel", "World"};

  std::string stdstrs[] = {
    "std::Hello",
    "std::Cruel",
    "std::World"
  };

  std::string_view pieces[] = {"Hello", "Cruel", "World"};

  const char* c_strs[] = {
    "Hello",
    "Cruel",
    "World"
  };

  int32_t i32s[] = {'H', 'C', 'W'};
  uint64_t ui64s[] = {12345678910LL, 10987654321LL};

  std::string::size_type old_size = result.size();
  turbo::str_append(&result);
  EXPECT_EQ(result.size(), old_size);

  old_size = result.size();
  turbo::str_append(&result, strs[0]);
  EXPECT_EQ(result.substr(old_size), "Hello");

  old_size = result.size();
  turbo::str_append(&result, strs[1], pieces[2]);
  EXPECT_EQ(result.substr(old_size), "CruelWorld");

  old_size = result.size();
  turbo::str_append(&result, stdstrs[0], ", ", pieces[2]);
  EXPECT_EQ(result.substr(old_size), "std::Hello, World");

  old_size = result.size();
  turbo::str_append(&result, strs[0], ", ", stdstrs[1], " ", strs[2], "!");
  EXPECT_EQ(result.substr(old_size), "Hello, std::Cruel World!");

  old_size = result.size();
  turbo::str_append(&result, pieces[0], ", ", pieces[1], " ", pieces[2]);
  EXPECT_EQ(result.substr(old_size), "Hello, Cruel World");

  old_size = result.size();
  turbo::str_append(&result, c_strs[0], ", ", c_strs[1], " ", c_strs[2]);
  EXPECT_EQ(result.substr(old_size), "Hello, Cruel World");

  old_size = result.size();
  turbo::str_append(&result, "ASCII ", i32s[0], ", ", i32s[1], " ", i32s[2], "!");
  EXPECT_EQ(result.substr(old_size), "ASCII 72, 67 87!");

  old_size = result.size();
  turbo::str_append(&result, ui64s[0], ", ", ui64s[1], "!");
  EXPECT_EQ(result.substr(old_size), "12345678910, 10987654321!");

  std::string one =
      "1";  // Actually, it's the size of this string that we want; a
            // 64-bit build distinguishes between size_t and uint64_t,
            // even though they're both unsigned 64-bit values.
  old_size = result.size();
  turbo::str_append(&result, "And a ", one.size(), " and a ",
                  &result[2] - &result[0], " and a ", one, " 2 3 4", "!");
  EXPECT_EQ(result.substr(old_size), "And a 1 and a 2 and a 1 2 3 4!");

  // result = turbo::str_cat("Single chars won't compile", '!');
  // result = turbo::str_cat("Neither will nullptrs", nullptr);
  old_size = result.size();
  turbo::str_append(&result,
                  "To output a char by ASCII/numeric value, use +: ", '!' + 0);
  EXPECT_EQ(result.substr(old_size),
            "To output a char by ASCII/numeric value, use +: 33");

  // Test 9 arguments, the old maximum
  old_size = result.size();
  turbo::str_append(&result, 1, 22, 333, 4444, 55555, 666666, 7777777, 88888888,
                  9);
  EXPECT_EQ(result.substr(old_size), "1223334444555556666667777777888888889");

  // No limit thanks to C++11's variadic templates
  old_size = result.size();
  turbo::str_append(
      &result, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,                           //
      "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",  //
      "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",  //
      "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",  //
      "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",  //
      "No limit thanks to C++11's variadic templates");
  EXPECT_EQ(result.substr(old_size),
            "12345678910abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "No limit thanks to C++11's variadic templates");
}

TEST(str_cat, VectorBoolReferenceTypes) {
  std::vector<bool> v;
  v.push_back(true);
  v.push_back(false);
  std::vector<bool> const& cv = v;
  // Test that vector<bool>::reference and vector<bool>::const_reference
  // are handled as if the were really bool types and not the proxy types
  // they really are.
  std::string result = turbo::str_cat(v[0], v[1], cv[0], cv[1]); // NOLINT
  EXPECT_EQ(result, "1010");
}

// Passing nullptr to memcpy is undefined behavior and this test
// provides coverage of codepaths that handle empty strings with nullptrs.
TEST(str_cat, AvoidsMemcpyWithNullptr) {
  EXPECT_EQ(turbo::str_cat(42, std::string_view{}), "42");

  // Cover CatPieces code.
  EXPECT_EQ(turbo::str_cat(1, 2, 3, 4, 5, std::string_view{}), "12345");

  // Cover AppendPieces.
  std::string result;
  turbo::str_append(&result, 1, 2, 3, 4, 5, std::string_view{});
  EXPECT_EQ(result, "12345");
}

#if GTEST_HAS_DEATH_TEST
TEST(str_append, Death) {
  std::string s = "self";
  // on linux it's "assertion", on mac it's "Assertion",
  // on chromiumos it's "Assertion ... failed".
  TURBO_EXPECT_DEBUG_DEATH(turbo::str_append(&s, s.c_str() + 1),
                          "ssertion.*failed");
  TURBO_EXPECT_DEBUG_DEATH(turbo::str_append(&s, s), "ssertion.*failed");
}
#endif  // GTEST_HAS_DEATH_TEST

TEST(str_append, CornerCases) {
  std::string result;
  turbo::str_append(&result, "");
  EXPECT_EQ(result, "");
  turbo::str_append(&result, "", "");
  EXPECT_EQ(result, "");
  turbo::str_append(&result, "", "", "");
  EXPECT_EQ(result, "");
  turbo::str_append(&result, "", "", "", "");
  EXPECT_EQ(result, "");
  turbo::str_append(&result, "", "", "", "", "");
  EXPECT_EQ(result, "");
}

TEST(str_append, CornerCasesNonEmptyAppend) {
  for (std::string result : {"hello", "a string too long to fit in the SSO"}) {
    const std::string expected = result;
    turbo::str_append(&result, "");
    EXPECT_EQ(result, expected);
    turbo::str_append(&result, "", "");
    EXPECT_EQ(result, expected);
    turbo::str_append(&result, "", "", "");
    EXPECT_EQ(result, expected);
    turbo::str_append(&result, "", "", "", "");
    EXPECT_EQ(result, expected);
    turbo::str_append(&result, "", "", "", "", "");
    EXPECT_EQ(result, expected);
  }
}

template <typename IntType>
void CheckHex(IntType v, const char* nopad_format, const char* zeropad_format,
              const char* spacepad_format) {
  char expected[256];

  std::string actual = turbo::str_cat(turbo::Hex(v, turbo::kNoPad));
  snprintf(expected, sizeof(expected), nopad_format, v);
  EXPECT_EQ(expected, actual) << " decimal value " << v;

  for (int spec = turbo::kZeroPad2; spec <= turbo::kZeroPad20; ++spec) {
    std::string actual =
        turbo::str_cat(turbo::Hex(v, static_cast<turbo::PadSpec>(spec)));
    snprintf(expected, sizeof(expected), zeropad_format,
             spec - turbo::kZeroPad2 + 2, v);
    EXPECT_EQ(expected, actual) << " decimal value " << v;
  }

  for (int spec = turbo::kSpacePad2; spec <= turbo::kSpacePad20; ++spec) {
    std::string actual =
        turbo::str_cat(turbo::Hex(v, static_cast<turbo::PadSpec>(spec)));
    snprintf(expected, sizeof(expected), spacepad_format,
             spec - turbo::kSpacePad2 + 2, v);
    EXPECT_EQ(expected, actual) << " decimal value " << v;
  }
}

template <typename IntType>
void CheckDec(IntType v, const char* nopad_format, const char* zeropad_format,
              const char* spacepad_format) {
  char expected[256];

  std::string actual = turbo::str_cat(turbo::Dec(v, turbo::kNoPad));
  snprintf(expected, sizeof(expected), nopad_format, v);
  EXPECT_EQ(expected, actual) << " decimal value " << v;

  for (int spec = turbo::kZeroPad2; spec <= turbo::kZeroPad20; ++spec) {
    std::string actual =
        turbo::str_cat(turbo::Dec(v, static_cast<turbo::PadSpec>(spec)));
    snprintf(expected, sizeof(expected), zeropad_format,
             spec - turbo::kZeroPad2 + 2, v);
    EXPECT_EQ(expected, actual)
        << " decimal value " << v << " format '" << zeropad_format
        << "' digits " << (spec - turbo::kZeroPad2 + 2);
  }

  for (int spec = turbo::kSpacePad2; spec <= turbo::kSpacePad20; ++spec) {
    std::string actual =
        turbo::str_cat(turbo::Dec(v, static_cast<turbo::PadSpec>(spec)));
    snprintf(expected, sizeof(expected), spacepad_format,
             spec - turbo::kSpacePad2 + 2, v);
    EXPECT_EQ(expected, actual)
        << " decimal value " << v << " format '" << spacepad_format
        << "' digits " << (spec - turbo::kSpacePad2 + 2);
  }
}

void CheckHexDec64(uint64_t v) {
  unsigned long long ullv = v;  // NOLINT(runtime/int)

  CheckHex(ullv, "%llx", "%0*llx", "%*llx");
  CheckDec(ullv, "%llu", "%0*llu", "%*llu");

  long long llv = static_cast<long long>(ullv);  // NOLINT(runtime/int)
  CheckDec(llv, "%lld", "%0*lld", "%*lld");

  if (sizeof(v) == sizeof(&v)) {
    auto uintptr = static_cast<uintptr_t>(v);
    void* ptr = reinterpret_cast<void*>(uintptr);
    CheckHex(ptr, "%llx", "%0*llx", "%*llx");
  }
}

void CheckHexDec32(uint32_t uv) {
  CheckHex(uv, "%x", "%0*x", "%*x");
  CheckDec(uv, "%u", "%0*u", "%*u");
  int32_t v = static_cast<int32_t>(uv);
  CheckDec(v, "%d", "%0*d", "%*d");

  if (sizeof(v) == sizeof(&v)) {
    auto uintptr = static_cast<uintptr_t>(v);
    void* ptr = reinterpret_cast<void*>(uintptr);
    CheckHex(ptr, "%x", "%0*x", "%*x");
  }
}

void CheckAll(uint64_t v) {
  CheckHexDec64(v);
  CheckHexDec32(static_cast<uint32_t>(v));
}

void TestFastPrints() {
  // Test all small ints; there aren't many and they're common.
  for (int i = 0; i < 10000; i++) {
    CheckAll(i);
  }

  CheckAll(std::numeric_limits<uint64_t>::max());
  CheckAll(std::numeric_limits<uint64_t>::max() - 1);
  CheckAll(std::numeric_limits<int64_t>::min());
  CheckAll(std::numeric_limits<int64_t>::min() + 1);
  CheckAll(std::numeric_limits<uint32_t>::max());
  CheckAll(std::numeric_limits<uint32_t>::max() - 1);
  CheckAll(std::numeric_limits<int32_t>::min());
  CheckAll(std::numeric_limits<int32_t>::min() + 1);
  CheckAll(999999999);              // fits in 32 bits
  CheckAll(1000000000);             // fits in 32 bits
  CheckAll(9999999999);             // doesn't fit in 32 bits
  CheckAll(10000000000);            // doesn't fit in 32 bits
  CheckAll(999999999999999999);     // fits in signed 64-bit
  CheckAll(9999999999999999999u);   // fits in unsigned 64-bit, but not signed.
  CheckAll(1000000000000000000);    // fits in signed 64-bit
  CheckAll(10000000000000000000u);  // fits in unsigned 64-bit, but not signed.

  CheckAll(999999999876543210);    // check all decimal digits, signed
  CheckAll(9999999999876543210u);  // check all decimal digits, unsigned.
  CheckAll(0x123456789abcdef0);    // check all hex digits
  CheckAll(0x12345678);

  int8_t minus_one_8bit = -1;
  EXPECT_EQ("ff", turbo::str_cat(turbo::Hex(minus_one_8bit)));

  int16_t minus_one_16bit = -1;
  EXPECT_EQ("ffff", turbo::str_cat(turbo::Hex(minus_one_16bit)));
}

TEST(Numbers, TestFunctionsMovedOverFromNumbersMain) {
  TestFastPrints();
}

struct PointStringify {
  template <typename FormatSink>
  friend void turbo_stringify(FormatSink& sink, const PointStringify& p) {
    sink.Append("(");
    sink.Append(turbo::str_cat(p.x));
    sink.Append(", ");
    sink.Append(turbo::str_cat(p.y));
    sink.Append(")");
  }

  double x = 10.0;
  double y = 20.0;
};

TEST(str_cat, TurboStringifyExample) {
  PointStringify p;
  EXPECT_EQ(turbo::str_cat(p), "(10, 20)");
  EXPECT_EQ(turbo::str_cat("a ", p, " z"), "a (10, 20) z");
}

struct PointStringifyUsingFormat {
  template <typename FormatSink>
  friend void turbo_stringify(FormatSink& sink,
                            const PointStringifyUsingFormat& p) {
    turbo::format(&sink, "(%g, %g)", p.x, p.y);
  }

  double x = 10.0;
  double y = 20.0;
};

TEST(str_cat, TurboStringifyExampleUsingFormat) {
  PointStringifyUsingFormat p;
  EXPECT_EQ(turbo::str_cat(p), "(10, 20)");
  EXPECT_EQ(turbo::str_cat("a ", p, " z"), "a (10, 20) z");
}

enum class EnumWithStringify { Many = 0, Choices = 1 };

template <typename Sink>
void turbo_stringify(Sink& sink, EnumWithStringify e) {
  turbo::format(&sink, "%s", e == EnumWithStringify::Many ? "Many" : "Choices");
}

TEST(str_cat, TurboStringifyWithEnum) {
  const auto e = EnumWithStringify::Choices;
  EXPECT_EQ(turbo::str_cat(e), "Choices");
}

template <typename Integer>
void CheckSingleArgumentIntegerLimits() {
  Integer max = std::numeric_limits<Integer>::max();
  Integer min = std::numeric_limits<Integer>::min();

  EXPECT_EQ(turbo::str_cat(max), std::to_string(max));
  EXPECT_EQ(turbo::str_cat(min), std::to_string(min));
}

TEST(str_cat, SingleArgumentLimits) {
  CheckSingleArgumentIntegerLimits<int32_t>();
  CheckSingleArgumentIntegerLimits<uint32_t>();
  CheckSingleArgumentIntegerLimits<int64_t>();
  CheckSingleArgumentIntegerLimits<uint64_t>();
}

}  // namespace
