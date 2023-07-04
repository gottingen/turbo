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

#include "turbo/strings/match.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "tests/doctest/doctest.h"

namespace {

    TEST_CASE("MatchTest, StartsWith") {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        CHECK(turbo::StartsWith(a, a));
        CHECK(turbo::StartsWith(a, "foo"));
        CHECK(turbo::StartsWith(a, e));
        CHECK(turbo::StartsWith(b, s1));
        CHECK(turbo::StartsWith(b, b));
        CHECK(turbo::StartsWith(b, e));
        CHECK(turbo::StartsWith(e, ""));
        CHECK_FALSE(turbo::StartsWith(a, b));
        CHECK_FALSE(turbo::StartsWith(b, a));
        CHECK_FALSE(turbo::StartsWith(e, a));
    }

    TEST_CASE("MatchTest, EndsWith") {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        CHECK(turbo::EndsWith(a, a));
        CHECK(turbo::EndsWith(a, "bar"));
        CHECK(turbo::EndsWith(a, e));
        CHECK(turbo::EndsWith(b, s1));
        CHECK(turbo::EndsWith(b, b));
        CHECK(turbo::EndsWith(b, e));
        CHECK(turbo::EndsWith(e, ""));
        CHECK_FALSE(turbo::EndsWith(a, b));
        CHECK_FALSE(turbo::EndsWith(b, a));
        CHECK_FALSE(turbo::EndsWith(e, a));
    }

    TEST_CASE("MatchTest, Contains") {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        std::string_view c("efg");
        std::string_view d("gh");
        CHECK(turbo::StrContains(a, a));
        CHECK(turbo::StrContains(a, b));
        CHECK(turbo::StrContains(a, c));
        CHECK_FALSE(turbo::StrContains(a, d));
        CHECK(turbo::StrContains("", ""));
        CHECK(turbo::StrContains("abc", ""));
        CHECK_FALSE(turbo::StrContains("", "a"));
    }

    TEST_CASE("MatchTest, ContainsChar") {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        CHECK(turbo::StrContains(a, 'a'));
        CHECK(turbo::StrContains(a, 'b'));
        CHECK(turbo::StrContains(a, 'e'));
        CHECK_FALSE(turbo::StrContains(a, 'h'));

        CHECK(turbo::StrContains(b, 'a'));
        CHECK(turbo::StrContains(b, 'b'));
        CHECK_FALSE(turbo::StrContains(b, 'e'));
        CHECK_FALSE(turbo::StrContains(b, 'h'));

        CHECK_FALSE(turbo::StrContains("", 'a'));
        CHECK_FALSE(turbo::StrContains("", 'a'));
    }

    TEST_CASE("MatchTest, ContainsIgnoreCaseChar") {
        std::string_view a("abcdefg");
        std::string_view b("ABCD");
        CHECK(turbo::StrIgnoreCaseContains(a, 'a'));
        CHECK(turbo::StrIgnoreCaseContains(a, 'A'));
        CHECK(turbo::StrIgnoreCaseContains(a, 'b'));
        CHECK(turbo::StrIgnoreCaseContains(a, 'B'));
        CHECK(turbo::StrIgnoreCaseContains(a, 'e'));
        CHECK(turbo::StrIgnoreCaseContains(a, 'E'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains(a, 'h'));

        CHECK(turbo::StrIgnoreCaseContains(b, 'a'));
        CHECK(turbo::StrIgnoreCaseContains(b, 'A'));
        CHECK(turbo::StrIgnoreCaseContains(b, 'b'));
        CHECK(turbo::StrIgnoreCaseContains(b, 'B'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains(b, 'e'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains(b, 'E'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains(b, 'h'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains(b, 'H'));

        CHECK_FALSE(turbo::StrIgnoreCaseContains("", 'a'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains("", 'A'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains("", 'a'));
        CHECK_FALSE(turbo::StrIgnoreCaseContains("", 'A'));
    }

    TEST_CASE("MatchTest, ContainsNull") {
        const std::string s = "foo";
        const char *cs = "foo";
        const std::string_view sv("foo");
        const std::string_view sv2("foo\0bar", 4);
        CHECK_EQ(s, "foo");
        CHECK_EQ(sv, "foo");
        CHECK_NE(sv2, "foo");
        CHECK(turbo::EndsWith(s, sv));
        CHECK(turbo::StartsWith(cs, sv));
        CHECK(turbo::StrContains(cs, sv));
        CHECK_FALSE(turbo::StrContains(cs, sv2));
    }

    TEST_CASE("MatchTest, EqualsIgnoreCase") {
        std::string text = "the";
        std::string_view data(text);

        CHECK(turbo::EqualsIgnoreCase(data, "The"));
        CHECK(turbo::EqualsIgnoreCase(data, "THE"));
        CHECK(turbo::EqualsIgnoreCase(data, "the"));
        CHECK_FALSE(turbo::EqualsIgnoreCase(data, "Quick"));
        CHECK_FALSE(turbo::EqualsIgnoreCase(data, "then"));
    }

    TEST_CASE("MatchTest, StartsWithIgnoreCase") {
        CHECK(turbo::StartsWithIgnoreCase("foo", "foo"));
        CHECK(turbo::StartsWithIgnoreCase("foo", "Fo"));
        CHECK(turbo::StartsWithIgnoreCase("foo", ""));
        CHECK_FALSE(turbo::StartsWithIgnoreCase("foo", "fooo"));
        CHECK_FALSE(turbo::StartsWithIgnoreCase("", "fo"));
    }

    TEST_CASE("MatchTest, EndsWithIgnoreCase") {
        CHECK(turbo::EndsWithIgnoreCase("foo", "foo"));
        CHECK(turbo::EndsWithIgnoreCase("foo", "Oo"));
        CHECK(turbo::EndsWithIgnoreCase("foo", ""));
        CHECK_FALSE(turbo::EndsWithIgnoreCase("foo", "fooo"));
        CHECK_FALSE(turbo::EndsWithIgnoreCase("", "fo"));
    }

}  // namespace
