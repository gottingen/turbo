
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <string>
#include "turbo/container/flat_hash_map.h"
#include "turbo/container/parallel_node_hash_map.h"
#include "turbo/container/parallel_flat_hash_map.h"
#include "testing/gtest_wrap.h"

TEST(CaseIgnoredFlatHashMap, all) {
    turbo::case_ignored_flat_hash_map<std::string, std::string> map;
    map["abc"] = "turbo";
    EXPECT_EQ(map["Abc"], "turbo");
    EXPECT_EQ(map["ABc"], "turbo");
    EXPECT_EQ(map["AbC"], "turbo");
    EXPECT_EQ(map["ABC"], "turbo");
    EXPECT_EQ(map["abc"], "turbo");
}