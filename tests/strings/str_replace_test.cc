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

#include <turbo/strings/str_replace.h>

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>

TEST(str_replace_all, OneReplacement) {
  std::string s;

  // Empty string.
  s = turbo::str_replace_all(s, {{"", ""}});
  EXPECT_EQ(s, "");
  s = turbo::str_replace_all(s, {{"x", ""}});
  EXPECT_EQ(s, "");
  s = turbo::str_replace_all(s, {{"", "y"}});
  EXPECT_EQ(s, "");
  s = turbo::str_replace_all(s, {{"x", "y"}});
  EXPECT_EQ(s, "");

  // Empty substring.
  s = turbo::str_replace_all("abc", {{"", ""}});
  EXPECT_EQ(s, "abc");
  s = turbo::str_replace_all("abc", {{"", "y"}});
  EXPECT_EQ(s, "abc");
  s = turbo::str_replace_all("abc", {{"x", ""}});
  EXPECT_EQ(s, "abc");

  // Substring not found.
  s = turbo::str_replace_all("abc", {{"xyz", "123"}});
  EXPECT_EQ(s, "abc");

  // Replace entire string.
  s = turbo::str_replace_all("abc", {{"abc", "xyz"}});
  EXPECT_EQ(s, "xyz");

  // Replace once at the start.
  s = turbo::str_replace_all("abc", {{"a", "x"}});
  EXPECT_EQ(s, "xbc");

  // Replace once in the middle.
  s = turbo::str_replace_all("abc", {{"b", "x"}});
  EXPECT_EQ(s, "axc");

  // Replace once at the end.
  s = turbo::str_replace_all("abc", {{"c", "x"}});
  EXPECT_EQ(s, "abx");

  // Replace multiple times with varying lengths of original/replacement.
  s = turbo::str_replace_all("ababa", {{"a", "xxx"}});
  EXPECT_EQ(s, "xxxbxxxbxxx");

  s = turbo::str_replace_all("ababa", {{"b", "xxx"}});
  EXPECT_EQ(s, "axxxaxxxa");

  s = turbo::str_replace_all("aaabaaabaaa", {{"aaa", "x"}});
  EXPECT_EQ(s, "xbxbx");

  s = turbo::str_replace_all("abbbabbba", {{"bbb", "x"}});
  EXPECT_EQ(s, "axaxa");

  // Overlapping matches are replaced greedily.
  s = turbo::str_replace_all("aaa", {{"aa", "x"}});
  EXPECT_EQ(s, "xa");

  // The replacements are not recursive.
  s = turbo::str_replace_all("aaa", {{"aa", "a"}});
  EXPECT_EQ(s, "aa");
}

TEST(str_replace_all, ManyReplacements) {
  std::string s;

  // Empty string.
  s = turbo::str_replace_all("", {{"", ""}, {"x", ""}, {"", "y"}, {"x", "y"}});
  EXPECT_EQ(s, "");

  // Empty substring.
  s = turbo::str_replace_all("abc", {{"", ""}, {"", "y"}, {"x", ""}});
  EXPECT_EQ(s, "abc");

  // Replace entire string, one char at a time
  s = turbo::str_replace_all("abc", {{"a", "x"}, {"b", "y"}, {"c", "z"}});
  EXPECT_EQ(s, "xyz");
  s = turbo::str_replace_all("zxy", {{"z", "x"}, {"x", "y"}, {"y", "z"}});
  EXPECT_EQ(s, "xyz");

  // Replace once at the start (longer matches take precedence)
  s = turbo::str_replace_all("abc", {{"a", "x"}, {"ab", "xy"}, {"abc", "xyz"}});
  EXPECT_EQ(s, "xyz");

  // Replace once in the middle.
  s = turbo::str_replace_all(
      "Abc!", {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc", "yz"}, {"c", "z"}});
  EXPECT_EQ(s, "Ayz!");

  // Replace once at the end.
  s = turbo::str_replace_all(
      "Abc!",
      {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc!", "yz?"}, {"c!", "z;"}});
  EXPECT_EQ(s, "Ayz?");

  // Replace multiple times with varying lengths of original/replacement.
  s = turbo::str_replace_all("ababa", {{"a", "xxx"}, {"b", "XXXX"}});
  EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

  // Overlapping matches are replaced greedily.
  s = turbo::str_replace_all("aaa", {{"aa", "x"}, {"a", "X"}});
  EXPECT_EQ(s, "xX");
  s = turbo::str_replace_all("aaa", {{"a", "X"}, {"aa", "x"}});
  EXPECT_EQ(s, "xX");

  // Two well-known sentences
  s = turbo::str_replace_all("the quick brown fox jumped over the lazy dogs",
                          {
                              {"brown", "box"},
                              {"dogs", "jugs"},
                              {"fox", "with"},
                              {"jumped", "five"},
                              {"over", "dozen"},
                              {"quick", "my"},
                              {"the", "pack"},
                              {"the lazy", "liquor"},
                          });
  EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
}

TEST(str_replace_all, ManyReplacementsInMap) {
  std::map<const char *, const char *> replacements;
  replacements["$who"] = "Bob";
  replacements["$count"] = "5";
  replacements["#Noun"] = "Apples";
  std::string s = turbo::str_replace_all("$who bought $count #Noun. Thanks $who!",
                                      replacements);
  EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

TEST(str_replace_all, ReplacementsInPlace) {
  std::string s = std::string("$who bought $count #Noun. Thanks $who!");
  int count;
  count = turbo::str_replace_all({{"$count", turbo::str_cat(5)},
                              {"$who", "Bob"},
                              {"#Noun", "Apples"}}, &s);
  EXPECT_EQ(count, 4);
  EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

TEST(str_replace_all, ReplacementsInPlaceInMap) {
  std::string s = std::string("$who bought $count #Noun. Thanks $who!");
  std::map<std::string_view, std::string_view> replacements;
  replacements["$who"] = "Bob";
  replacements["$count"] = "5";
  replacements["#Noun"] = "Apples";
  int count;
  count = turbo::str_replace_all(replacements, &s);
  EXPECT_EQ(count, 4);
  EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

struct Cont {
  Cont() = default;
  explicit Cont(std::string_view src) : data(src) {}

  std::string_view data;
};

template <int index>
std::string_view get(const Cont& c) {
  auto splitter = turbo::str_split(c.data, ':');
  auto it = splitter.begin();
  for (int i = 0; i < index; ++i) ++it;

  return *it;
}

TEST(str_replace_all, VariableNumber) {
  std::string s;
  {
    std::vector<std::pair<std::string, std::string>> replacements;

    s = "abc";
    EXPECT_EQ(0, turbo::str_replace_all(replacements, &s));
    EXPECT_EQ("abc", s);

    s = "abc";
    replacements.push_back({"a", "A"});
    EXPECT_EQ(1, turbo::str_replace_all(replacements, &s));
    EXPECT_EQ("Abc", s);

    s = "abc";
    replacements.push_back({"b", "B"});
    EXPECT_EQ(2, turbo::str_replace_all(replacements, &s));
    EXPECT_EQ("ABc", s);

    s = "abc";
    replacements.push_back({"d", "D"});
    EXPECT_EQ(2, turbo::str_replace_all(replacements, &s));
    EXPECT_EQ("ABc", s);

    EXPECT_EQ("ABcABc", turbo::str_replace_all("abcabc", replacements));
  }

  {
    std::map<const char*, const char*> replacements;
    replacements["aa"] = "x";
    replacements["a"] = "X";
    s = "aaa";
    EXPECT_EQ(2, turbo::str_replace_all(replacements, &s));
    EXPECT_EQ("xX", s);

    EXPECT_EQ("xxX", turbo::str_replace_all("aaaaa", replacements));
  }

  {
    std::list<std::pair<std::string_view, std::string_view>> replacements = {
        {"a", "x"}, {"b", "y"}, {"c", "z"}};

    std::string s = turbo::str_replace_all("abc", replacements);
    EXPECT_EQ(s, "xyz");
  }

  {
    using X = std::tuple<std::string_view, std::string, int>;
    std::vector<X> replacements(3);
    replacements[0] = X{"a", "x", 1};
    replacements[1] = X{"b", "y", 0};
    replacements[2] = X{"c", "z", -1};

    std::string s = turbo::str_replace_all("abc", replacements);
    EXPECT_EQ(s, "xyz");
  }

  {
    std::vector<Cont> replacements(3);
    replacements[0] = Cont{"a:x"};
    replacements[1] = Cont{"b:y"};
    replacements[2] = Cont{"c:z"};

    std::string s = turbo::str_replace_all("abc", replacements);
    EXPECT_EQ(s, "xyz");
  }
}

// Same as above, but using the in-place variant of turbo::str_replace_all,
// that returns the # of replacements performed.
TEST(str_replace_all, Inplace) {
  std::string s;
  int reps;

  // Empty string.
  s = "";
  reps = turbo::str_replace_all({{"", ""}, {"x", ""}, {"", "y"}, {"x", "y"}}, &s);
  EXPECT_EQ(reps, 0);
  EXPECT_EQ(s, "");

  // Empty substring.
  s = "abc";
  reps = turbo::str_replace_all({{"", ""}, {"", "y"}, {"x", ""}}, &s);
  EXPECT_EQ(reps, 0);
  EXPECT_EQ(s, "abc");

  // Replace entire string, one char at a time
  s = "abc";
  reps = turbo::str_replace_all({{"a", "x"}, {"b", "y"}, {"c", "z"}}, &s);
  EXPECT_EQ(reps, 3);
  EXPECT_EQ(s, "xyz");
  s = "zxy";
  reps = turbo::str_replace_all({{"z", "x"}, {"x", "y"}, {"y", "z"}}, &s);
  EXPECT_EQ(reps, 3);
  EXPECT_EQ(s, "xyz");

  // Replace once at the start (longer matches take precedence)
  s = "abc";
  reps = turbo::str_replace_all({{"a", "x"}, {"ab", "xy"}, {"abc", "xyz"}}, &s);
  EXPECT_EQ(reps, 1);
  EXPECT_EQ(s, "xyz");

  // Replace once in the middle.
  s = "Abc!";
  reps = turbo::str_replace_all(
      {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc", "yz"}, {"c", "z"}}, &s);
  EXPECT_EQ(reps, 1);
  EXPECT_EQ(s, "Ayz!");

  // Replace once at the end.
  s = "Abc!";
  reps = turbo::str_replace_all(
      {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc!", "yz?"}, {"c!", "z;"}}, &s);
  EXPECT_EQ(reps, 1);
  EXPECT_EQ(s, "Ayz?");

  // Replace multiple times with varying lengths of original/replacement.
  s = "ababa";
  reps = turbo::str_replace_all({{"a", "xxx"}, {"b", "XXXX"}}, &s);
  EXPECT_EQ(reps, 5);
  EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

  // Overlapping matches are replaced greedily.
  s = "aaa";
  reps = turbo::str_replace_all({{"aa", "x"}, {"a", "X"}}, &s);
  EXPECT_EQ(reps, 2);
  EXPECT_EQ(s, "xX");
  s = "aaa";
  reps = turbo::str_replace_all({{"a", "X"}, {"aa", "x"}}, &s);
  EXPECT_EQ(reps, 2);
  EXPECT_EQ(s, "xX");

  // Two well-known sentences
  s = "the quick brown fox jumped over the lazy dogs";
  reps = turbo::str_replace_all(
      {
          {"brown", "box"},
          {"dogs", "jugs"},
          {"fox", "with"},
          {"jumped", "five"},
          {"over", "dozen"},
          {"quick", "my"},
          {"the", "pack"},
          {"the lazy", "liquor"},
      },
      &s);
  EXPECT_EQ(reps, 8);
  EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
}
