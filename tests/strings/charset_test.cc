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

#include <turbo/strings/charset.h>

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/string_view.h>

namespace {

constexpr turbo::CharSet everything_map = ~turbo::CharSet();
constexpr turbo::CharSet nothing_map = turbo::CharSet();

TEST(Charmap, AllTests) {
  const turbo::CharSet also_nothing_map("");
  EXPECT_TRUE(everything_map.contains('\0'));
  EXPECT_FALSE(nothing_map.contains('\0'));
  EXPECT_FALSE(also_nothing_map.contains('\0'));
  for (unsigned char ch = 1; ch != 0; ++ch) {
    SCOPED_TRACE(ch);
    EXPECT_TRUE(everything_map.contains(ch));
    EXPECT_FALSE(nothing_map.contains(ch));
    EXPECT_FALSE(also_nothing_map.contains(ch));
  }

  const turbo::CharSet symbols(turbo::string_view("&@#@^!@?", 5));
  EXPECT_TRUE(symbols.contains('&'));
  EXPECT_TRUE(symbols.contains('@'));
  EXPECT_TRUE(symbols.contains('#'));
  EXPECT_TRUE(symbols.contains('^'));
  EXPECT_FALSE(symbols.contains('!'));
  EXPECT_FALSE(symbols.contains('?'));
  int cnt = 0;
  for (unsigned char ch = 1; ch != 0; ++ch) cnt += symbols.contains(ch);
  EXPECT_EQ(cnt, 4);

  const turbo::CharSet lets(turbo::string_view("^abcde", 3));
  const turbo::CharSet lets2(turbo::string_view("fghij\0klmnop", 10));
  const turbo::CharSet lets3("fghij\0klmnop");
  EXPECT_TRUE(lets2.contains('k'));
  EXPECT_FALSE(lets3.contains('k'));

  EXPECT_FALSE((symbols & lets).empty());
  EXPECT_TRUE((lets2 & lets).empty());
  EXPECT_FALSE((lets & symbols).empty());
  EXPECT_TRUE((lets & lets2).empty());

  EXPECT_TRUE(nothing_map.empty());
  EXPECT_FALSE(lets.empty());
}

std::string Members(const turbo::CharSet& m) {
  std::string r;
  for (size_t i = 0; i < 256; ++i)
    if (m.contains(i)) r.push_back(i);
  return r;
}

std::string ClosedRangeString(unsigned char lo, unsigned char hi) {
  // Don't depend on lo<hi. Just increment until lo==hi.
  std::string s;
  while (true) {
    s.push_back(lo);
    if (lo == hi) break;
    ++lo;
  }
  return s;
}

TEST(Charmap, Constexpr) {
  constexpr turbo::CharSet kEmpty = turbo::CharSet();
  EXPECT_EQ(Members(kEmpty), "");
  constexpr turbo::CharSet kA = turbo::CharSet::Char('A');
  EXPECT_EQ(Members(kA), "A");
  constexpr turbo::CharSet kAZ = turbo::CharSet::Range('A', 'Z');
  EXPECT_EQ(Members(kAZ), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  constexpr turbo::CharSet kIdentifier =
      turbo::CharSet::Range('0', '9') | turbo::CharSet::Range('A', 'Z') |
      turbo::CharSet::Range('a', 'z') | turbo::CharSet::Char('_');
  EXPECT_EQ(Members(kIdentifier),
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "_"
            "abcdefghijklmnopqrstuvwxyz");
  constexpr turbo::CharSet kAll = ~turbo::CharSet();
  for (size_t i = 0; i < 256; ++i) {
    SCOPED_TRACE(i);
    EXPECT_TRUE(kAll.contains(i));
  }
  constexpr turbo::CharSet kHello = turbo::CharSet("Hello, world!");
  EXPECT_EQ(Members(kHello), " !,Hdelorw");

  // test negation and intersection
  constexpr turbo::CharSet kABC =
      turbo::CharSet::Range('A', 'Z') & ~turbo::CharSet::Range('D', 'Z');
  EXPECT_EQ(Members(kABC), "ABC");

  // contains
  constexpr bool kContainsA = turbo::CharSet("abc").contains('a');
  EXPECT_TRUE(kContainsA);
  constexpr bool kContainsD = turbo::CharSet("abc").contains('d');
  EXPECT_FALSE(kContainsD);

  // empty
  constexpr bool kEmptyIsEmpty = turbo::CharSet().empty();
  EXPECT_TRUE(kEmptyIsEmpty);
  constexpr bool kNotEmptyIsEmpty = turbo::CharSet("abc").empty();
  EXPECT_FALSE(kNotEmptyIsEmpty);
}

TEST(Charmap, Range) {
  // Exhaustive testing takes too long, so test some of the boundaries that
  // are perhaps going to cause trouble.
  std::vector<size_t> poi = {0,   1,   2,   3,   4,   7,   8,   9,  15,
                             16,  17,  30,  31,  32,  33,  63,  64, 65,
                             127, 128, 129, 223, 224, 225, 254, 255};
  for (auto lo = poi.begin(); lo != poi.end(); ++lo) {
    SCOPED_TRACE(*lo);
    for (auto hi = lo; hi != poi.end(); ++hi) {
      SCOPED_TRACE(*hi);
      EXPECT_EQ(Members(turbo::CharSet::Range(*lo, *hi)),
                ClosedRangeString(*lo, *hi));
    }
  }
}

TEST(Charmap, NullByteWithStringView) {
  char characters[5] = {'a', 'b', '\0', 'd', 'x'};
  turbo::string_view view(characters, 5);
  turbo::CharSet tester(view);
  EXPECT_TRUE(tester.contains('a'));
  EXPECT_TRUE(tester.contains('b'));
  EXPECT_TRUE(tester.contains('\0'));
  EXPECT_TRUE(tester.contains('d'));
  EXPECT_TRUE(tester.contains('x'));
  EXPECT_FALSE(tester.contains('c'));
}

TEST(CharmapCtype, Match) {
  for (int c = 0; c < 256; ++c) {
    SCOPED_TRACE(c);
    SCOPED_TRACE(static_cast<char>(c));
    EXPECT_EQ(turbo::ascii_isupper(c),
              turbo::CharSet::AsciiUppercase().contains(c));
    EXPECT_EQ(turbo::ascii_islower(c),
              turbo::CharSet::AsciiLowercase().contains(c));
    EXPECT_EQ(turbo::ascii_isdigit(c), turbo::CharSet::AsciiDigits().contains(c));
    EXPECT_EQ(turbo::ascii_isalpha(c),
              turbo::CharSet::AsciiAlphabet().contains(c));
    EXPECT_EQ(turbo::ascii_isalnum(c),
              turbo::CharSet::AsciiAlphanumerics().contains(c));
    EXPECT_EQ(turbo::ascii_isxdigit(c),
              turbo::CharSet::AsciiHexDigits().contains(c));
    EXPECT_EQ(turbo::ascii_isprint(c),
              turbo::CharSet::AsciiPrintable().contains(c));
    EXPECT_EQ(turbo::ascii_isspace(c),
              turbo::CharSet::AsciiWhitespace().contains(c));
    EXPECT_EQ(turbo::ascii_ispunct(c),
              turbo::CharSet::AsciiPunctuation().contains(c));
  }
}

}  // namespace
