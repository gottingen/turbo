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

#include "turbo/strings/str_replace.h"

#include <list>
#include <map>
#include <tuple>

#include "gtest/gtest.h"
#include "turbo/format/str_format.h"
#include "turbo/strings/str_split.h"

TEST(StrReplaceAll, OneReplacement) {
    std::string s;

    // Empty string.
    s = turbo::StrReplaceAll(s, {{"", ""}});
    EXPECT_EQ(s, "");
    s = turbo::StrReplaceAll(s, {{"x", ""}});
    EXPECT_EQ(s, "");
    s = turbo::StrReplaceAll(s, {{"", "y"}});
    EXPECT_EQ(s, "");
    s = turbo::StrReplaceAll(s, {{"x", "y"}});
    EXPECT_EQ(s, "");

    // Empty substring.
    s = turbo::StrReplaceAll("abc", {{"", ""}});
    EXPECT_EQ(s, "abc");
    s = turbo::StrReplaceAll("abc", {{"", "y"}});
    EXPECT_EQ(s, "abc");
    s = turbo::StrReplaceAll("abc", {{"x", ""}});
    EXPECT_EQ(s, "abc");

    // Substring not found.
    s = turbo::StrReplaceAll("abc", {{"xyz", "123"}});
    EXPECT_EQ(s, "abc");

    // Replace entire string.
    s = turbo::StrReplaceAll("abc", {{"abc", "xyz"}});
    EXPECT_EQ(s, "xyz");

    // Replace once at the start.
    s = turbo::StrReplaceAll("abc", {{"a", "x"}});
    EXPECT_EQ(s, "xbc");

    // Replace once in the middle.
    s = turbo::StrReplaceAll("abc", {{"b", "x"}});
    EXPECT_EQ(s, "axc");

    // Replace once at the end.
    s = turbo::StrReplaceAll("abc", {{"c", "x"}});
    EXPECT_EQ(s, "abx");

    // Replace multiple times with varying lengths of original/replacement.
    s = turbo::StrReplaceAll("ababa", {{"a", "xxx"}});
    EXPECT_EQ(s, "xxxbxxxbxxx");

    s = turbo::StrReplaceAll("ababa", {{"b", "xxx"}});
    EXPECT_EQ(s, "axxxaxxxa");

    s = turbo::StrReplaceAll("aaabaaabaaa", {{"aaa", "x"}});
    EXPECT_EQ(s, "xbxbx");

    s = turbo::StrReplaceAll("abbbabbba", {{"bbb", "x"}});
    EXPECT_EQ(s, "axaxa");

    // Overlapping matches are replaced greedily.
    s = turbo::StrReplaceAll("aaa", {{"aa", "x"}});
    EXPECT_EQ(s, "xa");

    // The replacements are not recursive.
    s = turbo::StrReplaceAll("aaa", {{"aa", "a"}});
    EXPECT_EQ(s, "aa");
}

TEST(StrReplaceAll, ManyReplacements) {
    std::string s;

    // Empty string.
    s = turbo::StrReplaceAll("", {{"",  ""},
                                  {"x", ""},
                                  {"",  "y"},
                                  {"x", "y"}});
    EXPECT_EQ(s, "");

    // Empty substring.
    s = turbo::StrReplaceAll("abc", {{"",  ""},
                                     {"",  "y"},
                                     {"x", ""}});
    EXPECT_EQ(s, "abc");

    // Replace entire string, one char at a time
    s = turbo::StrReplaceAll("abc", {{"a", "x"},
                                     {"b", "y"},
                                     {"c", "z"}});
    EXPECT_EQ(s, "xyz");
    s = turbo::StrReplaceAll("zxy", {{"z", "x"},
                                     {"x", "y"},
                                     {"y", "z"}});
    EXPECT_EQ(s, "xyz");

    // Replace once at the start (longer matches take precedence)
    s = turbo::StrReplaceAll("abc", {{"a",   "x"},
                                     {"ab",  "xy"},
                                     {"abc", "xyz"}});
    EXPECT_EQ(s, "xyz");

    // Replace once in the middle.
    s = turbo::StrReplaceAll(
            "Abc!", {{"a",  "x"},
                     {"ab", "xy"},
                     {"b",  "y"},
                     {"bc", "yz"},
                     {"c",  "z"}});
    EXPECT_EQ(s, "Ayz!");

    // Replace once at the end.
    s = turbo::StrReplaceAll(
            "Abc!",
            {{"a",   "x"},
             {"ab",  "xy"},
             {"b",   "y"},
             {"bc!", "yz?"},
             {"c!",  "z;"}});
    EXPECT_EQ(s, "Ayz?");

    // Replace multiple times with varying lengths of original/replacement.
    s = turbo::StrReplaceAll("ababa", {{"a", "xxx"},
                                       {"b", "XXXX"}});
    EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

    // Overlapping matches are replaced greedily.
    s = turbo::StrReplaceAll("aaa", {{"aa", "x"},
                                     {"a",  "X"}});
    EXPECT_EQ(s, "xX");
    s = turbo::StrReplaceAll("aaa", {{"a",  "X"},
                                     {"aa", "x"}});
    EXPECT_EQ(s, "xX");

    // Two well-known sentences
    s = turbo::StrReplaceAll("the quick brown fox jumped over the lazy dogs",
                             {
                                     {"brown",    "box"},
                                     {"dogs",     "jugs"},
                                     {"fox",      "with"},
                                     {"jumped",   "five"},
                                     {"over",     "dozen"},
                                     {"quick",    "my"},
                                     {"the",      "pack"},
                                     {"the lazy", "liquor"},
                             });
    EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
}

TEST(StrReplaceAll, ManyReplacementsInMap) {
    std::map<const char *, const char *> replacements;
    replacements["$who"] = "Bob";
    replacements["$count"] = "5";
    replacements["#Noun"] = "Apples";
    std::string s = turbo::StrReplaceAll("$who bought $count #Noun. Thanks $who!",
                                         replacements);
    EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

TEST(StrReplaceAll, ReplacementsInPlace) {
    std::string s = std::string("$who bought $count #Noun. Thanks $who!");
    int count;
    count = turbo::StrReplaceAll({{"$count", turbo::Format(5)},
                                  {"$who",   "Bob"},
                                  {"#Noun",  "Apples"}}, &s);
    EXPECT_EQ(count, 4);
    EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

TEST(StrReplaceAll, ReplacementsInPlaceInMap) {
    std::string s = std::string("$who bought $count #Noun. Thanks $who!");
    std::map<std::string_view, std::string_view> replacements;
    replacements["$who"] = "Bob";
    replacements["$count"] = "5";
    replacements["#Noun"] = "Apples";
    int count;
    count = turbo::StrReplaceAll(replacements, &s);
    EXPECT_EQ(count, 4);
    EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
}

struct Cont {
    Cont() {}

    explicit Cont(std::string_view src) : data(src) {}

    std::string_view data;
};

template<int index>
std::string_view get(const Cont &c) {
    auto splitter = turbo::StrSplit(c.data, ':');
    auto it = splitter.begin();
    for (int i = 0; i < index; ++i) ++it;

    return *it;
}

TEST(StrReplaceAll, VariableNumber) {
    std::string s;
    {
        std::vector<std::pair<std::string, std::string>> replacements;

        s = "abc";
        EXPECT_EQ(0, turbo::StrReplaceAll(replacements, &s));
        EXPECT_EQ("abc", s);

        s = "abc";
        replacements.push_back({"a", "A"});
        EXPECT_EQ(1, turbo::StrReplaceAll(replacements, &s));
        EXPECT_EQ("Abc", s);

        s = "abc";
        replacements.push_back({"b", "B"});
        EXPECT_EQ(2, turbo::StrReplaceAll(replacements, &s));
        EXPECT_EQ("ABc", s);

        s = "abc";
        replacements.push_back({"d", "D"});
        EXPECT_EQ(2, turbo::StrReplaceAll(replacements, &s));
        EXPECT_EQ("ABc", s);

        EXPECT_EQ("ABcABc", turbo::StrReplaceAll("abcabc", replacements));
    }

    {
        std::map<const char *, const char *> replacements;
        replacements["aa"] = "x";
        replacements["a"] = "X";
        s = "aaa";
        EXPECT_EQ(2, turbo::StrReplaceAll(replacements, &s));
        EXPECT_EQ("xX", s);

        EXPECT_EQ("xxX", turbo::StrReplaceAll("aaaaa", replacements));
    }

    {
        std::list<std::pair<std::string_view, std::string_view>> replacements = {
                {"a", "x"},
                {"b", "y"},
                {"c", "z"}};

        std::string s = turbo::StrReplaceAll("abc", replacements);
        EXPECT_EQ(s, "xyz");
    }

    {
        using X = std::tuple<std::string_view, std::string, int>;
        std::vector<X> replacements(3);
        replacements[0] = X{"a", "x", 1};
        replacements[1] = X{"b", "y", 0};
        replacements[2] = X{"c", "z", -1};

        std::string s = turbo::StrReplaceAll("abc", replacements);
        EXPECT_EQ(s, "xyz");
    }

    {
        std::vector<Cont> replacements(3);
        replacements[0] = Cont{"a:x"};
        replacements[1] = Cont{"b:y"};
        replacements[2] = Cont{"c:z"};

        std::string s = turbo::StrReplaceAll("abc", replacements);
        EXPECT_EQ(s, "xyz");
    }
}

// Same as above, but using the in-place variant of turbo::StrReplaceAll,
// that returns the # of replacements performed.
TEST(StrReplaceAll, Inplace) {
    std::string s;
    int reps;

    // Empty string.
    s = "";
    reps = turbo::StrReplaceAll({{"",  ""},
                                 {"x", ""},
                                 {"",  "y"},
                                 {"x", "y"}}, &s);
    EXPECT_EQ(reps, 0);
    EXPECT_EQ(s, "");

    // Empty substring.
    s = "abc";
    reps = turbo::StrReplaceAll({{"",  ""},
                                 {"",  "y"},
                                 {"x", ""}}, &s);
    EXPECT_EQ(reps, 0);
    EXPECT_EQ(s, "abc");

    // Replace entire string, one char at a time
    s = "abc";
    reps = turbo::StrReplaceAll({{"a", "x"},
                                 {"b", "y"},
                                 {"c", "z"}}, &s);
    EXPECT_EQ(reps, 3);
    EXPECT_EQ(s, "xyz");
    s = "zxy";
    reps = turbo::StrReplaceAll({{"z", "x"},
                                 {"x", "y"},
                                 {"y", "z"}}, &s);
    EXPECT_EQ(reps, 3);
    EXPECT_EQ(s, "xyz");

    // Replace once at the start (longer matches take precedence)
    s = "abc";
    reps = turbo::StrReplaceAll({{"a",   "x"},
                                 {"ab",  "xy"},
                                 {"abc", "xyz"}}, &s);
    EXPECT_EQ(reps, 1);
    EXPECT_EQ(s, "xyz");

    // Replace once in the middle.
    s = "Abc!";
    reps = turbo::StrReplaceAll(
            {{"a",  "x"},
             {"ab", "xy"},
             {"b",  "y"},
             {"bc", "yz"},
             {"c",  "z"}}, &s);
    EXPECT_EQ(reps, 1);
    EXPECT_EQ(s, "Ayz!");

    // Replace once at the end.
    s = "Abc!";
    reps = turbo::StrReplaceAll(
            {{"a",   "x"},
             {"ab",  "xy"},
             {"b",   "y"},
             {"bc!", "yz?"},
             {"c!",  "z;"}}, &s);
    EXPECT_EQ(reps, 1);
    EXPECT_EQ(s, "Ayz?");

    // Replace multiple times with varying lengths of original/replacement.
    s = "ababa";
    reps = turbo::StrReplaceAll({{"a", "xxx"},
                                 {"b", "XXXX"}}, &s);
    EXPECT_EQ(reps, 5);
    EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

    // Overlapping matches are replaced greedily.
    s = "aaa";
    reps = turbo::StrReplaceAll({{"aa", "x"},
                                 {"a",  "X"}}, &s);
    EXPECT_EQ(reps, 2);
    EXPECT_EQ(s, "xX");
    s = "aaa";
    reps = turbo::StrReplaceAll({{"a",  "X"},
                                 {"aa", "x"}}, &s);
    EXPECT_EQ(reps, 2);
    EXPECT_EQ(s, "xX");

    // Two well-known sentences
    s = "the quick brown fox jumped over the lazy dogs";
    reps = turbo::StrReplaceAll(
            {
                    {"brown",    "box"},
                    {"dogs",     "jugs"},
                    {"fox",      "with"},
                    {"jumped",   "five"},
                    {"over",     "dozen"},
                    {"quick",    "my"},
                    {"the",      "pack"},
                    {"the lazy", "liquor"},
            },
            &s);
    EXPECT_EQ(reps, 8);
    EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
}
