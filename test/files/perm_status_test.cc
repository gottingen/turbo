
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(perm, all) {
    EXPECT_TRUE((turbo::perms::owner_read | turbo::perms::owner_write | turbo::perms::owner_exec) == turbo::perms::owner_all);
    EXPECT_TRUE((turbo::perms::group_read | turbo::perms::group_write | turbo::perms::group_exec) == turbo::perms::group_all);
    EXPECT_TRUE((turbo::perms::others_read | turbo::perms::others_write | turbo::perms::others_exec) == turbo::perms::others_all);
    EXPECT_TRUE((turbo::perms::owner_all | turbo::perms::group_all | turbo::perms::others_all) == turbo::perms::all);
    EXPECT_TRUE((turbo::perms::all | turbo::perms::set_uid | turbo::perms::set_gid | turbo::perms::sticky_bit) == turbo::perms::mask);
}

TEST(file_status, all) {
    {
        turbo::file_status fs;
        EXPECT_TRUE(fs.type() == turbo::file_type::none);
        EXPECT_TRUE(fs.permissions() == turbo::perms::unknown);
    }
    {
        turbo::file_status fs{turbo::file_type::regular};
        EXPECT_TRUE(fs.type() == turbo::file_type::regular);
        EXPECT_TRUE(fs.permissions() == turbo::perms::unknown);
    }
    {
        turbo::file_status
                fs{turbo::file_type::directory, turbo::perms::owner_read | turbo::perms::owner_write | turbo::perms::owner_exec};
        EXPECT_TRUE(fs.type() == turbo::file_type::directory);
        EXPECT_TRUE(fs.permissions() == turbo::perms::owner_all);
        fs.type(turbo::file_type::block);
        EXPECT_TRUE(fs.type() == turbo::file_type::block);
        fs.type(turbo::file_type::character);
        EXPECT_TRUE(fs.type() == turbo::file_type::character);
        fs.type(turbo::file_type::fifo);
        EXPECT_TRUE(fs.type() == turbo::file_type::fifo);
        fs.type(turbo::file_type::symlink);
        EXPECT_TRUE(fs.type()
                    == turbo::file_type::symlink);
        fs.type(turbo::file_type::socket);
        EXPECT_TRUE(fs.type() == turbo::file_type::socket);
        fs.permissions(fs.permissions()
                       | turbo::perms::group_all | turbo::perms::others_all);
        EXPECT_TRUE(fs.permissions()
                    == turbo::perms::all);
    }
    {
        turbo::file_status fst(turbo::file_type::regular);
        turbo::file_status fs(std::move(fst));
        EXPECT_TRUE(fs.type()
                    == turbo::file_type::regular);
        EXPECT_TRUE(fs.permissions()
                    == turbo::perms::unknown);
    }
}
