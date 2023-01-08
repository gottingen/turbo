
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/base/base64.h"

#include "testing/gtest_wrap.h"

namespace flare::base {

    TEST(Base64Test, Basic) {
        const std::string kText = "hello world";
        const std::string kBase64Text = "aGVsbG8gd29ybGQ=";

        std::string encoded;
        std::string decoded;
        bool ok;

        base64_encode(kText, &encoded);
        EXPECT_EQ(kBase64Text, encoded);

        ok = base64_decode(encoded, &decoded);
        EXPECT_TRUE(ok);
        EXPECT_EQ(kText, decoded);
    }

}  // namespace namespace flare::base
