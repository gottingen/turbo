
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/base/uninitialized.h"
#include "testing/gtest_wrap.h"

namespace {

    int resize_call_count = 0;

    struct resizable_string {
        void resize(size_t) { resize_call_count += 1; }
    };

    int resize_default_init_call_count = 0;

    struct resize_default_init_string {
        void resize(size_t) { resize_call_count += 1; }

        void __resize_default_init(size_t) { resize_default_init_call_count += 1; }
    };

    TEST(ResizeUninit, WithAndWithout) {
        resize_call_count = 0;
        resize_default_init_call_count = 0;
        {
            resizable_string rs;

            EXPECT_EQ(resize_call_count, 0);
            EXPECT_EQ(resize_default_init_call_count, 0);
            EXPECT_FALSE(
                    flare::base::string_supports_uninitialized_resize(&rs));
            EXPECT_EQ(resize_call_count, 0);
            EXPECT_EQ(resize_default_init_call_count, 0);
            flare::base::string_resize_uninitialized(&rs, 237);
            EXPECT_EQ(resize_call_count, 1);
            EXPECT_EQ(resize_default_init_call_count, 0);
        }

        resize_call_count = 0;
        resize_default_init_call_count = 0;
        {
            resize_default_init_string rus;

            EXPECT_EQ(resize_call_count, 0);
            EXPECT_EQ(resize_default_init_call_count, 0);
            EXPECT_TRUE(
                    flare::base::string_supports_uninitialized_resize(&rus));
            EXPECT_EQ(resize_call_count, 0);
            EXPECT_EQ(resize_default_init_call_count, 0);
            flare::base::string_resize_uninitialized(&rus, 237);
            EXPECT_EQ(resize_call_count, 0);
            EXPECT_EQ(resize_default_init_call_count, 1);
        }
    }

}  // namespace
