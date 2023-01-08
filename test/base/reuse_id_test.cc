
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "flare/base/reuse_id.h"

#include "testing/gtest_wrap.h"

struct fd_tag;
struct fd_tag1;

TEST(reuse_id, different_tag) {

    auto id = flare::reuse_id<size_t, fd_tag>::instance();
    auto id1 = flare::reuse_id<size_t, fd_tag1>::instance();
    EXPECT_EQ(0UL, id->next());
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(2UL, id->next());
    EXPECT_EQ(0UL, id1->next());
    id->free(1);
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(1UL, id1->next());

}

struct same_tag{};

TEST(reuse_id, different_type) {

    auto id = flare::reuse_id<size_t, same_tag>::instance();
    auto id1 = flare::reuse_id<uint32_t, same_tag>::instance();
    EXPECT_EQ(0UL, id->next());
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(2UL, id->next());
    EXPECT_EQ(0UL, id1->next());
    id->free(1);
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(1UL, id1->next());

}

struct diff_max_tag{};

TEST(reuse_id, different_max) {

    auto id = flare::reuse_id<size_t, diff_max_tag, 100>::instance();
    auto id1 = flare::reuse_id<size_t, diff_max_tag, 200>::instance();
    EXPECT_EQ(0UL, id->next());
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(2UL, id->next());
    EXPECT_EQ(0UL, id1->next());
    id->free(1);
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(1UL, id1->next());

}

TEST(reuse_id, max) {

    auto id = flare::reuse_id<size_t, fd_tag, 100>::instance();
    for(int i = 0; i < 100; i++) {
        id->next();
    }
    EXPECT_EQ(100UL, id->next());
    EXPECT_EQ(100UL, id->next());

    for(int i = 99; i > 50; i--) {
        id->free(i);
    }
    EXPECT_EQ(51UL, id->next());

    for(int i = 0; i < 100; i++) {
        id->next();
    }

    for(int i = 50; i < 99; i++) {
        id->free(i);
    }
    EXPECT_EQ(98UL, id->next());
    EXPECT_EQ(false, id->free(100));
    EXPECT_EQ(false, id->free(110));
}