// Copyright 2022 The Turbo Authors.
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

#include "turbo/strings/match.h"

#include "gtest/gtest.h"

namespace {

TEST(MatchTest, StartsWith) {
  const std::string s1("123\0abc", 7);
  const std::string_view a("foobar");
  const std::string_view b(s1);
  const std::string_view e;
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
  const std::string_view a("foobar");
  const std::string_view b(s1);
  const std::string_view e;
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
  std::string_view a("abcdefg");
  std::string_view b("abcd");
  std::string_view c("efg");
  std::string_view d("gh");
  EXPECT_TRUE(turbo::StrContains(a, a));
  EXPECT_TRUE(turbo::StrContains(a, b));
  EXPECT_TRUE(turbo::StrContains(a, c));
  EXPECT_FALSE(turbo::StrContains(a, d));
  EXPECT_TRUE(turbo::StrContains("", ""));
  EXPECT_TRUE(turbo::StrContains("abc", ""));
  EXPECT_FALSE(turbo::StrContains("", "a"));
}

TEST(MatchTest, ContainsChar) {
  std::string_view a("abcdefg");
  std::string_view b("abcd");
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
  const std::string_view sv("foo");
  const std::string_view sv2("foo\0bar", 4);
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
  std::string_view data(text);

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

}  // namespace
