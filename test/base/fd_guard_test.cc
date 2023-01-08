
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <sys/types.h>                          // open
#include <sys/stat.h>                           // ^
#include <fcntl.h>                              // ^
#include "testing/gtest_wrap.h"
#include <errno.h>
#include "flare/base/fd_guard.h"

namespace {

    class FDGuardTest : public ::testing::Test {
    protected:
        FDGuardTest() {
        };

        virtual ~FDGuardTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    TEST_F(FDGuardTest, default_constructor) {
        flare::base::fd_guard guard;
        ASSERT_EQ(-1, guard);
    }

    TEST_F(FDGuardTest, destructor_closes_fd) {
        int fd = -1;
        {
            flare::base::fd_guard guard(open(".tmp1", O_WRONLY | O_CREAT, 0600));
            ASSERT_GT(guard, 0);
            fd = guard;
        }
        char dummy = 0;
        ASSERT_EQ(-1L, write(fd, &dummy, 1));
        ASSERT_EQ(EBADF, errno);
    }

    TEST_F(FDGuardTest, reset_closes_previous_fd) {
        flare::base::fd_guard guard(open(".tmp1", O_WRONLY | O_CREAT, 0600));
        ASSERT_GT(guard, 0);
        const int fd = guard;
        const int fd2 = open(".tmp2", O_WRONLY | O_CREAT, 0600);
        guard.reset(fd2);
        char dummy = 0;
        ASSERT_EQ(-1L, write(fd, &dummy, 1));
        ASSERT_EQ(EBADF, errno);
        guard.reset(-1);
        ASSERT_EQ(-1L, write(fd2, &dummy, 1));
        ASSERT_EQ(EBADF, errno);
    }

    TEST_F(FDGuardTest, release) {
        flare::base::fd_guard guard(open(".tmp1", O_WRONLY | O_CREAT, 0600));
        ASSERT_GT(guard, 0);
        const int fd = guard;
        ASSERT_EQ(fd, guard.release());
        ASSERT_EQ(-1, guard);
        close(fd);
    }
}
