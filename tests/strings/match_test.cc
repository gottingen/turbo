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

TEST(MatchTest, StartsWith) {
  const std::string s1("123\0abc", 7);
  const turbo::string_view a("foobar");
  const turbo::string_view b(s1);
  const turbo::string_view e;
  EXPECT_TRUE(turbo::StartsWith(a, a));
  EXPECT_TRUE(turbo::StartsWith(a, "foo"));
  EXPECT_TRUE(turbo::StartsWith(a, e));
  EXPECT_TRUE(turbo::StartsWith(b, s1));
  EXPECT_TRUE(turbo::StartsWith(b, b));
  EXPECT_TRUE(turbo::StartsWith(b, e));
  EXPECT_TRUE(turbo::StartsWith(e, ""));
  EXPECT_FALSE(turbo::StartsWith(a, b));
  EXPECT_FALSE(turbo::StartsWith(b, a));
  EXPECT_FALSE(turbo::StartsWith(e, a));
}

TEST(MatchTest, EndsWith) {
  const std::string s1("123\0abc", 7);
  const turbo::string_view a("foobar");
  const turbo::string_view b(s1);
  const turbo::string_view e;
  EXPECT_TRUE(turbo::EndsWith(a, a));
  EXPECT_TRUE(turbo::EndsWith(a, "bar"));
  EXPECT_TRUE(turbo::EndsWith(a, e));
  EXPECT_TRUE(turbo::EndsWith(b, s1));
  EXPECT_TRUE(turbo::EndsWith(b, b));
  EXPECT_TRUE(turbo::EndsWith(b, e));
  EXPECT_TRUE(turbo::EndsWith(e, ""));
  EXPECT_FALSE(turbo::EndsWith(a, b));
  EXPECT_FALSE(turbo::EndsWith(b, a));
  EXPECT_FALSE(turbo::EndsWith(e, a));
}

TEST(MatchTest, Contains) {
  turbo::string_view a("abcdefg");
  turbo::string_view b("abcd");
  turbo::string_view c("efg");
  turbo::string_view d("gh");
  EXPECT_TRUE(turbo::StrContains(a, a));
  EXPECT_TRUE(turbo::StrContains(a, b));
  EXPECT_TRUE(turbo::StrContains(a, c));
  EXPECT_FALSE(turbo::StrContains(a, d));
  EXPECT_TRUE(turbo::StrContains("", ""));
  EXPECT_TRUE(turbo::StrContains("abc", ""));
  EXPECT_FALSE(turbo::StrContains("", "a"));
}

TEST(MatchTest, ContainsChar) {
  turbo::string_view a("abcdefg");
  turbo::string_view b("abcd");
  EXPECT_TRUE(turbo::StrContains(a, 'a'));
  EXPECT_TRUE(turbo::StrContains(a, 'b'));
  EXPECT_TRUE(turbo::StrContains(a, 'e'));
  EXPECT_FALSE(turbo::StrContains(a, 'h'));

  EXPECT_TRUE(turbo::StrContains(b, 'a'));
  EXPECT_TRUE(turbo::StrContains(b, 'b'));
  EXPECT_FALSE(turbo::StrContains(b, 'e'));
  EXPECT_FALSE(turbo::StrContains(b, 'h'));

  EXPECT_FALSE(turbo::StrContains("", 'a'));
  EXPECT_FALSE(turbo::StrContains("", 'a'));
}

TEST(MatchTest, ContainsNull) {
  const std::string s = "foo";
  const char* cs = "foo";
  const turbo::string_view sv("foo");
  const turbo::string_view sv2("foo\0bar", 4);
  EXPECT_EQ(s, "foo");
  EXPECT_EQ(sv, "foo");
  EXPECT_NE(sv2, "foo");
  EXPECT_TRUE(turbo::EndsWith(s, sv));
  EXPECT_TRUE(turbo::StartsWith(cs, sv));
  EXPECT_TRUE(turbo::StrContains(cs, sv));
  EXPECT_FALSE(turbo::StrContains(cs, sv2));
}

TEST(MatchTest, EqualsIgnoreCase) {
  std::string text = "the";
  turbo::string_view data(text);

  EXPECT_TRUE(turbo::EqualsIgnoreCase(data, "The"));
  EXPECT_TRUE(turbo::EqualsIgnoreCase(data, "THE"));
  EXPECT_TRUE(turbo::EqualsIgnoreCase(data, "the"));
  EXPECT_FALSE(turbo::EqualsIgnoreCase(data, "Quick"));
  EXPECT_FALSE(turbo::EqualsIgnoreCase(data, "then"));
}

TEST(MatchTest, StartsWithIgnoreCase) {
  EXPECT_TRUE(turbo::StartsWithIgnoreCase("foo", "foo"));
  EXPECT_TRUE(turbo::StartsWithIgnoreCase("foo", "Fo"));
  EXPECT_TRUE(turbo::StartsWithIgnoreCase("foo", ""));
  EXPECT_FALSE(turbo::StartsWithIgnoreCase("foo", "fooo"));
  EXPECT_FALSE(turbo::StartsWithIgnoreCase("", "fo"));
}

TEST(MatchTest, EndsWithIgnoreCase) {
  EXPECT_TRUE(turbo::EndsWithIgnoreCase("foo", "foo"));
  EXPECT_TRUE(turbo::EndsWithIgnoreCase("foo", "Oo"));
  EXPECT_TRUE(turbo::EndsWithIgnoreCase("foo", ""));
  EXPECT_FALSE(turbo::EndsWithIgnoreCase("foo", "fooo"));
  EXPECT_FALSE(turbo::EndsWithIgnoreCase("", "fo"));
}

TEST(MatchTest, ContainsIgnoreCase) {
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("foo", "foo"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("FOO", "Foo"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("--FOO", "Foo"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("FOO--", "Foo"));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase("BAR", "Foo"));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase("BAR", "Foo"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("123456", "123456"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("123456", "234"));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("", ""));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase("abc", ""));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase("", "a"));
}

TEST(MatchTest, ContainsCharIgnoreCase) {
  turbo::string_view a("AaBCdefg!");
  turbo::string_view b("AaBCd!");
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'a'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'A'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'b'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'B'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'e'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, 'E'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(a, 'h'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(a, 'H'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(a, '!'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(a, '?'));

  EXPECT_TRUE(turbo::StrContainsIgnoreCase(b, 'a'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(b, 'A'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(b, 'b'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(b, 'B'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(b, 'e'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(b, 'E'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(b, 'h'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(b, 'H'));
  EXPECT_TRUE(turbo::StrContainsIgnoreCase(b, '!'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase(b, '?'));

  EXPECT_FALSE(turbo::StrContainsIgnoreCase("", 'a'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase("", 'A'));
  EXPECT_FALSE(turbo::StrContainsIgnoreCase("", '0'));
}

TEST(MatchTest, FindLongestCommonPrefix) {
  EXPECT_EQ(turbo::FindLongestCommonPrefix("", ""), "");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("", "abc"), "");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abc", ""), "");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("ab", "abc"), "ab");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abc", "ab"), "ab");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abc", "abd"), "ab");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abc", "abcd"), "abc");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abcd", "abcd"), "abcd");
  EXPECT_EQ(turbo::FindLongestCommonPrefix("abcd", "efgh"), "");

  // "abcde" v. "abc" but in the middle of other data
  EXPECT_EQ(turbo::FindLongestCommonPrefix(
                turbo::string_view("1234 abcdef").substr(5, 5),
                turbo::string_view("5678 abcdef").substr(5, 3)),
            "abc");
}

// Since the little-endian implementation involves a bit of if-else and various
// return paths, the following tests aims to provide full test coverage of the
// implementation.
TEST(MatchTest, FindLongestCommonPrefixLoad16Mismatch) {
  const std::string x1 = "abcdefgh";
  const std::string x2 = "abcde_";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesNoLast) {
  const std::string x1 = "abcdef";
  const std::string x2 = "abcdef";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcdef");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcdef");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesLastCharMismatches) {
  const std::string x1 = "abcdefg";
  const std::string x2 = "abcdef_h";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcdef");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcdef");
}

TEST(MatchTest, FindLongestCommonPrefixLoad16MatchesLastMatches) {
  const std::string x1 = "abcde";
  const std::string x2 = "abcdefgh";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixSize8Load64Mismatches) {
  const std::string x1 = "abcdefghijk";
  const std::string x2 = "abcde_g_";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcde");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcde");
}

TEST(MatchTest, FindLongestCommonPrefixSize8Load64Matches) {
  const std::string x1 = "abcdefgh";
  const std::string x2 = "abcdefgh";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "abcdefgh");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "abcdefgh");
}

TEST(MatchTest, FindLongestCommonPrefixSize15Load64Mismatches) {
  const std::string x1 = "012345670123456";
  const std::string x2 = "0123456701_34_6";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "0123456701");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "0123456701");
}

TEST(MatchTest, FindLongestCommonPrefixSize15Load64Matches) {
  const std::string x1 = "012345670123456";
  const std::string x2 = "0123456701234567";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "012345670123456");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "012345670123456");
}

TEST(MatchTest, FindLongestCommonPrefixSizeFirstByteOfLast8BytesMismatch) {
  const std::string x1 = "012345670123456701234567";
  const std::string x2 = "0123456701234567_1234567";
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), "0123456701234567");
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), "0123456701234567");
}

TEST(MatchTest, FindLongestCommonPrefixLargeLastCharMismatches) {
  const std::string x1(300, 'x');
  std::string x2 = x1;
  x2.back() = '#';
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), std::string(299, 'x'));
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), std::string(299, 'x'));
}

TEST(MatchTest, FindLongestCommonPrefixLargeFullMatch) {
  const std::string x1(300, 'x');
  const std::string x2 = x1;
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x1, x2), std::string(300, 'x'));
  EXPECT_EQ(turbo::FindLongestCommonPrefix(x2, x1), std::string(300, 'x'));
}

TEST(MatchTest, FindLongestCommonSuffix) {
  EXPECT_EQ(turbo::FindLongestCommonSuffix("", ""), "");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("", "abc"), "");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("abc", ""), "");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("bc", "abc"), "bc");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("abc", "bc"), "bc");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("abc", "dbc"), "bc");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("bcd", "abcd"), "bcd");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("abcd", "abcd"), "abcd");
  EXPECT_EQ(turbo::FindLongestCommonSuffix("abcd", "efgh"), "");

  // "abcde" v. "cde" but in the middle of other data
  EXPECT_EQ(turbo::FindLongestCommonSuffix(
                turbo::string_view("1234 abcdef").substr(5, 5),
                turbo::string_view("5678 abcdef").substr(7, 3)),
            "cde");
}

}  // namespace
