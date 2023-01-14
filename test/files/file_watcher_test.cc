
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "testing/sstream_workaround.h"
#include "testing/gtest_wrap.h"
#include "turbo/files/file_watcher.h"
#include "turbo/log/logging.h"

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

    /// check basic functions of turbo::file_watcher
    TEST_F(FileWatcherTest, random_op) {
        srand(time(0));

        turbo::file_watcher fw;
        EXPECT_EQ (0, fw.init("dummy_file"));

        for (int i = 0; i < 30; ++i) {
            if (rand() % 2) {
                const turbo::file_watcher::Change ret = fw.check_and_consume();
                switch (ret) {
                    case turbo::file_watcher::UPDATED:
                        TURBO_LOG(INFO) << fw.filepath() << " is updated";
                        break;
                    case turbo::file_watcher::CREATED:
                        TURBO_LOG(INFO) << fw.filepath() << " is created";
                        break;
                    case turbo::file_watcher::DELETED:
                        TURBO_LOG(INFO) << fw.filepath() << " is deleted";
                        break;
                    case turbo::file_watcher::UNCHANGED:
                        TURBO_LOG(INFO) << fw.filepath() << " does not change or still not exist";
                        break;
                }
            }

            switch (rand() % 2) {
                case 0:
                    ASSERT_EQ(0, system("touch dummy_file"));
                    TURBO_LOG(INFO) << "action: touch dummy_file";
                    break;
                case 1:
                    ASSERT_EQ(0, system("rm -f dummy_file"));
                    TURBO_LOG(INFO) << "action: rm -f dummy_file";
                    break;
                case 2:
                    TURBO_LOG(INFO) << "action: (nothing)";
                    break;
            }

            usleep(10000);
        }
        ASSERT_EQ(0, system("rm -f dummy_file"));
    }

}  // namespace
