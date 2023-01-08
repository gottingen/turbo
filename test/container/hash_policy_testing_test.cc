
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/container/flat_hash_map.h"
#include "hash_policy_testing.h"

#include "testing/gtest_wrap.h"

namespace flare {
    namespace priv {
        namespace {

            TEST(_, Hash) {
                StatefulTestingHash h1;
                EXPECT_EQ(1, h1.id());
                StatefulTestingHash h2;
                EXPECT_EQ(2, h2.id());
                StatefulTestingHash h1c(h1);
                EXPECT_EQ(1, h1c.id());
                StatefulTestingHash h2m(std::move(h2));
                EXPECT_EQ(2, h2m.id());
                EXPECT_EQ(0, h2.id());
                StatefulTestingHash h3;
                EXPECT_EQ(3, h3.id());
                h3 = StatefulTestingHash();
                EXPECT_EQ(4, h3.id());
                h3 = std::move(h1);
                EXPECT_EQ(1, h3.id());
            }

        }  // namespace
    }  // namespace priv
}  // namespace flare
