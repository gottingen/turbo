
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/strings/compare.h"
#include "testing/gtest_wrap.h"

namespace {

    TEST(MatchTest, EqualsIgnoreCase) {
        std::string text = "the";
        std::string_view data(text);

        EXPECT_TRUE(turbo::equal_case(data, "The"));
        EXPECT_TRUE(turbo::equal_case(data, "THE"));
        EXPECT_TRUE(turbo::equal_case(data, "the"));
        EXPECT_FALSE(turbo::equal_case(data, "Quick"));
        EXPECT_FALSE(turbo::equal_case(data, "then"));
    }

}
