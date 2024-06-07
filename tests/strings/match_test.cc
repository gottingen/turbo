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

#include <turbo/strings/match.h>

#include <string>

#include <gtest/gtest.h>
#include <turbo/strings/string_view.h>

namespace {

TEST(MatchTest, starts_with) {
  const std::string s1("123\0abc", 7);
  const std::string_view a("foobar");
  const std::string_view b(s1);
  const std::string_view e;
  EXPECT_TRUE(turbo::starts_with(a, a));
  EXPECT_TRUE(turbo::starts_with(a, "foo"));
  EXPECT_TRUE(turbo::starts_with(a, e));
  EXPECT_TRUE(turbo::starts_with(b, s1));
  EXPECT_TRUE(turbo::starts_with(b, b));
  EXPECT_TRUE(turbo::starts_with(b, e));
  EXPECT_TRUE(turbo::starts_with(e, ""));
  EXPECT_FALSE(turbo::starts_with(a, b));
  EXPECT_FALSE(turbo::starts_with(b, a));
  EXPECT_FALSE(turbo::starts_with(e, a));
}

TEST(MatchTest, ends_with) {
  const std::string s1("123\0abc", 7);
  const std::string_view a("foobar");
  const std::string_view b(s1);
  const std::string_view e;
  EXPECT_TRUE(turbo::ends_with(a, a));
  EXPECT_TRUE(turbo::ends_with(a, "bar"));
  EXPECT_TRUE(turbo::ends_with(a, e));
  EXPECT_TRUE(turbo::ends_with(b, s1));
  EXPECT_TRUE(turbo::ends_with(b, b));
  EXPECT_TRUE(turbo::ends_with(b, e));
  EXPECT_TRUE(turbo::ends_with(e, ""));
  EXPECT_FALSE(turbo::ends_with(a, b));
  EXPECT_FALSE(turbo::ends_with(b, a));
  EXPECT_FALSE(turbo::ends_with(e, a));
}

TEST(MatchTest, Contains) {
  std::string_view a("abcdefg");
  std::string_view b("abcd");
  std::string_view c("efg");
  std::string_view d("gh");
  EXPECT_TRUE(turbo::str_contains(a, a));
  EXPECT_TRUE(turbo::str_contains(a, b));
  EXPECT_TRUE(turbo::str_contains(a, c));
  EXPECT_FALSE(turbo::str_contains(a, d));
  EXPECT_TRUE(turbo::str_contains("", ""));
  EXPECT_TRUE(turbo::str_contains("abc", ""));
  EXPECT_FALSE(turbo::str_contains("", "a"));
}

TEST(MatchTest, ContainsChar) {
  std::string_view a("abcdefg");
  std::string_view b("abcd");
  EXPECT_TRUE(turbo::str_contains(a, 'a'));
  EXPECT_TRUE(turbo::str_contains(a, 'b'));
  EXPECT_TRUE(turbo::str_contains(a, 'e'));
  EXPECT_FALSE(turbo::str_contains(a, 'h'));

  EXPECT_TRUE(turbo::str_contains(b, 'a'));
  EXPECT_TRUE(turbo::str_contains(b, 'b'));
  EXPECT_FALSE(turbo::str_contains(b, 'e'));
  EXPECT_FALSE(turbo::str_contains(b, 'h'));

  EXPECT_FALSE(turbo::str_contains("", 'a'));
  EXPECT_FALSE(turbo::str_contains("", 'a'));
}

TEST(MatchTest, ContainsNull) {
  const std::string s = "foo";
  const char* cs = "foo";
  const std::string_view sv("foo");
  const std::string_view sv2("foo\0bar", 4);
  EXPECT_EQ(s, "foo");
  EXPECT_EQ(sv, "foo");
  EXPECT_NE(sv2, "foo");
  EXPECT_TRUE(turbo::ends_with(s, sv));
  EXPECT_TRUE(turbo::starts_with(cs, sv));
  EXPECT_TRUE(turbo::str_contains(cs, sv));
  EXPECT_FALSE(turbo::str_contains(cs, sv2));
}

TEST(MatchTest, equals_ignore_case) {
  std::string text = "the";
  std::string_view data(text);

  EXPECT_TRUE(turbo::equals_ignore_case(data, "The"));
  EXPECT_TRUE(turbo::equals_ignore_case(data, "THE"));
  EXPECT_TRUE(turbo::equals_ignore_case(data, "the"));
  EXPECT_FALSE(turbo::equals_ignore_case(data, "Quick"));
  EXPECT_FALSE(turbo::equals_ignore_case(data, "then"));
}

TEST(MatchTest, starts_with_ignore_case) {
  EXPECT_TRUE(turbo::starts_with_ignore_case("foo", "foo"));
  EXPECT_TRUE(turbo::starts_with_ignore_case("foo", "Fo"));
  EXPECT_TRUE(turbo::starts_with_ignore_case("foo", ""));
  EXPECT_FALSE(turbo::starts_with_ignore_case("foo", "fooo"));
  EXPECT_FALSE(turbo::starts_with_ignore_case("", "fo"));
}

TEST(MatchTest, ends_with_ignore_case) {
  EXPECT_TRUE(turbo::ends_with_ignore_case("foo", "foo"));
  EXPECT_TRUE(turbo::ends_with_ignore_case("foo", "Oo"));
  EXPECT_TRUE(turbo::ends_with_ignore_case("foo", ""));
  EXPECT_FALSE(turbo::ends_with_ignore_case("foo", "fooo"));
  EXPECT_FALSE(turbo::ends_with_ignore_case("", "fo"));
}

TEST(MatchTest, ContainsIgnoreCase) {
  EXPECT_TRUE(turbo::str_contains_ignore_case("foo", "foo"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("FOO", "Foo"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("--FOO", "Foo"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("FOO--", "Foo"));
  EXPECT_FALSE(turbo::str_contains_ignore_case("BAR", "Foo"));
  EXPECT_FALSE(turbo::str_contains_ignore_case("BAR", "Foo"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("123456", "123456"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("123456", "234"));
  EXPECT_TRUE(turbo::str_contains_ignore_case("", ""));
  EXPECT_TRUE(turbo::str_contains_ignore_case("abc", ""));
  EXPECT_FALSE(turbo::str_contains_ignore_case("", "a"));
}

TEST(MatchTest, ContainsCharIgnoreCase) {
  std::string_view a("AaBCdefg!");
  std::string_view b("AaBCd!");
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'a'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'A'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'b'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'B'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'e'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, 'E'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(a, 'h'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(a, 'H'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(a, '!'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(a, '?'));

  EXPECT_TRUE(turbo::str_contains_ignore_case(b, 'a'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(b, 'A'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(b, 'b'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(b, 'B'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(b, 'e'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(b, 'E'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(b, 'h'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(b, 'H'));
  EXPECT_TRUE(turbo::str_contains_ignore_case(b, '!'));
  EXPECT_FALSE(turbo::str_contains_ignore_case(b, '?'));

  EXPECT_FALSE(turbo::str_contains_ignore_case("", 'a'));
  EXPECT_FALSE(turbo::str_contains_ignore_case("", 'A'));
  EXPECT_FALSE(turbo::str_contains_ignore_case("", '0'));
}

TEST(MatchTest, find_longest_common_prefix) {
  EXPECT_EQ(turbo::find_longest_common_prefix("", ""), "");
  EXPECT_EQ(turbo::find_longest_common_prefix("", "abc"), "");
  EXPECT_EQ(turbo::find_longest_common_prefix("abc", ""), "");
  EXPECT_EQ(turbo::find_longest_common_prefix("ab", "abc"), "ab");
  EXPECT_EQ(turbo::find_longest_common_prefix("abc", "ab"), "ab");
  EXPECT_EQ(turbo::find_longest_common_prefix("abc", "abd"), "ab");
  EXPECT_EQ(turbo::find_longest_common_prefix("abc", "abcd"), "abc");
  EXPECT_EQ(turbo::find_longest_common_prefix("abcd", "abcd"), "abcd");
  EXPECT_EQ(turbo::find_longest_common_prefix("abcd", "efgh"), "");

  // "abcde" v. "abc" but in the middle of other data
  EXPECT_EQ(turbo::find_longest_common_prefix(
                std::string_view("1234 abcdef").substr(5, 5),
                std::string_view("5678 abcdef").substr(5, 3)),
            "abc");
}

// Since the little-endian implementation involves a bit of if-else and various
// return paths, the following tests aims to provide full test coverage of the
// implementation.
TEST(MatchTest, FindLongestCommonPrefixLoad16Mismatch) {
  const std::string x1 = "abcdefgh";
  const std::string x2 = "abcde_";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesNoLast) {
  const std::string x1 = "abcdef";
  const std::string x2 = "abcdef";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcdef");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcdef");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesLastCharMismatches) {
  const std::string x1 = "abcdefg";
  const std::string x2 = "abcdef_h";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcdef");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcdef");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesLastMatches) {
  const std::string x1 = "abcde";
  const std::string x2 = "abcdefgh";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixSize8Load64Mismatches) {
  const std::string x1 = "abcdefghijk";
  const std::string x2 = "abcde_g_";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixSize8Load64Matches) {
  const std::string x1 = "abcdefgh";
  const std::string x2 = "abcdefgh";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "abcdefgh");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "abcdefgh");
}

TEST(MatchTest, FindLongestCommonPrefixSize15Load64Mismatches) {
  const std::string x1 = "012345670123456";
  const std::string x2 = "0123456701_34_6";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "0123456701");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "0123456701");
}

TEST(MatchTest, FindLongestCommonPrefixSize15Load64Matches) {
  const std::string x1 = "012345670123456";
  const std::string x2 = "0123456701234567";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "012345670123456");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "012345670123456");
}

TEST(MatchTest, FindLongestCommonPrefixSizeFirstByteOfLast8BytesMismatch) {
  const std::string x1 = "012345670123456701234567";
  const std::string x2 = "0123456701234567_1234567";
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), "0123456701234567");
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), "0123456701234567");
}

TEST(MatchTest, FindLongestCommonPrefixLargeLastCharMismatches) {
  const std::string x1(300, 'x');
  std::string x2 = x1;
  x2.back() = '#';
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), std::string(299, 'x'));
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), std::string(299, 'x'));
}

TEST(MatchTest, FindLongestCommonPrefixLargeFullMatch) {
  const std::string x1(300, 'x');
  const std::string x2 = x1;
  EXPECT_EQ(turbo::find_longest_common_prefix(x1, x2), std::string(300, 'x'));
  EXPECT_EQ(turbo::find_longest_common_prefix(x2, x1), std::string(300, 'x'));
}

TEST(MatchTest, find_longest_common_suffix) {
  EXPECT_EQ(turbo::find_longest_common_suffix("", ""), "");
  EXPECT_EQ(turbo::find_longest_common_suffix("", "abc"), "");
  EXPECT_EQ(turbo::find_longest_common_suffix("abc", ""), "");
  EXPECT_EQ(turbo::find_longest_common_suffix("bc", "abc"), "bc");
  EXPECT_EQ(turbo::find_longest_common_suffix("abc", "bc"), "bc");
  EXPECT_EQ(turbo::find_longest_common_suffix("abc", "dbc"), "bc");
  EXPECT_EQ(turbo::find_longest_common_suffix("bcd", "abcd"), "bcd");
  EXPECT_EQ(turbo::find_longest_common_suffix("abcd", "abcd"), "abcd");
  EXPECT_EQ(turbo::find_longest_common_suffix("abcd", "efgh"), "");

  // "abcde" v. "cde" but in the middle of other data
  EXPECT_EQ(turbo::find_longest_common_suffix(
                std::string_view("1234 abcdef").substr(5, 5),
                std::string_view("5678 abcdef").substr(7, 3)),
            "cde");
}

}  // namespace
