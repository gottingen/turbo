
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/strings/trim.h"
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include "testing/gtest_wrap.h"
#include "turbo/base/profile.h"

TEST(trim_left, FromStringView) {
    EXPECT_EQ(std::string_view{},
              turbo::trim_left(std::string_view{}));
    EXPECT_EQ("foo", turbo::trim_left({"foo"}));
    EXPECT_EQ("foo", turbo::trim_left({"\t  \n\f\r\n\vfoo"}));
    EXPECT_EQ("foo foo\n ",
              turbo::trim_left({"\t  \n\f\r\n\vfoo foo\n "}));
    EXPECT_EQ(std::string_view{}, turbo::trim_left(
            {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_left, trim_inplace_left) {
    std::string str;

    turbo::trim_inplace_left(&str);
    EXPECT_EQ("", str);

    str = "foo";
    turbo::trim_inplace_left(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo";
    turbo::trim_inplace_left(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\n ";
    turbo::trim_inplace_left(&str);
    EXPECT_EQ("foo foo\n ", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::trim_inplace_left(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_right, FromStringView) {
    EXPECT_EQ(std::string_view{},
              turbo::trim_right(std::string_view{}));
    EXPECT_EQ("foo", turbo::trim_right({"foo"}));
    EXPECT_EQ("foo", turbo::trim_right({"foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(" \nfoo foo",
              turbo::trim_right({" \nfoo foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(std::string_view{}, turbo::trim_right(
            {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_right, trim_inplace_right) {
    std::string str;

    turbo::trim_inplace_right(&str);
    EXPECT_EQ("", str);

    str = "foo";
    turbo::trim_inplace_right(&str);
    EXPECT_EQ("foo", str);

    str = "foo\t  \n\f\r\n\v";
    turbo::trim_inplace_right(&str);
    EXPECT_EQ("foo", str);

    str = " \nfoo foo\t  \n\f\r\n\v";
    turbo::trim_inplace_right(&str);
    EXPECT_EQ(" \nfoo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::trim_inplace_right(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_all, FromStringView) {
    EXPECT_EQ(std::string_view{},
              turbo::trim_all(std::string_view{}));
    EXPECT_EQ("foo", turbo::trim_all({"foo"}));
    EXPECT_EQ("foo",
              turbo::trim_all({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
    EXPECT_EQ("foo foo", turbo::trim_all(
            {"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(std::string_view{},
              turbo::trim_all({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_all, trim_inplace_all) {
    std::string str;

    turbo::trim_inplace_all(&str);
    EXPECT_EQ("", str);

    str = "foo";
    turbo::trim_inplace_all(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
    turbo::trim_inplace_all(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
    turbo::trim_inplace_all(&str);
    EXPECT_EQ("foo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    turbo::trim_inplace_all(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_complete, trim_inplace_complete) {
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
        std::string s(inputs[i]);
        turbo::trim_inplace_complete(&s);
        EXPECT_EQ(outputs[i], s);
    }
}
