
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/strings/case_conv.h"
#include "turbo/strings/ascii.h"
#include <string_view>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include "testing/gtest_wrap.h"
#include "turbo/base/profile.h"


TEST(AsciiStrTo, Lower) {
    const char buf[] = "ABCDEF";
    const std::string str("GHIJKL");
    const std::string str2("MNOPQR");
    const std::string_view sp(str2);

    EXPECT_EQ("abcdef", turbo::string_to_lower(buf));
    EXPECT_EQ("ghijkl", turbo::string_to_lower(str));
    EXPECT_EQ("mnopqr", turbo::string_to_lower(sp));

    char mutable_buf[] = "Mutable";
    std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                   mutable_buf, turbo::ascii::to_lower);
    EXPECT_STREQ("mutable", mutable_buf);
}

TEST(AsciiStrTo, Upper) {
    const char buf[] = "abcdef";
    const std::string str("ghijkl");
    const std::string str2("mnopqr");
    const std::string_view sp(str2);

    EXPECT_EQ("ABCDEF", turbo::string_to_upper(buf));
    EXPECT_EQ("GHIJKL", turbo::string_to_upper(str));
    EXPECT_EQ("MNOPQR", turbo::string_to_upper(sp));

    char mutable_buf[] = "Mutable";
    std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                   mutable_buf, turbo::ascii::to_upper);
    EXPECT_STREQ("MUTABLE", mutable_buf);
}
