// Copyright 2023 The Turbo Authors.
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
//

#include "turbo/strings/str_trim.h"
#include "turbo/strings/inlined_string.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "tests/doctest/doctest.h"

TEST_CASE("TrimLeft, FromStringView") {
    CHECK_EQ(std::string_view{},
             turbo::TrimLeft(std::string_view{}));
    CHECK_EQ("foo", turbo::TrimLeft({"foo"}));
    CHECK_EQ("foo", turbo::TrimLeft({"\t  \n\f\r\n\vfoo"}));
    CHECK_EQ("foo foo\n ",
             turbo::TrimLeft({"\t  \n\f\r\n\vfoo foo\n "}));
    CHECK_EQ(std::string_view{}, turbo::TrimLeft(
            {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}


template<typename Str>
void TestInPlace() {
    Str str;

    turbo::TrimLeft(&str);
    CHECK_EQ("", str);

    str = "foo";
    turbo::TrimLeft(&str);
    CHECK_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo";
    turbo::TrimLeft(&str);
    CHECK_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\n ";
    turbo::TrimLeft(&str);
    CHECK_EQ("foo foo\n ", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::TrimLeft(&str);
    CHECK_EQ(std::string_view{}, str);
}

TEST_CASE("TrimLeft, InPlace") {
    TestInPlace<std::string>();

    TestInPlace<turbo::inlined_string>();

}

TEST_CASE("TrimRight, FromStringView") {
    CHECK_EQ(std::string_view{}, turbo::TrimRight(std::string_view{}));
    CHECK_EQ("foo", turbo::TrimRight({"foo"}));
    CHECK_EQ("foo", turbo::TrimRight({"foo\t  \n\f\r\n\v"}));
    CHECK_EQ(" \nfoo foo", turbo::TrimRight({" \nfoo foo\t  \n\f\r\n\v"}));
    CHECK_EQ(std::string_view{}, turbo::TrimRight({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

template<typename String>
void StripTrailingAsciiWhitespaceinplace() {
    String str;

    turbo::TrimRight(&str);
    CHECK_EQ("", str);

    str = "foo";
    turbo::TrimRight(&str);
    CHECK_EQ("foo", str);

    str = "foo\t  \n\f\r\n\v";
    turbo::TrimRight(&str);
    CHECK_EQ("foo", str);

    str = " \nfoo foo\t  \n\f\r\n\v";
    turbo::TrimRight(&str);
    CHECK_EQ(" \nfoo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::TrimRight(&str);
    CHECK_EQ(std::string_view{}, str);
}

TEST_CASE("TrimRight, InPlace") {
    StripTrailingAsciiWhitespaceinplace<std::string>();

    StripTrailingAsciiWhitespaceinplace<turbo::inlined_string>();

}

TEST_CASE("Trim, FromStringView") {
    CHECK_EQ(std::string_view{},
             turbo::Trim(std::string_view{}));
    CHECK_EQ("foo", turbo::Trim({"foo"}));
    CHECK_EQ("foo", turbo::Trim({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
    CHECK_EQ("foo foo", turbo::Trim({"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
    CHECK_EQ(std::string_view{}, turbo::Trim({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

template<typename Str>
void StripAsciiWhitespaceInPlace() {
    Str str;

    turbo::Trim(&str);
    CHECK_EQ("", str);

    str = "foo";
    turbo::Trim(&str);
    CHECK_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
    turbo::Trim(&str);
    CHECK_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
    turbo::Trim(&str);
    CHECK_EQ("foo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::Trim(&str);
    CHECK_EQ(std::string_view{}, str);
}

TEST_CASE("Trim, InPlace") {
    StripAsciiWhitespaceInPlace<std::string>();

    StripAsciiWhitespaceInPlace<turbo::inlined_string>();

}


template<typename String>
void RemoveExtraAsciiWhitespaceInplace() {
    const char *inputs[] = {"No extra space",
                            "  Leading whitespace",
                            "Trailing whitespace  ",
                            "  Leading and trailing  ",
                            " Whitespace \t  in\v   middle  ",
                            "'Eeeeep!  \n Newlines!\n",
                            "nospaces",
                            "",
                            "\n\t a\t\n\nb \t\n"};

    const char *outputs[] = {
            "No extra space",
            "Leading whitespace",
            "Trailing whitespace",
            "Leading and trailing",
            "Whitespace in middle",
            "'Eeeeep! Newlines!",
            "nospaces",
            "",
            "a\nb",
    };
    const int NUM_TESTS = TURBO_ARRAY_SIZE(inputs);

    for (int i = 0; i < NUM_TESTS; i++) {
        String s(inputs[i]);
        turbo::TrimAll(&s);
        CHECK_EQ(outputs[i], s);
    }
}

TEST_CASE("TrimAll, InPlace") {
    RemoveExtraAsciiWhitespaceInplace<std::string>();
    RemoveExtraAsciiWhitespaceInplace<turbo::inlined_string>();
}