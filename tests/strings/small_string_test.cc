//
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
//
// Created by jeff on 24-6-5.
//

#include <turbo/strings/small_string.h>
#include <gtest/gtest.h>
#include <gtest/gtest-printers.h>
#include <turbo/hash/hash.h>
#include <climits>
#include <cstring>
#include <stdarg.h>

using namespace turbo;

namespace turbo {
        template<unsigned N>
        inline void PrintTo(const turbo::SmallString <N> &str, ::std::ostream *os) { *os << str.str(); }
}
namespace {

    // Test fixture class
    class SmallStringTest : public testing::Test {
    protected:
        typedef SmallString<40> StringType;

        StringType theString;

        void assertEmpty(StringType & v) {
            // Size tests
            EXPECT_EQ(0u, v.size());
            EXPECT_TRUE(v.empty());
            // Iterator tests
            EXPECT_TRUE(v.begin() == v.end());
        }
    };

    // New string test.
    TEST_F(SmallStringTest, EmptyStringTest) {
        SCOPED_TRACE("EmptyStringTest");
        assertEmpty(theString);
        EXPECT_TRUE(theString.rbegin() == theString.rend());
    }

    TEST_F(SmallStringTest, AssignRepeated) {
        theString.assign(3, 'a');
        EXPECT_EQ(3u, theString.size());
        EXPECT_STREQ("aaa", theString.c_str());
    }

    TEST_F(SmallStringTest, AssignIterPair) {
         turbo::string_view abc = "abc";
        theString.assign(abc.begin(), abc.end());
        EXPECT_EQ(3u, theString.size());
        EXPECT_STREQ("abc", theString.c_str());
    }

    TEST_F(SmallStringTest, Assignstringview) {
         turbo::string_view abc = "abc";
        theString.assign(abc);
        EXPECT_EQ(3u, theString.size());
        EXPECT_STREQ("abc", theString.c_str());
    }

    TEST_F(SmallStringTest, AssignSmallVector) {
         turbo::string_view abc = "abc";
        SmallVector<char, 10> abcVec(abc.begin(), abc.end());
        theString.assign(abcVec);
        EXPECT_EQ(3u, theString.size());
        EXPECT_STREQ("abc", theString.c_str());
    }

    TEST_F(SmallStringTest, Assignstringviews) {
        theString.assign({"abc", "def", "ghi"});
        EXPECT_EQ(9u, theString.size());
        EXPECT_STREQ("abcdefghi", theString.c_str());
    }

    TEST_F(SmallStringTest, AppendIterPair) {
         turbo::string_view abc = "abc";
        theString.append(abc.begin(), abc.end());
        theString.append(abc.begin(), abc.end());
        EXPECT_EQ(6u, theString.size());
        EXPECT_STREQ("abcabc", theString.c_str());
    }

    TEST_F(SmallStringTest, Appendstringview) {
         turbo::string_view abc = "abc";
        theString.append(abc);
        theString.append(abc);
        EXPECT_EQ(6u, theString.size());
        EXPECT_STREQ("abcabc", theString.c_str());
    }

    TEST_F(SmallStringTest, AppendSmallVector) {
         turbo::string_view abc = "abc";
        SmallVector<char, 10> abcVec(abc.begin(), abc.end());
        theString.append(abcVec);
        theString.append(abcVec);
        EXPECT_EQ(6u, theString.size());
        EXPECT_STREQ("abcabc", theString.c_str());
    }

    TEST_F(SmallStringTest, Appendstringviews) {
        theString.append({"abc", "def", "ghi"});
        EXPECT_EQ(9u, theString.size());
        EXPECT_STREQ("abcdefghi", theString.c_str());
         turbo::string_view Jkl = "jkl";
        std::string Mno = "mno";
        SmallString<4> Pqr("pqr");
        const char *Stu = "stu";
        theString.append({Jkl, Mno, Pqr, Stu});
        EXPECT_EQ(21u, theString.size());
        EXPECT_STREQ("abcdefghijklmnopqrstu", theString.c_str());
    }

    TEST_F(SmallStringTest, stringviewConversion) {
         turbo::string_view abc = "abc";
        theString.assign(abc.begin(), abc.end());
         turbo::string_view thestringview = theString;
        EXPECT_EQ("abc", thestringview);
    }

    TEST_F(SmallStringTest, StdStringConversion) {
         turbo::string_view abc = "abc";
        theString.assign(abc.begin(), abc.end());
        std::string theStdString = std::string(theString);
        EXPECT_EQ("abc", theStdString);
    }

    TEST_F(SmallStringTest, Substr) {
        theString = "hello";
        EXPECT_EQ("lo", theString.substr(3));
        EXPECT_EQ("", theString.substr(100));
        EXPECT_EQ("hello", theString.substr(0, 100));
        EXPECT_EQ("o", theString.substr(4, 10));
    }

    TEST_F(SmallStringTest, Slice) {
        theString = "hello";
        EXPECT_EQ("l", theString.slice(2, 3));
        EXPECT_EQ("ell", theString.slice(1, 4));
        EXPECT_EQ("llo", theString.slice(2, 100));
        EXPECT_EQ("", theString.slice(2, 1));
        EXPECT_EQ("", theString.slice(10, 20));
    }

    TEST_F(SmallStringTest, Find) {
        theString = "hello";
        EXPECT_EQ(2U, theString.find('l'));
        EXPECT_EQ( turbo::string_view::npos, theString.find('z'));
        EXPECT_EQ( turbo::string_view::npos, theString.find("helloworld"));
        EXPECT_EQ(0U, theString.find("hello"));
        EXPECT_EQ(1U, theString.find("ello"));
        EXPECT_EQ( turbo::string_view::npos, theString.find("zz"));
        EXPECT_EQ(2U, theString.find("ll", 2));
        EXPECT_EQ( turbo::string_view::npos, theString.find("ll", 3));
        EXPECT_EQ(0U, theString.find(""));

        EXPECT_EQ(3U, theString.rfind('l'));
        EXPECT_EQ( turbo::string_view::npos, theString.rfind('z'));
        EXPECT_EQ( turbo::string_view::npos, theString.rfind("helloworld"));
        EXPECT_EQ(0U, theString.rfind("hello"));
        EXPECT_EQ(1U, theString.rfind("ello"));
        EXPECT_EQ( turbo::string_view::npos, theString.rfind("zz"));

        EXPECT_EQ(2U, theString.find_first_of('l'));
        EXPECT_EQ(1U, theString.find_first_of("el"));
        EXPECT_EQ( turbo::string_view::npos, theString.find_first_of("xyz"));

        EXPECT_EQ(1U, theString.find_first_not_of('h'));
        EXPECT_EQ(4U, theString.find_first_not_of("hel"));
        EXPECT_EQ( turbo::string_view::npos, theString.find_first_not_of("hello"));

        theString = "hellx xello hell ello world foo bar hello";
        EXPECT_EQ(36U, theString.find("hello"));
        EXPECT_EQ(28U, theString.find("foo"));
        EXPECT_EQ(12U, theString.find("hell", 2));
        EXPECT_EQ(0U, theString.find(""));
    }

    TEST_F(SmallStringTest, Realloc) {
        theString = "abcd";
        theString.reserve(100);
        EXPECT_EQ("abcd", theString);
        unsigned const N = 100000;
        theString.reserve(N);
        for (unsigned i = 0; i < N - 4; ++i)
            theString.push_back('y');
        EXPECT_EQ("abcdyyy", theString.substr(0, 7));
    }

    TEST_F(SmallStringTest, Comparisons) {
        EXPECT_GT( 0, SmallString<10>("aab").compare("aad"));
        EXPECT_EQ( 0, SmallString<10>("aab").compare("aab"));
        EXPECT_LT( 0, SmallString<10>("aab").compare("aaa"));
        EXPECT_GT( 0, SmallString<10>("aab").compare("aabb"));
        EXPECT_LT( 0, SmallString<10>("aab").compare("aa"));
        EXPECT_LT( 0, SmallString<10>("\xFF").compare("\1"));

        EXPECT_EQ(-1, SmallString<10>("AaB").compare_insensitive("aAd"));
        EXPECT_EQ( 0, SmallString<10>("AaB").compare_insensitive("aab"));
        EXPECT_EQ( 1, SmallString<10>("AaB").compare_insensitive("AAA"));
        EXPECT_EQ(-1, SmallString<10>("AaB").compare_insensitive("aaBb"));
        EXPECT_EQ( 1, SmallString<10>("AaB").compare_insensitive("aA"));
        EXPECT_EQ( 1, SmallString<10>("\xFF").compare_insensitive("\1"));

    }

    TEST_F(SmallStringTest, Hash) {
        turbo::string_view abc = "abcvd";
        SmallString<10> abc2(abc);
        EXPECT_EQ(turbo::Hash<turbo::string_view>()(abc), turbo::Hash<SmallString<10>>()(abc2));
    }
} // namespace
