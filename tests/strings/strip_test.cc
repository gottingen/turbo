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

// This file contains functions that remove a defined part from the string,
// i.e., strip the string.

#include "turbo/strings/str_strip.h"
#include "turbo/strings/str_trim.h"
#include <cassert>
#include <cstdio>
#include <cstring>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "turbo/strings/string_view.h"

namespace {

TEST_CASE("Strip, ConsumePrefixOneChar") {
  std::string_view input("abc");
  CHECK(turbo::ConsumePrefix(&input, "a"));
  CHECK_EQ(input, "bc");

  CHECK_FALSE(turbo::ConsumePrefix(&input, "x"));
  CHECK_EQ(input, "bc");

  CHECK(turbo::ConsumePrefix(&input, "b"));
  CHECK_EQ(input, "c");

  CHECK(turbo::ConsumePrefix(&input, "c"));
  CHECK_EQ(input, "");

  CHECK_FALSE(turbo::ConsumePrefix(&input, "a"));
  CHECK_EQ(input, "");
}

TEST_CASE("Strip, ConsumePrefix") {
  std::string_view input("abcdef");
  CHECK_FALSE(turbo::ConsumePrefix(&input, "abcdefg"));
  CHECK_EQ(input, "abcdef");

  CHECK_FALSE(turbo::ConsumePrefix(&input, "abce"));
  CHECK_EQ(input, "abcdef");

  CHECK(turbo::ConsumePrefix(&input, ""));
  CHECK_EQ(input, "abcdef");

  CHECK_FALSE(turbo::ConsumePrefix(&input, "abcdeg"));
  CHECK_EQ(input, "abcdef");

  CHECK(turbo::ConsumePrefix(&input, "abcdef"));
  CHECK_EQ(input, "");

  input = "abcdef";
  CHECK(turbo::ConsumePrefix(&input, "abcde"));
  CHECK_EQ(input, "f");
}

TEST_CASE("Strip, ConsumeSuffix") {
  std::string_view input("abcdef");
  CHECK_FALSE(turbo::ConsumeSuffix(&input, "abcdefg"));
  CHECK_EQ(input, "abcdef");

  CHECK(turbo::ConsumeSuffix(&input, ""));
  CHECK_EQ(input, "abcdef");

  CHECK(turbo::ConsumeSuffix(&input, "def"));
  CHECK_EQ(input, "abc");

  input = "abcdef";
  CHECK_FALSE(turbo::ConsumeSuffix(&input, "abcdeg"));
  CHECK_EQ(input, "abcdef");

  CHECK(turbo::ConsumeSuffix(&input, "f"));
  CHECK_EQ(input, "abcde");

  CHECK(turbo::ConsumeSuffix(&input, "abcde"));
  CHECK_EQ(input, "");
}

TEST_CASE("Strip, StripPrefix") {
  const std::string_view null_str;

  CHECK_EQ(turbo::StripPrefix("foobar", "foo"), "bar");
  CHECK_EQ(turbo::StripPrefix("foobar", ""), "foobar");
  CHECK_EQ(turbo::StripPrefix("foobar", null_str), "foobar");
  CHECK_EQ(turbo::StripPrefix("foobar", "foobar"), "");
  CHECK_EQ(turbo::StripPrefix("foobar", "bar"), "foobar");
  CHECK_EQ(turbo::StripPrefix("foobar", "foobarr"), "foobar");
  CHECK_EQ(turbo::StripPrefix("", ""), "");
}

TEST_CASE("Strip, StripSuffix") {
  const std::string_view null_str;

  CHECK_EQ(turbo::StripSuffix("foobar", "bar"), "foo");
  CHECK_EQ(turbo::StripSuffix("foobar", ""), "foobar");
  CHECK_EQ(turbo::StripSuffix("foobar", null_str), "foobar");
  CHECK_EQ(turbo::StripSuffix("foobar", "foobar"), "");
  CHECK_EQ(turbo::StripSuffix("foobar", "foo"), "foobar");
  CHECK_EQ(turbo::StripSuffix("foobar", "ffoobar"), "foobar");
  CHECK_EQ(turbo::StripSuffix("", ""), "");
}

TEST_CASE("Strip, TrimAll") {
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
    turbo::TrimAll(&s);
    CHECK_EQ(std::string_view(outputs[i]), s);
  }

  // Test that turbo::TrimAll returns immediately for empty
  // strings (It was adding the \0 character to the C++ std::string, which broke
  // tests involving empty())
  std::string zero_string = "";
  assert(zero_string.empty());
  turbo::TrimAll(&zero_string);
  CHECK_EQ(zero_string.size(), 0);
  CHECK(zero_string.empty());
}

TEST_CASE("Strip, TrimRight") {
  std::string test = "foo  ";
  turbo::TrimRight(&test);
  CHECK_EQ(test, "foo");

  test = "   ";
  turbo::TrimRight(&test);
  CHECK_EQ(test, "");

  test = "";
  turbo::TrimRight(&test);
  CHECK_EQ(test, "");

  test = " abc\t";
  turbo::TrimRight(&test);
  CHECK_EQ(test, " abc");
}

TEST_CASE("String, TrimLeft") {
  std::string_view orig = "\t  \n\f\r\n\vfoo";
  CHECK_EQ("foo", turbo::TrimLeft(orig));
  orig = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  CHECK_EQ(std::string_view(), turbo::TrimLeft(orig));
}

TEST_CASE("Strip, Trim") {
  std::string test2 = "\t  \f\r\n\vfoo \t\f\r\v\n";
  turbo::Trim(&test2);
  CHECK_EQ(test2, "foo");
  std::string test3 = "bar";
  turbo::Trim(&test3);
  CHECK_EQ(test3, "bar");
  std::string test4 = "\t  \f\r\n\vfoo";
  turbo::Trim(&test4);
  CHECK_EQ(test4, "foo");
  std::string test5 = "foo \t\f\r\v\n";
  turbo::Trim(&test5);
  CHECK_EQ(test5, "foo");
  std::string_view test6("\t  \f\r\n\vfoo \t\f\r\v\n");
  test6 = turbo::Trim(test6);
  CHECK_EQ(test6, "foo");
  test6 = turbo::Trim(test6);
  CHECK_EQ(test6, "foo");  // already stripped
}

}  // namespace
