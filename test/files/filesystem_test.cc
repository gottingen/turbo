
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "testing/sstream_workaround.h"
#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(TemporaryDirectory, fsTestTmpdir) {
    turbo::file_path tempPath;
    {
        TemporaryDirectory t;
        tempPath = t.path();
        EXPECT_TRUE(turbo::exists(turbo::file_path(t.path()))
        );
        EXPECT_TRUE(turbo::is_directory(t.path())
        );
    }
    EXPECT_TRUE(!
                        turbo::exists(tempPath)
    );
}
