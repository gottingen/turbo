
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/container/cache/bucket.h"

#include "testing/gtest_wrap.h"

TEST(Bucket, TestSet) {
    flare::cache_bucket<int, int> bucket;
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        auto item = bucket.set(10, 20, 10, existing);
        EXPECT_EQ(20, item->value());
        EXPECT_EQ(1, bucket.size());
        EXPECT_EQ(nullptr, existing);
        item = bucket.get(10);
        EXPECT_EQ(20, item->value());
    }
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        auto item = bucket.set(10, 30, 10, existing);
        EXPECT_EQ(30, item->value());
        EXPECT_EQ(1, bucket.size());
        EXPECT_NE(nullptr, existing);
        EXPECT_EQ(20, existing->value());
        item = bucket.get(10);
        EXPECT_EQ(30, item->value());
    }
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        auto item = bucket.set(30, 20, 10, existing);
        EXPECT_EQ(20, item->value());
        EXPECT_EQ(2, bucket.size());
        EXPECT_EQ(nullptr, existing);
        item = bucket.get(30);
        EXPECT_EQ(20, item->value());
    }
}

TEST(Bucket, TestDel) {
    flare::cache_bucket<int, int> bucket;
    {
        auto itemDel = bucket.remove(10);
        EXPECT_EQ(nullptr, itemDel);
    }
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        bucket.set(10, 20, 10, existing);
        auto item = bucket.remove(10);
        EXPECT_EQ(20, item->value());
        EXPECT_EQ(0, bucket.size());
        item = bucket.remove(10);
        EXPECT_EQ(nullptr, item);
        EXPECT_EQ(0, bucket.size());
    }
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        bucket.set(10, 20, 10, existing);
        bucket.set(20, 30, 10, existing);
        bucket.set(30, 40, 10, existing);
        EXPECT_EQ(3, bucket.size());
        auto item = bucket.remove(10);
        EXPECT_EQ(20, item->value());
        EXPECT_EQ(2, bucket.size());
        item = bucket.remove(20);
        EXPECT_EQ(30, item->value());
        EXPECT_EQ(1, bucket.size());
        item = bucket.remove(30);
        EXPECT_EQ(40, item->value());
        EXPECT_EQ(0, bucket.size());
    }
}

TEST(Bucket, TestGet) {
    flare::cache_bucket<int, int> bucket;
    {
        auto item = bucket.get(10);
        EXPECT_EQ(nullptr, item);
    }
    {
        flare::cache_item_ptr<int, int> existing = nullptr;
        bucket.set(10, 20, 10, existing);
        auto item = bucket.get(10);
        EXPECT_EQ(20, item->value());
    }
}
