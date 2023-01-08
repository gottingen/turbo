
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "flare/files/file_watcher.h"
#include "flare/log/logging.h"

namespace {
    class FileWatcherTest : public ::testing::Test {
    protected:
        FileWatcherTest() {};

        virtual ~FileWatcherTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    /// check basic functions of flare::file_watcher
    TEST_F(FileWatcherTest, random_op) {
        srand(time(0));

        flare::file_watcher fw;
        EXPECT_EQ (0, fw.init("dummy_file"));

        for (int i = 0; i < 30; ++i) {
            if (rand() % 2) {
                const flare::file_watcher::Change ret = fw.check_and_consume();
                switch (ret) {
                    case flare::file_watcher::UPDATED:
                        FLARE_LOG(INFO) << fw.filepath() << " is updated";
                        break;
                    case flare::file_watcher::CREATED:
                        FLARE_LOG(INFO) << fw.filepath() << " is created";
                        break;
                    case flare::file_watcher::DELETED:
                        FLARE_LOG(INFO) << fw.filepath() << " is deleted";
                        break;
                    case flare::file_watcher::UNCHANGED:
                        FLARE_LOG(INFO) << fw.filepath() << " does not change or still not exist";
                        break;
                }
            }

            switch (rand() % 2) {
                case 0:
                    ASSERT_EQ(0, system("touch dummy_file"));
                    FLARE_LOG(INFO) << "action: touch dummy_file";
                    break;
                case 1:
                    ASSERT_EQ(0, system("rm -f dummy_file"));
                    FLARE_LOG(INFO) << "action: rm -f dummy_file";
                    break;
                case 2:
                    FLARE_LOG(INFO) << "action: (nothing)";
                    break;
            }

            usleep(10000);
        }
        ASSERT_EQ(0, system("rm -f dummy_file"));
    }

}  // namespace
