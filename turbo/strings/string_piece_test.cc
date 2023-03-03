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

#include "turbo/strings/string_piece.h"

#include <stdlib.h>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "turbo/platform/port.h"
#include "turbo/platform/dynamic_annotations.h"
#include "turbo/platform/options.h"
#include "gtest/gtest.h"

#if defined(__ANDROID__)
// We don't control the death messaging when using std::string_piece.
// Android assert messages only go to system log, so death tests cannot inspect
// the message for matching.
#define TURBO_EXPECT_DEATH_IF_SUPPORTED(statement, regex) \
  EXPECT_DEATH_IF_SUPPORTED(statement, ".*")
#else
#define TURBO_EXPECT_DEATH_IF_SUPPORTED(statement, regex) \
  EXPECT_DEATH_IF_SUPPORTED(statement, regex)
#endif

namespace {

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

TEST(StringViewTest, Ctor) {
  {
    // Null.
    turbo::string_piece s10;
    EXPECT_TRUE(s10.data() == nullptr);
    EXPECT_EQ(0u, s10.length());
  }

  {
    // const char* without length.
    const char* hello = "hello";
    turbo::string_piece s20(hello);
    EXPECT_TRUE(s20.data() == hello);
    EXPECT_EQ(5u, s20.length());

    // const char* with length.
    turbo::string_piece s21(hello, 4);
    EXPECT_TRUE(s21.data() == hello);
    EXPECT_EQ(4u, s21.length());

    // Not recommended, but valid C++
    turbo::string_piece s22(hello, 6);
    EXPECT_TRUE(s22.data() == hello);
    EXPECT_EQ(6u, s22.length());
  }

  {
    // std::string.
    std::string hola = "hola";
    turbo::string_piece s30(hola);
    EXPECT_TRUE(s30.data() == hola.data());
    EXPECT_EQ(4u, s30.length());

    // std::string with embedded '\0'.
    hola.push_back('\0');
    hola.append("h2");
    hola.push_back('\0');
    turbo::string_piece s31(hola);
    EXPECT_TRUE(s31.data() == hola.data());
    EXPECT_EQ(8u, s31.length());
  }

  {
    using mstring =
        std::basic_string<char, std::char_traits<char>, Mallocator<char>>;
    mstring str1("BUNGIE-JUMPING!");
    const mstring str2("SLEEPING!");

    turbo::string_piece s1(str1);
    s1.remove_prefix(strlen("BUNGIE-JUM"));

    turbo::string_piece s2(str2);
    s2.remove_prefix(strlen("SLEE"));

    EXPECT_EQ(s1, s2);
    EXPECT_EQ(s1, "PING!");
  }

  // TODO(mec): turbo::string_piece(const turbo::string_piece&);
}

TEST(StringViewTest, Swap) {
  turbo::string_piece a("a");
  turbo::string_piece b("bbb");
  EXPECT_TRUE(noexcept(a.swap(b)));
  a.swap(b);
  EXPECT_EQ(a, "bbb");
  EXPECT_EQ(b, "a");
  a.swap(b);
  EXPECT_EQ(a, "a");
  EXPECT_EQ(b, "bbb");
}

TEST(StringViewTest, STLComparator) {
  std::string s1("foo");
  std::string s2("bar");
  std::string s3("baz");

  turbo::string_piece p1(s1);
  turbo::string_piece p2(s2);
  turbo::string_piece p3(s3);

  typedef std::map<turbo::string_piece, int> TestMap;
  TestMap map;

  map.insert(std::make_pair(p1, 0));
  map.insert(std::make_pair(p2, 1));
  map.insert(std::make_pair(p3, 2));
  EXPECT_EQ(map.size(), 3u);

  TestMap::const_iterator iter = map.begin();
  EXPECT_EQ(iter->second, 1);
  ++iter;
  EXPECT_EQ(iter->second, 2);
  ++iter;
  EXPECT_EQ(iter->second, 0);
  ++iter;
  EXPECT_TRUE(iter == map.end());

  TestMap::iterator new_iter = map.find("zot");
  EXPECT_TRUE(new_iter == map.end());

  new_iter = map.find("bar");
  EXPECT_TRUE(new_iter != map.end());

  map.erase(new_iter);
  EXPECT_EQ(map.size(), 2u);

  iter = map.begin();
  EXPECT_EQ(iter->second, 2);
  ++iter;
  EXPECT_EQ(iter->second, 0);
  ++iter;
  EXPECT_TRUE(iter == map.end());
}

#define COMPARE(result, op, x, y)                                      \
  EXPECT_EQ(result, turbo::string_piece((x)) op turbo::string_piece((y))); \
  EXPECT_EQ(result, turbo::string_piece((x)).compare(turbo::string_piece((y))) op 0)

TEST(StringViewTest, ComparisonOperators) {
  COMPARE(true, ==, "",   "");
  COMPARE(true, ==, "", turbo::string_piece());
  COMPARE(true, ==, turbo::string_piece(), "");
  COMPARE(true, ==, "a",  "a");
  COMPARE(true, ==, "aa", "aa");
  COMPARE(false, ==, "a",  "");
  COMPARE(false, ==, "",   "a");
  COMPARE(false, ==, "a",  "b");
  COMPARE(false, ==, "a",  "aa");
  COMPARE(false, ==, "aa", "a");

  COMPARE(false, !=, "",   "");
  COMPARE(false, !=, "a",  "a");
  COMPARE(false, !=, "aa", "aa");
  COMPARE(true, !=, "a",  "");
  COMPARE(true, !=, "",   "a");
  COMPARE(true, !=, "a",  "b");
  COMPARE(true, !=, "a",  "aa");
  COMPARE(true, !=, "aa", "a");

  COMPARE(true, <, "a",  "b");
  COMPARE(true, <, "a",  "aa");
  COMPARE(true, <, "aa", "b");
  COMPARE(true, <, "aa", "bb");
  COMPARE(false, <, "a",  "a");
  COMPARE(false, <, "b",  "a");
  COMPARE(false, <, "aa", "a");
  COMPARE(false, <, "b",  "aa");
  COMPARE(false, <, "bb", "aa");

  COMPARE(true, <=, "a",  "a");
  COMPARE(true, <=, "a",  "b");
  COMPARE(true, <=, "a",  "aa");
  COMPARE(true, <=, "aa", "b");
  COMPARE(true, <=, "aa", "bb");
  COMPARE(false, <=, "b",  "a");
  COMPARE(false, <=, "aa", "a");
  COMPARE(false, <=, "b",  "aa");
  COMPARE(false, <=, "bb", "aa");

  COMPARE(false, >=, "a",  "b");
  COMPARE(false, >=, "a",  "aa");
  COMPARE(false, >=, "aa", "b");
  COMPARE(false, >=, "aa", "bb");
  COMPARE(true, >=, "a",  "a");
  COMPARE(true, >=, "b",  "a");
  COMPARE(true, >=, "aa", "a");
  COMPARE(true, >=, "b",  "aa");
  COMPARE(true, >=, "bb", "aa");

  COMPARE(false, >, "a",  "a");
  COMPARE(false, >, "a",  "b");
  COMPARE(false, >, "a",  "aa");
  COMPARE(false, >, "aa", "b");
  COMPARE(false, >, "aa", "bb");
  COMPARE(true, >, "b",  "a");
  COMPARE(true, >, "aa", "a");
  COMPARE(true, >, "b",  "aa");
  COMPARE(true, >, "bb", "aa");
}

TEST(StringViewTest, ComparisonOperatorsByCharacterPosition) {
  std::string x;
  for (size_t i = 0; i < 256; i++) {
    x += 'a';
    std::string y = x;
    COMPARE(true, ==, x, y);
    for (size_t j = 0; j < i; j++) {
      std::string z = x;
      z[j] = 'b';       // Differs in position 'j'
      COMPARE(false, ==, x, z);
      COMPARE(true, <, x, z);
      COMPARE(true, >, z, x);
      if (j + 1 < i) {
        z[j + 1] = 'A';  // Differs in position 'j+1' as well
        COMPARE(false, ==, x, z);
        COMPARE(true, <, x, z);
        COMPARE(true, >, z, x);
        z[j + 1] = 'z';  // Differs in position 'j+1' as well
        COMPARE(false, ==, x, z);
        COMPARE(true, <, x, z);
        COMPARE(true, >, z, x);
      }
    }
  }
}
#undef COMPARE

// Sadly, our users often confuse std::string::npos with
// turbo::string_piece::npos; So much so that we test here that they are the same.
// They need to both be unsigned, and both be the maximum-valued integer of
// their type.

template <typename T>
struct is_type {
  template <typename U>
  static bool same(U) {
    return false;
  }
  static bool same(T) { return true; }
};

TEST(StringViewTest, NposMatchesStdStringView) {
  EXPECT_EQ(turbo::string_piece::npos, std::string::npos);

  EXPECT_TRUE(is_type<size_t>::same(turbo::string_piece::npos));
  EXPECT_FALSE(is_type<size_t>::same(""));

  // Make sure turbo::string_piece::npos continues to be a header constant.
  char test[turbo::string_piece::npos & 1] = {0};
  EXPECT_EQ(0, test[0]);
}

TEST(StringViewTest, STL1) {
  const turbo::string_piece a("abcdefghijklmnopqrstuvwxyz");
  const turbo::string_piece b("abc");
  const turbo::string_piece c("xyz");
  const turbo::string_piece d("foobar");
  const turbo::string_piece e;
  std::string temp("123");
  temp += '\0';
  temp += "456";
  const turbo::string_piece f(temp);

  EXPECT_EQ(a[6], 'g');
  EXPECT_EQ(b[0], 'a');
  EXPECT_EQ(c[2], 'z');
  EXPECT_EQ(f[3], '\0');
  EXPECT_EQ(f[5], '5');

  EXPECT_EQ(*d.data(), 'f');
  EXPECT_EQ(d.data()[5], 'r');
  EXPECT_TRUE(e.data() == nullptr);

  EXPECT_EQ(*a.begin(), 'a');
  EXPECT_EQ(*(b.begin() + 2), 'c');
  EXPECT_EQ(*(c.end() - 1), 'z');

  EXPECT_EQ(*a.rbegin(), 'z');
  EXPECT_EQ(*(b.rbegin() + 2), 'a');
  EXPECT_EQ(*(c.rend() - 1), 'x');
  EXPECT_TRUE(a.rbegin() + 26 == a.rend());

  EXPECT_EQ(a.size(), 26u);
  EXPECT_EQ(b.size(), 3u);
  EXPECT_EQ(c.size(), 3u);
  EXPECT_EQ(d.size(), 6u);
  EXPECT_EQ(e.size(), 0u);
  EXPECT_EQ(f.size(), 7u);

  EXPECT_TRUE(!d.empty());
  EXPECT_TRUE(d.begin() != d.end());
  EXPECT_TRUE(d.begin() + 6 == d.end());

  EXPECT_TRUE(e.empty());
  EXPECT_TRUE(e.begin() == e.end());

  char buf[4] = { '%', '%', '%', '%' };
  EXPECT_EQ(a.copy(buf, 4), 4u);
  EXPECT_EQ(buf[0], a[0]);
  EXPECT_EQ(buf[1], a[1]);
  EXPECT_EQ(buf[2], a[2]);
  EXPECT_EQ(buf[3], a[3]);
  EXPECT_EQ(a.copy(buf, 3, 7), 3u);
  EXPECT_EQ(buf[0], a[7]);
  EXPECT_EQ(buf[1], a[8]);
  EXPECT_EQ(buf[2], a[9]);
  EXPECT_EQ(buf[3], a[3]);
  EXPECT_EQ(c.copy(buf, 99), 3u);
  EXPECT_EQ(buf[0], c[0]);
  EXPECT_EQ(buf[1], c[1]);
  EXPECT_EQ(buf[2], c[2]);
  EXPECT_EQ(buf[3], a[3]);
#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW(a.copy(buf, 1, 27), std::out_of_range);
#else
  TURBO_EXPECT_DEATH_IF_SUPPORTED(a.copy(buf, 1, 27), "turbo::string_piece::copy");
#endif
}

// Separated from STL1() because some compilers produce an overly
// large stack frame for the combined function.
TEST(StringViewTest, STL2) {
  const turbo::string_piece a("abcdefghijklmnopqrstuvwxyz");
  const turbo::string_piece b("abc");
  const turbo::string_piece c("xyz");
  turbo::string_piece d("foobar");
  const turbo::string_piece e;
  const turbo::string_piece f(
      "123"
      "\0"
      "456",
      7);

  d = turbo::string_piece();
  EXPECT_EQ(d.size(), 0u);
  EXPECT_TRUE(d.empty());
  EXPECT_TRUE(d.data() == nullptr);
  EXPECT_TRUE(d.begin() == d.end());

  EXPECT_EQ(a.find(b), 0u);
  EXPECT_EQ(a.find(b, 1), turbo::string_piece::npos);
  EXPECT_EQ(a.find(c), 23u);
  EXPECT_EQ(a.find(c, 9), 23u);
  EXPECT_EQ(a.find(c, turbo::string_piece::npos), turbo::string_piece::npos);
  EXPECT_EQ(b.find(c), turbo::string_piece::npos);
  EXPECT_EQ(b.find(c, turbo::string_piece::npos), turbo::string_piece::npos);
  EXPECT_EQ(a.find(d), 0u);
  EXPECT_EQ(a.find(e), 0u);
  EXPECT_EQ(a.find(d, 12), 12u);
  EXPECT_EQ(a.find(e, 17), 17u);
  turbo::string_piece g("xx not found bb");
  EXPECT_EQ(a.find(g), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(d.find(b), turbo::string_piece::npos);
  EXPECT_EQ(e.find(b), turbo::string_piece::npos);
  EXPECT_EQ(d.find(b, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find(b, 7), turbo::string_piece::npos);

  size_t empty_search_pos = std::string().find(std::string());
  EXPECT_EQ(d.find(d), empty_search_pos);
  EXPECT_EQ(d.find(e), empty_search_pos);
  EXPECT_EQ(e.find(d), empty_search_pos);
  EXPECT_EQ(e.find(e), empty_search_pos);
  EXPECT_EQ(d.find(d, 4), std::string().find(std::string(), 4));
  EXPECT_EQ(d.find(e, 4), std::string().find(std::string(), 4));
  EXPECT_EQ(e.find(d, 4), std::string().find(std::string(), 4));
  EXPECT_EQ(e.find(e, 4), std::string().find(std::string(), 4));

  EXPECT_EQ(a.find('a'), 0u);
  EXPECT_EQ(a.find('c'), 2u);
  EXPECT_EQ(a.find('z'), 25u);
  EXPECT_EQ(a.find('$'), turbo::string_piece::npos);
  EXPECT_EQ(a.find('\0'), turbo::string_piece::npos);
  EXPECT_EQ(f.find('\0'), 3u);
  EXPECT_EQ(f.find('3'), 2u);
  EXPECT_EQ(f.find('5'), 5u);
  EXPECT_EQ(g.find('o'), 4u);
  EXPECT_EQ(g.find('o', 4), 4u);
  EXPECT_EQ(g.find('o', 5), 8u);
  EXPECT_EQ(a.find('b', 5), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(d.find('\0'), turbo::string_piece::npos);
  EXPECT_EQ(e.find('\0'), turbo::string_piece::npos);
  EXPECT_EQ(d.find('\0', 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find('\0', 7), turbo::string_piece::npos);
  EXPECT_EQ(d.find('x'), turbo::string_piece::npos);
  EXPECT_EQ(e.find('x'), turbo::string_piece::npos);
  EXPECT_EQ(d.find('x', 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find('x', 7), turbo::string_piece::npos);

  EXPECT_EQ(a.find(b.data(), 1, 0), 1u);
  EXPECT_EQ(a.find(c.data(), 9, 0), 9u);
  EXPECT_EQ(a.find(c.data(), turbo::string_piece::npos, 0),
            turbo::string_piece::npos);
  EXPECT_EQ(b.find(c.data(), turbo::string_piece::npos, 0),
            turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(d.find(b.data(), 4, 0), turbo::string_piece::npos);
  EXPECT_EQ(e.find(b.data(), 7, 0), turbo::string_piece::npos);

  EXPECT_EQ(a.find(b.data(), 1), turbo::string_piece::npos);
  EXPECT_EQ(a.find(c.data(), 9), 23u);
  EXPECT_EQ(a.find(c.data(), turbo::string_piece::npos), turbo::string_piece::npos);
  EXPECT_EQ(b.find(c.data(), turbo::string_piece::npos), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(d.find(b.data(), 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find(b.data(), 7), turbo::string_piece::npos);

  EXPECT_EQ(a.rfind(b), 0u);
  EXPECT_EQ(a.rfind(b, 1), 0u);
  EXPECT_EQ(a.rfind(c), 23u);
  EXPECT_EQ(a.rfind(c, 22), turbo::string_piece::npos);
  EXPECT_EQ(a.rfind(c, 1), turbo::string_piece::npos);
  EXPECT_EQ(a.rfind(c, 0), turbo::string_piece::npos);
  EXPECT_EQ(b.rfind(c), turbo::string_piece::npos);
  EXPECT_EQ(b.rfind(c, 0), turbo::string_piece::npos);
  EXPECT_EQ(a.rfind(d), std::string(a).rfind(std::string()));
  EXPECT_EQ(a.rfind(e), std::string(a).rfind(std::string()));
  EXPECT_EQ(a.rfind(d, 12), 12u);
  EXPECT_EQ(a.rfind(e, 17), 17u);
  EXPECT_EQ(a.rfind(g), turbo::string_piece::npos);
  EXPECT_EQ(d.rfind(b), turbo::string_piece::npos);
  EXPECT_EQ(e.rfind(b), turbo::string_piece::npos);
  EXPECT_EQ(d.rfind(b, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.rfind(b, 7), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(d.rfind(d, 4), std::string().rfind(std::string()));
  EXPECT_EQ(e.rfind(d, 7), std::string().rfind(std::string()));
  EXPECT_EQ(d.rfind(e, 4), std::string().rfind(std::string()));
  EXPECT_EQ(e.rfind(e, 7), std::string().rfind(std::string()));
  EXPECT_EQ(d.rfind(d), std::string().rfind(std::string()));
  EXPECT_EQ(e.rfind(d), std::string().rfind(std::string()));
  EXPECT_EQ(d.rfind(e), std::string().rfind(std::string()));
  EXPECT_EQ(e.rfind(e), std::string().rfind(std::string()));

  EXPECT_EQ(g.rfind('o'), 8u);
  EXPECT_EQ(g.rfind('q'), turbo::string_piece::npos);
  EXPECT_EQ(g.rfind('o', 8), 8u);
  EXPECT_EQ(g.rfind('o', 7), 4u);
  EXPECT_EQ(g.rfind('o', 3), turbo::string_piece::npos);
  EXPECT_EQ(f.rfind('\0'), 3u);
  EXPECT_EQ(f.rfind('\0', 12), 3u);
  EXPECT_EQ(f.rfind('3'), 2u);
  EXPECT_EQ(f.rfind('5'), 5u);
  // empty string nonsense
  EXPECT_EQ(d.rfind('o'), turbo::string_piece::npos);
  EXPECT_EQ(e.rfind('o'), turbo::string_piece::npos);
  EXPECT_EQ(d.rfind('o', 4), turbo::string_piece::npos);
  EXPECT_EQ(e.rfind('o', 7), turbo::string_piece::npos);

  EXPECT_EQ(a.rfind(b.data(), 1, 0), 1u);
  EXPECT_EQ(a.rfind(c.data(), 22, 0), 22u);
  EXPECT_EQ(a.rfind(c.data(), 1, 0), 1u);
  EXPECT_EQ(a.rfind(c.data(), 0, 0), 0u);
  EXPECT_EQ(b.rfind(c.data(), 0, 0), 0u);
  EXPECT_EQ(d.rfind(b.data(), 4, 0), 0u);
  EXPECT_EQ(e.rfind(b.data(), 7, 0), 0u);
}

// Continued from STL2
TEST(StringViewTest, STL2FindFirst) {
  const turbo::string_piece a("abcdefghijklmnopqrstuvwxyz");
  const turbo::string_piece b("abc");
  const turbo::string_piece c("xyz");
  turbo::string_piece d("foobar");
  const turbo::string_piece e;
  const turbo::string_piece f(
      "123"
      "\0"
      "456",
      7);
  turbo::string_piece g("xx not found bb");

  d = turbo::string_piece();
  EXPECT_EQ(a.find_first_of(b), 0u);
  EXPECT_EQ(a.find_first_of(b, 0), 0u);
  EXPECT_EQ(a.find_first_of(b, 1), 1u);
  EXPECT_EQ(a.find_first_of(b, 2), 2u);
  EXPECT_EQ(a.find_first_of(b, 3), turbo::string_piece::npos);
  EXPECT_EQ(a.find_first_of(c), 23u);
  EXPECT_EQ(a.find_first_of(c, 23), 23u);
  EXPECT_EQ(a.find_first_of(c, 24), 24u);
  EXPECT_EQ(a.find_first_of(c, 25), 25u);
  EXPECT_EQ(a.find_first_of(c, 26), turbo::string_piece::npos);
  EXPECT_EQ(g.find_first_of(b), 13u);
  EXPECT_EQ(g.find_first_of(c), 0u);
  EXPECT_EQ(a.find_first_of(f), turbo::string_piece::npos);
  EXPECT_EQ(f.find_first_of(a), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(a.find_first_of(d), turbo::string_piece::npos);
  EXPECT_EQ(a.find_first_of(e), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_of(b), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_of(b), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_of(d), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_of(d), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_of(e), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_of(e), turbo::string_piece::npos);

  EXPECT_EQ(a.find_first_not_of(b), 3u);
  EXPECT_EQ(a.find_first_not_of(c), 0u);
  EXPECT_EQ(b.find_first_not_of(a), turbo::string_piece::npos);
  EXPECT_EQ(c.find_first_not_of(a), turbo::string_piece::npos);
  EXPECT_EQ(f.find_first_not_of(a), 0u);
  EXPECT_EQ(a.find_first_not_of(f), 0u);
  EXPECT_EQ(a.find_first_not_of(d), 0u);
  EXPECT_EQ(a.find_first_not_of(e), 0u);
  // empty string nonsense
  EXPECT_EQ(a.find_first_not_of(d), 0u);
  EXPECT_EQ(a.find_first_not_of(e), 0u);
  EXPECT_EQ(a.find_first_not_of(d, 1), 1u);
  EXPECT_EQ(a.find_first_not_of(e, 1), 1u);
  EXPECT_EQ(a.find_first_not_of(d, a.size() - 1), a.size() - 1);
  EXPECT_EQ(a.find_first_not_of(e, a.size() - 1), a.size() - 1);
  EXPECT_EQ(a.find_first_not_of(d, a.size()), turbo::string_piece::npos);
  EXPECT_EQ(a.find_first_not_of(e, a.size()), turbo::string_piece::npos);
  EXPECT_EQ(a.find_first_not_of(d, turbo::string_piece::npos),
            turbo::string_piece::npos);
  EXPECT_EQ(a.find_first_not_of(e, turbo::string_piece::npos),
            turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_not_of(a), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_not_of(a), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_not_of(d), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_not_of(d), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_not_of(e), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_not_of(e), turbo::string_piece::npos);

  turbo::string_piece h("====");
  EXPECT_EQ(h.find_first_not_of('='), turbo::string_piece::npos);
  EXPECT_EQ(h.find_first_not_of('=', 3), turbo::string_piece::npos);
  EXPECT_EQ(h.find_first_not_of('\0'), 0u);
  EXPECT_EQ(g.find_first_not_of('x'), 2u);
  EXPECT_EQ(f.find_first_not_of('\0'), 0u);
  EXPECT_EQ(f.find_first_not_of('\0', 3), 4u);
  EXPECT_EQ(f.find_first_not_of('\0', 2), 2u);
  // empty string nonsense
  EXPECT_EQ(d.find_first_not_of('x'), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_not_of('x'), turbo::string_piece::npos);
  EXPECT_EQ(d.find_first_not_of('\0'), turbo::string_piece::npos);
  EXPECT_EQ(e.find_first_not_of('\0'), turbo::string_piece::npos);
}

// Continued from STL2
TEST(StringViewTest, STL2FindLast) {
  const turbo::string_piece a("abcdefghijklmnopqrstuvwxyz");
  const turbo::string_piece b("abc");
  const turbo::string_piece c("xyz");
  turbo::string_piece d("foobar");
  const turbo::string_piece e;
  const turbo::string_piece f(
      "123"
      "\0"
      "456",
      7);
  turbo::string_piece g("xx not found bb");
  turbo::string_piece h("====");
  turbo::string_piece i("56");

  d = turbo::string_piece();
  EXPECT_EQ(h.find_last_of(a), turbo::string_piece::npos);
  EXPECT_EQ(g.find_last_of(a), g.size() - 1);
  EXPECT_EQ(a.find_last_of(b), 2u);
  EXPECT_EQ(a.find_last_of(c), a.size() - 1);
  EXPECT_EQ(f.find_last_of(i), 6u);
  EXPECT_EQ(a.find_last_of('a'), 0u);
  EXPECT_EQ(a.find_last_of('b'), 1u);
  EXPECT_EQ(a.find_last_of('z'), 25u);
  EXPECT_EQ(a.find_last_of('a', 5), 0u);
  EXPECT_EQ(a.find_last_of('b', 5), 1u);
  EXPECT_EQ(a.find_last_of('b', 0), turbo::string_piece::npos);
  EXPECT_EQ(a.find_last_of('z', 25), 25u);
  EXPECT_EQ(a.find_last_of('z', 24), turbo::string_piece::npos);
  EXPECT_EQ(f.find_last_of(i, 5), 5u);
  EXPECT_EQ(f.find_last_of(i, 6), 6u);
  EXPECT_EQ(f.find_last_of(a, 4), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(f.find_last_of(d), turbo::string_piece::npos);
  EXPECT_EQ(f.find_last_of(e), turbo::string_piece::npos);
  EXPECT_EQ(f.find_last_of(d, 4), turbo::string_piece::npos);
  EXPECT_EQ(f.find_last_of(e, 4), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(d), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(e), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(d), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(e), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(f), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(f), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(d, 4), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(e, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(d, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(e, 4), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_of(f, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_of(f, 4), turbo::string_piece::npos);

  EXPECT_EQ(a.find_last_not_of(b), a.size() - 1);
  EXPECT_EQ(a.find_last_not_of(c), 22u);
  EXPECT_EQ(b.find_last_not_of(a), turbo::string_piece::npos);
  EXPECT_EQ(b.find_last_not_of(b), turbo::string_piece::npos);
  EXPECT_EQ(f.find_last_not_of(i), 4u);
  EXPECT_EQ(a.find_last_not_of(c, 24), 22u);
  EXPECT_EQ(a.find_last_not_of(b, 3), 3u);
  EXPECT_EQ(a.find_last_not_of(b, 2), turbo::string_piece::npos);
  // empty string nonsense
  EXPECT_EQ(f.find_last_not_of(d), f.size() - 1);
  EXPECT_EQ(f.find_last_not_of(e), f.size() - 1);
  EXPECT_EQ(f.find_last_not_of(d, 4), 4u);
  EXPECT_EQ(f.find_last_not_of(e, 4), 4u);
  EXPECT_EQ(d.find_last_not_of(d), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of(e), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(d), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(e), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of(f), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(f), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of(d, 4), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of(e, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(d, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(e, 4), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of(f, 4), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of(f, 4), turbo::string_piece::npos);

  EXPECT_EQ(h.find_last_not_of('x'), h.size() - 1);
  EXPECT_EQ(h.find_last_not_of('='), turbo::string_piece::npos);
  EXPECT_EQ(b.find_last_not_of('c'), 1u);
  EXPECT_EQ(h.find_last_not_of('x', 2), 2u);
  EXPECT_EQ(h.find_last_not_of('=', 2), turbo::string_piece::npos);
  EXPECT_EQ(b.find_last_not_of('b', 1), 0u);
  // empty string nonsense
  EXPECT_EQ(d.find_last_not_of('x'), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of('x'), turbo::string_piece::npos);
  EXPECT_EQ(d.find_last_not_of('\0'), turbo::string_piece::npos);
  EXPECT_EQ(e.find_last_not_of('\0'), turbo::string_piece::npos);
}

// Continued from STL2
TEST(StringViewTest, STL2Substr) {
  const turbo::string_piece a("abcdefghijklmnopqrstuvwxyz");
  const turbo::string_piece b("abc");
  const turbo::string_piece c("xyz");
  turbo::string_piece d("foobar");
  const turbo::string_piece e;

  d = turbo::string_piece();
  EXPECT_EQ(a.substr(0, 3), b);
  EXPECT_EQ(a.substr(23), c);
  EXPECT_EQ(a.substr(23, 3), c);
  EXPECT_EQ(a.substr(23, 99), c);
  EXPECT_EQ(a.substr(0), a);
  EXPECT_EQ(a.substr(), a);
  EXPECT_EQ(a.substr(3, 2), "de");
  // empty string nonsense
  EXPECT_EQ(d.substr(0, 99), e);
  // use of npos
  EXPECT_EQ(a.substr(0, turbo::string_piece::npos), a);
  EXPECT_EQ(a.substr(23, turbo::string_piece::npos), c);
  // throw exception
#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW((void)a.substr(99, 2), std::out_of_range);
#else
  TURBO_EXPECT_DEATH_IF_SUPPORTED((void)a.substr(99, 2),
                                 "turbo::string_piece::substr");
#endif
}

TEST(StringViewTest, TruncSubstr) {
  const turbo::string_piece hi("hi");
  EXPECT_EQ("", turbo::ClippedSubstr(hi, 0, 0));
  EXPECT_EQ("h", turbo::ClippedSubstr(hi, 0, 1));
  EXPECT_EQ("hi", turbo::ClippedSubstr(hi, 0));
  EXPECT_EQ("i", turbo::ClippedSubstr(hi, 1));
  EXPECT_EQ("", turbo::ClippedSubstr(hi, 2));
  EXPECT_EQ("", turbo::ClippedSubstr(hi, 3));  // truncation
  EXPECT_EQ("", turbo::ClippedSubstr(hi, 3, 2));  // truncation
}

TEST(StringViewTest, UTF8) {
  std::string utf8 = "\u00E1";
  std::string utf8_twice = utf8 + " " + utf8;
  size_t utf8_len = strlen(utf8.data());
  EXPECT_EQ(utf8_len, turbo::string_piece(utf8_twice).find_first_of(" "));
  EXPECT_EQ(utf8_len, turbo::string_piece(utf8_twice).find_first_of(" \t"));
}

TEST(StringViewTest, FindConformance) {
  struct {
    std::string haystack;
    std::string needle;
  } specs[] = {
    {"", ""},
    {"", "a"},
    {"a", ""},
    {"a", "a"},
    {"a", "b"},
    {"aa", ""},
    {"aa", "a"},
    {"aa", "b"},
    {"ab", "a"},
    {"ab", "b"},
    {"abcd", ""},
    {"abcd", "a"},
    {"abcd", "d"},
    {"abcd", "ab"},
    {"abcd", "bc"},
    {"abcd", "cd"},
    {"abcd", "abcd"},
  };
  for (const auto& s : specs) {
    SCOPED_TRACE(s.haystack);
    SCOPED_TRACE(s.needle);
    std::string st = s.haystack;
    turbo::string_piece sp = s.haystack;
    for (size_t i = 0; i <= sp.size(); ++i) {
      size_t pos = (i == sp.size()) ? turbo::string_piece::npos : i;
      SCOPED_TRACE(pos);
      EXPECT_EQ(sp.find(s.needle, pos),
                st.find(s.needle, pos));
      EXPECT_EQ(sp.rfind(s.needle, pos),
                st.rfind(s.needle, pos));
      EXPECT_EQ(sp.find_first_of(s.needle, pos),
                st.find_first_of(s.needle, pos));
      EXPECT_EQ(sp.find_first_not_of(s.needle, pos),
                st.find_first_not_of(s.needle, pos));
      EXPECT_EQ(sp.find_last_of(s.needle, pos),
                st.find_last_of(s.needle, pos));
      EXPECT_EQ(sp.find_last_not_of(s.needle, pos),
                st.find_last_not_of(s.needle, pos));
    }
  }
}

TEST(StringViewTest, Remove) {
  turbo::string_piece a("foobar");
  std::string s1("123");
  s1 += '\0';
  s1 += "456";
  turbo::string_piece e;
  std::string s2;

  // remove_prefix
  turbo::string_piece c(a);
  c.remove_prefix(3);
  EXPECT_EQ(c, "bar");
  c = a;
  c.remove_prefix(0);
  EXPECT_EQ(c, a);
  c.remove_prefix(c.size());
  EXPECT_EQ(c, e);

  // remove_suffix
  c = a;
  c.remove_suffix(3);
  EXPECT_EQ(c, "foo");
  c = a;
  c.remove_suffix(0);
  EXPECT_EQ(c, a);
  c.remove_suffix(c.size());
  EXPECT_EQ(c, e);
}

TEST(StringViewTest, Set) {
  turbo::string_piece a("foobar");
  turbo::string_piece empty;
  turbo::string_piece b;

  // set
  b = turbo::string_piece("foobar", 6);
  EXPECT_EQ(b, a);
  b = turbo::string_piece("foobar", 0);
  EXPECT_EQ(b, empty);
  b = turbo::string_piece("foobar", 7);
  EXPECT_NE(b, a);

  b = turbo::string_piece("foobar");
  EXPECT_EQ(b, a);
}

TEST(StringViewTest, FrontBack) {
  static const char arr[] = "abcd";
  const turbo::string_piece csp(arr, 4);
  EXPECT_EQ(&arr[0], &csp.front());
  EXPECT_EQ(&arr[3], &csp.back());
}

TEST(StringViewTest, FrontBackSingleChar) {
  static const char c = 'a';
  const turbo::string_piece csp(&c, 1);
  EXPECT_EQ(&c, &csp.front());
  EXPECT_EQ(&c, &csp.back());
}

TEST(StringViewTest, FrontBackEmpty) {
#if !defined(NDEBUG) || TURBO_OPTION_HARDENED
  // Turbo's string_piece implementation has debug assertions that check that
  // front() and back() are not called on an empty string_piece.
  turbo::string_piece sv;
  TURBO_EXPECT_DEATH_IF_SUPPORTED(sv.front(), "");
  TURBO_EXPECT_DEATH_IF_SUPPORTED(sv.back(), "");
#endif
}

TEST(StringViewTest, NULLInput) {
  turbo::string_piece s;
  EXPECT_EQ(s.data(), nullptr);
  EXPECT_EQ(s.size(), 0u);
  s = turbo::string_piece(nullptr);
  EXPECT_EQ(s.data(), nullptr);
  EXPECT_EQ(s.size(), 0u);

  // .ToString() on a turbo::string_piece with nullptr should produce the empty
  // string.
  EXPECT_EQ("", std::string(s));
}

TEST(StringViewTest, Comparisons2) {
  // The `compare` member has 6 overloads (v: string_piece, s: const char*):
  //  (1) compare(v)
  //  (2) compare(pos1, count1, v)
  //  (3) compare(pos1, count1, v, pos2, count2)
  //  (4) compare(s)
  //  (5) compare(pos1, count1, s)
  //  (6) compare(pos1, count1, s, count2)

  turbo::string_piece abc("abcdefghijklmnopqrstuvwxyz");

  // check comparison operations on strings longer than 4 bytes.
  EXPECT_EQ(abc, turbo::string_piece("abcdefghijklmnopqrstuvwxyz"));
  EXPECT_EQ(abc.compare(turbo::string_piece("abcdefghijklmnopqrstuvwxyz")), 0);

  EXPECT_LT(abc, turbo::string_piece("abcdefghijklmnopqrstuvwxzz"));
  EXPECT_LT(abc.compare(turbo::string_piece("abcdefghijklmnopqrstuvwxzz")), 0);

  EXPECT_GT(abc, turbo::string_piece("abcdefghijklmnopqrstuvwxyy"));
  EXPECT_GT(abc.compare(turbo::string_piece("abcdefghijklmnopqrstuvwxyy")), 0);

  // The "substr" variants of `compare`.
  turbo::string_piece digits("0123456789");
  auto npos = turbo::string_piece::npos;

  // Taking string_piece
  EXPECT_EQ(digits.compare(3, npos, turbo::string_piece("3456789")), 0);  // 2
  EXPECT_EQ(digits.compare(3, 4, turbo::string_piece("3456")), 0);        // 2
  EXPECT_EQ(digits.compare(10, 0, turbo::string_piece()), 0);             // 2
  EXPECT_EQ(digits.compare(3, 4, turbo::string_piece("0123456789"), 3, 4),
            0);  // 3
  EXPECT_LT(digits.compare(3, 4, turbo::string_piece("0123456789"), 3, 5),
            0);  // 3
  EXPECT_LT(digits.compare(0, npos, turbo::string_piece("0123456789"), 3, 5),
            0);  // 3
  // Taking const char*
  EXPECT_EQ(digits.compare(3, 4, "3456"), 0);                 // 5
  EXPECT_EQ(digits.compare(3, npos, "3456789"), 0);           // 5
  EXPECT_EQ(digits.compare(10, 0, ""), 0);                    // 5
  EXPECT_EQ(digits.compare(3, 4, "0123456789", 3, 4), 0);     // 6
  EXPECT_LT(digits.compare(3, 4, "0123456789", 3, 5), 0);     // 6
  EXPECT_LT(digits.compare(0, npos, "0123456789", 3, 5), 0);  // 6
}

TEST(StringViewTest, At) {
  turbo::string_piece abc = "abc";
  EXPECT_EQ(abc.at(0), 'a');
  EXPECT_EQ(abc.at(1), 'b');
  EXPECT_EQ(abc.at(2), 'c');
#ifdef TURBO_HAVE_EXCEPTIONS
  EXPECT_THROW((void)abc.at(3), std::out_of_range);
#else
  TURBO_EXPECT_DEATH_IF_SUPPORTED((void)abc.at(3), "turbo::string_piece::at");
#endif
}

struct MyCharAlloc : std::allocator<char> {};

TEST(StringViewTest, ExplicitConversionOperator) {
  turbo::string_piece sp = "hi";
  EXPECT_EQ(sp, std::string(sp));
}

TEST(StringViewTest, NullSafeStringView) {
  {
    turbo::string_piece s = turbo::NullSafeStringView(nullptr);
    EXPECT_EQ(nullptr, s.data());
    EXPECT_EQ(0u, s.size());
    EXPECT_EQ(turbo::string_piece(), s);
  }
  {
    static const char kHi[] = "hi";
    turbo::string_piece s = turbo::NullSafeStringView(kHi);
    EXPECT_EQ(kHi, s.data());
    EXPECT_EQ(strlen(kHi), s.size());
    EXPECT_EQ(turbo::string_piece("hi"), s);
  }
}

TEST(StringViewTest, ConstexprNullSafeStringView) {
  {
    constexpr turbo::string_piece s = turbo::NullSafeStringView(nullptr);
    EXPECT_EQ(nullptr, s.data());
    EXPECT_EQ(0u, s.size());
    EXPECT_EQ(turbo::string_piece(), s);
  }
#if !defined(_MSC_VER) || _MSC_VER >= 1910
  // MSVC 2017+ is required for good constexpr string_piece support.
  // See the implementation of `turbo::string_piece::StrlenInternal()`.
  {
    static constexpr char kHi[] = "hi";
    turbo::string_piece s = turbo::NullSafeStringView(kHi);
    EXPECT_EQ(kHi, s.data());
    EXPECT_EQ(strlen(kHi), s.size());
    EXPECT_EQ(turbo::string_piece("hi"), s);
  }
  {
    constexpr turbo::string_piece s = turbo::NullSafeStringView("hello");
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ("hello", s);
  }
#endif
}

TEST(StringViewTest, ConstexprCompiles) {
  constexpr turbo::string_piece sp;
  constexpr turbo::string_piece cstr(nullptr);
  constexpr turbo::string_piece cstr_len("cstr", 4);


// This duplicates the check for __builtin_strlen in the header.
#if TURBO_HAVE_BUILTIN(__builtin_strlen) || \
    (defined(__GNUC__) && !defined(__clang__))
#define TURBO_HAVE_CONSTEXPR_STRING_VIEW_FROM_CSTR 1
#elif defined(__GNUC__)  // GCC or clang
#error GCC/clang should have constexpr string_piece.
#endif

// MSVC 2017+ should be able to construct a constexpr string_piece from a cstr.
#if defined(_MSC_VER) && _MSC_VER >= 1910
#define TURBO_HAVE_CONSTEXPR_STRING_VIEW_FROM_CSTR 1
#endif

#ifdef TURBO_HAVE_CONSTEXPR_STRING_VIEW_FROM_CSTR
  constexpr turbo::string_piece cstr_strlen("foo");
  EXPECT_EQ(cstr_strlen.length(), 3u);
  constexpr turbo::string_piece cstr_strlen2 = "bar";
  EXPECT_EQ(cstr_strlen2, "bar");

#if TURBO_HAVE_BUILTIN(__builtin_memcmp) || \
    (defined(__GNUC__) && !defined(__clang__))
#define TURBO_HAVE_CONSTEXPR_STRING_VIEW_COMPARISON 1
#endif
#ifdef TURBO_HAVE_CONSTEXPR_STRING_VIEW_COMPARISON
  constexpr turbo::string_piece foo = "foo";
  constexpr turbo::string_piece bar = "bar";
  constexpr bool foo_eq_bar = foo == bar;
  constexpr bool foo_ne_bar = foo != bar;
  constexpr bool foo_lt_bar = foo < bar;
  constexpr bool foo_le_bar = foo <= bar;
  constexpr bool foo_gt_bar = foo > bar;
  constexpr bool foo_ge_bar = foo >= bar;
  constexpr int foo_compare_bar = foo.compare(bar);
  EXPECT_FALSE(foo_eq_bar);
  EXPECT_TRUE(foo_ne_bar);
  EXPECT_FALSE(foo_lt_bar);
  EXPECT_FALSE(foo_le_bar);
  EXPECT_TRUE(foo_gt_bar);
  EXPECT_TRUE(foo_ge_bar);
  EXPECT_GT(foo_compare_bar, 0);
#endif
#endif

#if !defined(__clang__) || 3 < __clang_major__ || \
  (3 == __clang_major__ && 4 < __clang_minor__)
  // older clang versions (< 3.5) complain that:
  //   "cannot perform pointer arithmetic on null pointer"
  constexpr turbo::string_piece::iterator const_begin_empty = sp.begin();
  constexpr turbo::string_piece::iterator const_end_empty = sp.end();
  EXPECT_EQ(const_begin_empty, const_end_empty);

  constexpr turbo::string_piece::iterator const_begin_nullptr = cstr.begin();
  constexpr turbo::string_piece::iterator const_end_nullptr = cstr.end();
  EXPECT_EQ(const_begin_nullptr, const_end_nullptr);
#endif  // !defined(__clang__) || ...

  constexpr turbo::string_piece::iterator const_begin = cstr_len.begin();
  constexpr turbo::string_piece::iterator const_end = cstr_len.end();
  constexpr turbo::string_piece::size_type const_size = cstr_len.size();
  constexpr turbo::string_piece::size_type const_length = cstr_len.length();
  static_assert(const_begin + const_size == const_end,
                "pointer arithmetic check");
  static_assert(const_begin + const_length == const_end,
                "pointer arithmetic check");
#ifndef _MSC_VER
  // MSVC has bugs doing constexpr pointer arithmetic.
  // https://developercommunity.visualstudio.com/content/problem/482192/bad-pointer-arithmetic-in-constepxr-2019-rc1-svc1.html
  EXPECT_EQ(const_begin + const_size, const_end);
  EXPECT_EQ(const_begin + const_length, const_end);
#endif

  constexpr bool isempty = sp.empty();
  EXPECT_TRUE(isempty);

  constexpr const char c = cstr_len[2];
  EXPECT_EQ(c, 't');

  constexpr const char cfront = cstr_len.front();
  constexpr const char cback = cstr_len.back();
  EXPECT_EQ(cfront, 'c');
  EXPECT_EQ(cback, 'r');

  constexpr const char* np = sp.data();
  constexpr const char* cstr_ptr = cstr_len.data();
  EXPECT_EQ(np, nullptr);
  EXPECT_NE(cstr_ptr, nullptr);

  constexpr size_t sp_npos = sp.npos;
  EXPECT_EQ(sp_npos, static_cast<size_t>(-1));
}

constexpr char ConstexprMethodsHelper() {
#if defined(__cplusplus) && __cplusplus >= 201402L
  turbo::string_piece str("123", 3);
  str.remove_prefix(1);
  str.remove_suffix(1);
  turbo::string_piece bar;
  str.swap(bar);
  return bar.front();
#else
  return '2';
#endif
}

TEST(StringViewTest, ConstexprMethods) {
  // remove_prefix, remove_suffix, swap
  static_assert(ConstexprMethodsHelper() == '2', "");

  // substr
  constexpr turbo::string_piece foobar("foobar", 6);
  constexpr turbo::string_piece foo = foobar.substr(0, 3);
  constexpr turbo::string_piece bar = foobar.substr(3);
  EXPECT_EQ(foo, "foo");
  EXPECT_EQ(bar, "bar");
}

TEST(StringViewTest, Noexcept) {
  EXPECT_TRUE((std::is_nothrow_constructible<turbo::string_piece,
                                             const std::string&>::value));
  EXPECT_TRUE((std::is_nothrow_constructible<turbo::string_piece,
                                             const std::string&>::value));
  EXPECT_TRUE(std::is_nothrow_constructible<turbo::string_piece>::value);
  constexpr turbo::string_piece sp;
  EXPECT_TRUE(noexcept(sp.begin()));
  EXPECT_TRUE(noexcept(sp.end()));
  EXPECT_TRUE(noexcept(sp.cbegin()));
  EXPECT_TRUE(noexcept(sp.cend()));
  EXPECT_TRUE(noexcept(sp.rbegin()));
  EXPECT_TRUE(noexcept(sp.rend()));
  EXPECT_TRUE(noexcept(sp.crbegin()));
  EXPECT_TRUE(noexcept(sp.crend()));
  EXPECT_TRUE(noexcept(sp.size()));
  EXPECT_TRUE(noexcept(sp.length()));
  EXPECT_TRUE(noexcept(sp.empty()));
  EXPECT_TRUE(noexcept(sp.data()));
  EXPECT_TRUE(noexcept(sp.compare(sp)));
  EXPECT_TRUE(noexcept(sp.find(sp)));
  EXPECT_TRUE(noexcept(sp.find('f')));
  EXPECT_TRUE(noexcept(sp.rfind(sp)));
  EXPECT_TRUE(noexcept(sp.rfind('f')));
  EXPECT_TRUE(noexcept(sp.find_first_of(sp)));
  EXPECT_TRUE(noexcept(sp.find_first_of('f')));
  EXPECT_TRUE(noexcept(sp.find_last_of(sp)));
  EXPECT_TRUE(noexcept(sp.find_last_of('f')));
  EXPECT_TRUE(noexcept(sp.find_first_not_of(sp)));
  EXPECT_TRUE(noexcept(sp.find_first_not_of('f')));
  EXPECT_TRUE(noexcept(sp.find_last_not_of(sp)));
  EXPECT_TRUE(noexcept(sp.find_last_not_of('f')));
}

TEST(StringViewTest, BoundsCheck) {
#if !defined(NDEBUG) || TURBO_OPTION_HARDENED
  // Turbo's string_piece implementation has bounds-checking in debug mode.
  turbo::string_piece h = "hello";
  TURBO_EXPECT_DEATH_IF_SUPPORTED(h[5], "");
  TURBO_EXPECT_DEATH_IF_SUPPORTED(h[static_cast<size_t>(-1)], "");
#endif
}

TEST(ComparisonOpsTest, StringCompareNotAmbiguous) {
  EXPECT_EQ("hello", std::string("hello"));
  EXPECT_LT("hello", std::string("world"));
}

TEST(ComparisonOpsTest, HeterogeneousStringViewEquals) {
  EXPECT_EQ(turbo::string_piece("hello"), std::string("hello"));
  EXPECT_EQ("hello", turbo::string_piece("hello"));
}

TEST(FindOneCharTest, EdgeCases) {
  turbo::string_piece a("xxyyyxx");

  // Set a = "xyyyx".
  a.remove_prefix(1);
  a.remove_suffix(1);

  EXPECT_EQ(0u, a.find('x'));
  EXPECT_EQ(0u, a.find('x', 0));
  EXPECT_EQ(4u, a.find('x', 1));
  EXPECT_EQ(4u, a.find('x', 4));
  EXPECT_EQ(turbo::string_piece::npos, a.find('x', 5));

  EXPECT_EQ(4u, a.rfind('x'));
  EXPECT_EQ(4u, a.rfind('x', 5));
  EXPECT_EQ(4u, a.rfind('x', 4));
  EXPECT_EQ(0u, a.rfind('x', 3));
  EXPECT_EQ(0u, a.rfind('x', 0));

  // Set a = "yyy".
  a.remove_prefix(1);
  a.remove_suffix(1);

  EXPECT_EQ(turbo::string_piece::npos, a.find('x'));
  EXPECT_EQ(turbo::string_piece::npos, a.rfind('x'));
}

#ifndef TURBO_HAVE_THREAD_SANITIZER  // Allocates too much memory for tsan.
TEST(HugeStringView, TwoPointTwoGB) {
  if (sizeof(size_t) <= 4)
    return;
  // Try a huge string piece.
  const size_t size = size_t{2200} * 1000 * 1000;
  std::string s(size, 'a');
  turbo::string_piece sp(s);
  EXPECT_EQ(size, sp.length());
  sp.remove_prefix(1);
  EXPECT_EQ(size - 1, sp.length());
  sp.remove_suffix(2);
  EXPECT_EQ(size - 1 - 2, sp.length());
}
#endif  // TURBO_HAVE_THREAD_SANITIZER

#if !defined(NDEBUG)
TEST(NonNegativeLenTest, NonNegativeLen) {
  TURBO_EXPECT_DEATH_IF_SUPPORTED(
      turbo::string_piece("xyz", static_cast<size_t>(-1)), "len <= kMaxSize");
}

TEST(LenExceedsMaxSizeTest, LenExceedsMaxSize) {
  auto max_size = turbo::string_piece().max_size();

  // This should construct ok (although the view itself is obviously invalid).
  turbo::string_piece ok_view("", max_size);

  // Adding one to the max should trigger an assertion.
  TURBO_EXPECT_DEATH_IF_SUPPORTED(turbo::string_piece("", max_size + 1),
                                 "len <= kMaxSize");
}
#endif  // !defined(NDEBUG)

class StringViewStreamTest : public ::testing::Test {
 public:
  // Set negative 'width' for right justification.
  template <typename T>
  std::string Pad(const T& s, int width, char fill = 0) {
    std::ostringstream oss;
    if (fill != 0) {
      oss << std::setfill(fill);
    }
    if (width < 0) {
      width = -width;
      oss << std::right;
    }
    oss << std::setw(width) << s;
    return oss.str();
  }
};

TEST_F(StringViewStreamTest, Padding) {
  std::string s("hello");
  turbo::string_piece sp(s);
  for (int w = -64; w < 64; ++w) {
    SCOPED_TRACE(w);
    EXPECT_EQ(Pad(s, w), Pad(sp, w));
  }
  for (int w = -64; w < 64; ++w) {
    SCOPED_TRACE(w);
    EXPECT_EQ(Pad(s, w, '#'), Pad(sp, w, '#'));
  }
}

TEST_F(StringViewStreamTest, ResetsWidth) {
  // Width should reset after one formatted write.
  // If we weren't resetting width after formatting the string_piece,
  // we'd have width=5 carrying over to the printing of the "]",
  // creating "[###hi####]".
  std::string s = "hi";
  turbo::string_piece sp = s;
  {
    std::ostringstream oss;
    oss << "[" << std::setfill('#') << std::setw(5) << s << "]";
    ASSERT_EQ("[###hi]", oss.str());
  }
  {
    std::ostringstream oss;
    oss << "[" << std::setfill('#') << std::setw(5) << sp << "]";
    EXPECT_EQ("[###hi]", oss.str());
  }
}

}  // namespace
