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

// This file contains functions that remove a defined part from the string,
// i.e., strip the string.

#include <turbo/strings/strip.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/string_view.h>

namespace {

TEST(Strip, ConsumePrefixOneChar) {
  turbo::string_view input("abc");
  EXPECT_TRUE(turbo::ConsumePrefix(&input, "a"));
  EXPECT_EQ(input, "bc");

  EXPECT_FALSE(turbo::ConsumePrefix(&input, "x"));
  EXPECT_EQ(input, "bc");

  EXPECT_TRUE(turbo::ConsumePrefix(&input, "b"));
  EXPECT_EQ(input, "c");

  EXPECT_TRUE(turbo::ConsumePrefix(&input, "c"));
  EXPECT_EQ(input, "");

  EXPECT_FALSE(turbo::ConsumePrefix(&input, "a"));
  EXPECT_EQ(input, "");
}

TEST(Strip, ConsumePrefix) {
  turbo::string_view input("abcdef");
  EXPECT_FALSE(turbo::ConsumePrefix(&input, "abcdefg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_FALSE(turbo::ConsumePrefix(&input, "abce"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(turbo::ConsumePrefix(&input, ""));
  EXPECT_EQ(input, "abcdef");

  EXPECT_FALSE(turbo::ConsumePrefix(&input, "abcdeg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(turbo::ConsumePrefix(&input, "abcdef"));
  EXPECT_EQ(input, "");

  input = "abcdef";
  EXPECT_TRUE(turbo::ConsumePrefix(&input, "abcde"));
  EXPECT_EQ(input, "f");
}

TEST(Strip, ConsumeSuffix) {
  turbo::string_view input("abcdef");
  EXPECT_FALSE(turbo::ConsumeSuffix(&input, "abcdefg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(turbo::ConsumeSuffix(&input, ""));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(turbo::ConsumeSuffix(&input, "def"));
  EXPECT_EQ(input, "abc");

  input = "abcdef";
  EXPECT_FALSE(turbo::ConsumeSuffix(&input, "abcdeg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(turbo::ConsumeSuffix(&input, "f"));
  EXPECT_EQ(input, "abcde");

  EXPECT_TRUE(turbo::ConsumeSuffix(&input, "abcde"));
  EXPECT_EQ(input, "");
}

TEST(Strip, StripPrefix) {
  const turbo::string_view null_str;

  EXPECT_EQ(turbo::StripPrefix("foobar", "foo"), "bar");
  EXPECT_EQ(turbo::StripPrefix("foobar", ""), "foobar");
  EXPECT_EQ(turbo::StripPrefix("foobar", null_str), "foobar");
  EXPECT_EQ(turbo::StripPrefix("foobar", "foobar"), "");
  EXPECT_EQ(turbo::StripPrefix("foobar", "bar"), "foobar");
  EXPECT_EQ(turbo::StripPrefix("foobar", "foobarr"), "foobar");
  EXPECT_EQ(turbo::StripPrefix("", ""), "");
}

TEST(Strip, StripSuffix) {
  const turbo::string_view null_str;

  EXPECT_EQ(turbo::StripSuffix("foobar", "bar"), "foo");
  EXPECT_EQ(turbo::StripSuffix("foobar", ""), "foobar");
  EXPECT_EQ(turbo::StripSuffix("foobar", null_str), "foobar");
  EXPECT_EQ(turbo::StripSuffix("foobar", "foobar"), "");
  EXPECT_EQ(turbo::StripSuffix("foobar", "foo"), "foobar");
  EXPECT_EQ(turbo::StripSuffix("foobar", "ffoobar"), "foobar");
  EXPECT_EQ(turbo::StripSuffix("", ""), "");
}

TEST(Strip, RemoveExtraAsciiWhitespace) {
  const char* inputs[] = {
      "No extra space",
      "  Leading whitespace",
      "Trailing whitespace  ",
      "  Leading and trailing  ",
      " Whitespace \t  in\v   middle  ",
      "'Eeeeep!  \n Newlines!\n",
      "nospaces",
  };
  const char* outputs[] = {
      "No extra space",
      "Leading whitespace",
      "Trailing whitespace",
      "Leading and trailing",
      "Whitespace in middle",
      "'Eeeeep! Newlines!",
      "nospaces",
  };
  int NUM_TESTS = 7;

  for (int i = 0; i < NUM_TESTS; i++) {
    std::string s(inputs[i]);
    turbo::RemoveExtraAsciiWhitespace(&s);
    EXPECT_STREQ(outputs[i], s.c_str());
  }

  // Test that turbo::RemoveExtraAsciiWhitespace returns immediately for empty
  // strings (It was adding the \0 character to the C++ std::string, which broke
  // tests involving empty())
  std::string zero_string = "";
  assert(zero_string.empty());
  turbo::RemoveExtraAsciiWhitespace(&zero_string);
  EXPECT_EQ(zero_string.size(), 0);
  EXPECT_TRUE(zero_string.empty());
}

TEST(Strip, StripTrailingAsciiWhitespace) {
  std::string test = "foo  ";
  turbo::StripTrailingAsciiWhitespace(&test);
  EXPECT_EQ(test, "foo");

  test = "   ";
  turbo::StripTrailingAsciiWhitespace(&test);
  EXPECT_EQ(test, "");

  test = "";
  turbo::StripTrailingAsciiWhitespace(&test);
  EXPECT_EQ(test, "");

  test = " abc\t";
  turbo::StripTrailingAsciiWhitespace(&test);
  EXPECT_EQ(test, " abc");
}

TEST(String, StripLeadingAsciiWhitespace) {
  turbo::string_view orig = "\t  \n\f\r\n\vfoo";
  EXPECT_EQ("foo", turbo::StripLeadingAsciiWhitespace(orig));
  orig = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  EXPECT_EQ(turbo::string_view(), turbo::StripLeadingAsciiWhitespace(orig));
}

TEST(Strip, StripAsciiWhitespace) {
  std::string test2 = "\t  \f\r\n\vfoo \t\f\r\v\n";
  turbo::StripAsciiWhitespace(&test2);
  EXPECT_EQ(test2, "foo");
  std::string test3 = "bar";
  turbo::StripAsciiWhitespace(&test3);
  EXPECT_EQ(test3, "bar");
  std::string test4 = "\t  \f\r\n\vfoo";
  turbo::StripAsciiWhitespace(&test4);
  EXPECT_EQ(test4, "foo");
  std::string test5 = "foo \t\f\r\v\n";
  turbo::StripAsciiWhitespace(&test5);
  EXPECT_EQ(test5, "foo");
  turbo::string_view test6("\t  \f\r\n\vfoo \t\f\r\v\n");
  test6 = turbo::StripAsciiWhitespace(test6);
  EXPECT_EQ(test6, "foo");
  test6 = turbo::StripAsciiWhitespace(test6);
  EXPECT_EQ(test6, "foo");  // already stripped
}

}  // namespace
