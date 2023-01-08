
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <testing/filesystem_test_util.h>
#include "flare/files/files_util.h"
#include "testing/gtest_wrap.h"
#include <deque>
#include <set>
#include <list>


struct my_container {
public:
    using value_type = flare::file_path;
public:
    my_container() = default;
    my_container(flare::file_path &&p):path(std::move(p)) {

    }

    flare::file_path path;
};

struct my_container_str {
public:
    using value_type = std::string;
public:
    my_container_str() = default;
    my_container_str(const std::string &p):path(p) {

    }
    std::string path;
};

TEST(files_util, list_directory) {
    std::error_code ec;
    // can
    //std::vector<std::string_view> files_invalud = flare::list_directory_recursive("./", ec, flare::file_and_directory());
    std::vector<my_container> files_my_c = flare::list_directory("./", ec, flare::file_and_directory());
    std::vector<my_container_str> files_my_cs = flare::list_directory("./", ec, flare::file_and_directory());
    EXPECT_EQ(files_my_c.size(), files_my_cs.size());
    std::vector<std::string> files_v = flare::list_directory("./", ec, flare::file_and_directory());
    std::vector<std::string> files_v_r = flare::list_directory_recursive("./", ec, flare::file_and_directory());
    std::deque<std::string> files_deque = flare::list_directory("./", ec, flare::only_directory());
    std::set<std::string> files_set = flare::list_directory("./", ec, flare::file_and_directory());
    std::list<std::string> files_list = flare::list_directory("./", ec, flare::only_file());
    EXPECT_GT(files_v.size(), 0UL);
    EXPECT_GT(files_v.size(), files_deque.size());
    EXPECT_LT(files_v.size(), files_v_r.size());
    EXPECT_EQ(files_v.size(), files_set.size());
    EXPECT_GT(files_v.size(), files_list.size());
}

TEST(files_util, list_directory_type) {
    std::error_code ec;
    std::vector<flare::file_path> files_v = flare::list_directory("./", ec, flare::file_and_directory());
    std::set<std::string> files_set = flare::list_directory("./", ec, flare::file_and_directory());
    EXPECT_GT(files_v.size(), 0UL);
    EXPECT_EQ(files_v.size(), files_set.size());
}

TEST(files_util, join_path_multi_container) {
    std::string prefix = "/usr";
    std::string local = "local";
    std::string lib = "lib";
    std::vector<std::string> vs{prefix, local, lib};
    std::set<std::string> ss{prefix, local, lib};
    std::set<std::string_view> sss{prefix, local, lib};

    auto vp = flare::join_path(vs);
    auto sp = flare::join_path(vs);
    EXPECT_EQ(vp, sp);
    EXPECT_EQ(vp, "/usr/local/lib");
    auto vp1 = flare::join_path("/root",vs);
    EXPECT_EQ(vp1, vp);
}

TEST(files_util, join_path_multi_container_file_path) {
    flare::file_path prefix = "/usr";
    flare::file_path local = "local";
    flare::file_path lib = "lib";
    std::vector<flare::file_path> vs{prefix, local, lib};
    std::set<flare::file_path> ss{prefix, local, lib};

    auto vp = flare::join_path(vs);
    auto sp = flare::join_path(vs);
    EXPECT_EQ(vp, sp);
    EXPECT_EQ(vp, "/usr/local/lib");
    auto vp1 = flare::join_path("/root",vs);
    EXPECT_EQ(vp1, vp);
}

//TODO(jeff) more corner case for directory_iterator