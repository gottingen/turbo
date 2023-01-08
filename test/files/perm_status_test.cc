
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(perm, all) {
    EXPECT_TRUE((flare::perms::owner_read | flare::perms::owner_write | flare::perms::owner_exec) == flare::perms::owner_all);
    EXPECT_TRUE((flare::perms::group_read | flare::perms::group_write | flare::perms::group_exec) == flare::perms::group_all);
    EXPECT_TRUE((flare::perms::others_read | flare::perms::others_write | flare::perms::others_exec) == flare::perms::others_all);
    EXPECT_TRUE((flare::perms::owner_all | flare::perms::group_all | flare::perms::others_all) == flare::perms::all);
    EXPECT_TRUE((flare::perms::all | flare::perms::set_uid | flare::perms::set_gid | flare::perms::sticky_bit) == flare::perms::mask);
}

TEST(file_status, all) {
    {
        flare::file_status fs;
        EXPECT_TRUE(fs.type() == flare::file_type::none);
        EXPECT_TRUE(fs.permissions() == flare::perms::unknown);
    }
    {
        flare::file_status fs{flare::file_type::regular};
        EXPECT_TRUE(fs.type() == flare::file_type::regular);
        EXPECT_TRUE(fs.permissions() == flare::perms::unknown);
    }
    {
        flare::file_status
                fs{flare::file_type::directory, flare::perms::owner_read | flare::perms::owner_write | flare::perms::owner_exec};
        EXPECT_TRUE(fs.type() == flare::file_type::directory);
        EXPECT_TRUE(fs.permissions() == flare::perms::owner_all);
        fs.type(flare::file_type::block);
        EXPECT_TRUE(fs.type() == flare::file_type::block);
        fs.type(flare::file_type::character);
        EXPECT_TRUE(fs.type() == flare::file_type::character);
        fs.type(flare::file_type::fifo);
        EXPECT_TRUE(fs.type() == flare::file_type::fifo);
        fs.type(flare::file_type::symlink);
        EXPECT_TRUE(fs.type()
                    == flare::file_type::symlink);
        fs.type(flare::file_type::socket);
        EXPECT_TRUE(fs.type() == flare::file_type::socket);
        fs.permissions(fs.permissions()
                       | flare::perms::group_all | flare::perms::others_all);
        EXPECT_TRUE(fs.permissions()
                    == flare::perms::all);
    }
    {
        flare::file_status fst(flare::file_type::regular);
        flare::file_status fs(std::move(fst));
        EXPECT_TRUE(fs.type()
                    == flare::file_type::regular);
        EXPECT_TRUE(fs.permissions()
                    == flare::perms::unknown);
    }
}
