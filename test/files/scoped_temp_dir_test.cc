
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <string>
#include "flare/files/scoped_temp_dir.h"
#include "testing/gtest_wrap.h"

namespace flare {

    TEST(scoped_temp_dir, FullPath) {
        flare::file_path test_path;
        EXPECT_TRUE(create_new_temp_directory("scoped_temp_dir",
                                  &test_path))<<test_path;

        // Against an existing dir, it should get destroyed when leaving scope.
        EXPECT_TRUE(flare::exists(test_path));
        {
            scoped_temp_dir dir;
            EXPECT_TRUE(dir.set(test_path));
            EXPECT_TRUE(dir.is_valid());
        }
        EXPECT_FALSE(flare::exists(test_path));

        {
            scoped_temp_dir dir;
            EXPECT_TRUE(dir.set(test_path));
            // Now the dir doesn't exist, so ensure that it gets created.
            EXPECT_TRUE(flare::exists(test_path));
            // When we call Release(), it shouldn't get destroyed when leaving scope.
            flare::file_path path = dir.take();
            EXPECT_EQ(path, test_path);
            EXPECT_FALSE(dir.is_valid());
        }
        EXPECT_TRUE(flare::exists(test_path));

        // Clean up.
        {
            scoped_temp_dir dir;
            EXPECT_TRUE(dir.set(test_path));
        }
        EXPECT_FALSE(flare::exists(test_path));
    }

    TEST(scoped_temp_dir, TempDir) {
        // In this case, just verify that a directory was created and that it's a
        // child of TempDir.
        flare::file_path test_path;
        {
            scoped_temp_dir dir;
            EXPECT_TRUE(dir.create_unique_temp_dir());
            test_path = dir.path();
            EXPECT_TRUE(flare::exists(test_path));
            flare::file_path tmp_dir = flare::temp_directory_path();
            EXPECT_TRUE(test_path.generic_string().find(tmp_dir.generic_string()) != std::string::npos);
        }
        EXPECT_FALSE(flare::exists(test_path));
    }

    TEST(scoped_temp_dir, UniqueTempDirUnderPath) {
        // Create a path which will contain a unique temp path.
        flare::file_path base_path;
        ASSERT_TRUE(create_new_temp_directory("base_dir",
                                              &base_path));

        flare::file_path test_path;
        {
            scoped_temp_dir dir;
            EXPECT_TRUE(dir.create_unique_temp_dir_under_path(base_path));
            test_path = dir.path();
            EXPECT_TRUE(flare::exists(test_path));
            EXPECT_TRUE(test_path.generic_string().find(base_path.generic_string()) != std::string::npos);
        }
        EXPECT_FALSE(flare::exists(test_path));
        flare::remove_all(base_path);
    }

    TEST(scoped_temp_dir, MultipleInvocations) {
        scoped_temp_dir dir;
        EXPECT_TRUE(dir.create_unique_temp_dir());
        EXPECT_FALSE(dir.create_unique_temp_dir());
        EXPECT_TRUE(dir.remove());
        EXPECT_TRUE(dir.create_unique_temp_dir());
        EXPECT_FALSE(dir.create_unique_temp_dir());
        scoped_temp_dir other_dir;
        EXPECT_TRUE(other_dir.set(dir.take()));
        EXPECT_TRUE(dir.create_unique_temp_dir());
        EXPECT_FALSE(dir.create_unique_temp_dir());
        EXPECT_FALSE(other_dir.create_unique_temp_dir());
    }


}  // namespace flare
