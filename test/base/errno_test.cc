
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "flare/base/errno.h"

class ErrnoTest : public ::testing::Test {
protected:
    ErrnoTest() {
    };

    virtual ~ErrnoTest() {};

    virtual void SetUp() {
    };

    virtual void TearDown() {
    };
};

#define ESTOP -114
#define EBREAK -115
#define ESTH -116
#define EOK -117
#define EMYERROR -30

FLARE_REGISTER_ERRNO(ESTOP, "the thread is stopping")

FLARE_REGISTER_ERRNO(EBREAK, "the thread is interrupted")

FLARE_REGISTER_ERRNO(ESTH, "something happened")

FLARE_REGISTER_ERRNO(EOK, "OK!")

FLARE_REGISTER_ERRNO(EMYERROR, "my error");

TEST_F(ErrnoTest, system_errno) {
    errno = EPIPE;
    ASSERT_STREQ("Broken pipe", flare_error());
    ASSERT_STREQ("Interrupted system call", flare_error(EINTR));
}

TEST_F(ErrnoTest, customized_errno) {
    ASSERT_STREQ("the thread is stopping", flare_error(ESTOP));
    ASSERT_STREQ("the thread is interrupted", flare_error(EBREAK));
    ASSERT_STREQ("something happened", flare_error(ESTH));
    ASSERT_STREQ("OK!", flare_error(EOK));
    ASSERT_STREQ("Unknown error 1000", flare_error(1000));

    errno = ESTOP;
    printf("Something got wrong, %m\n");
    printf("Something got wrong, %s\n", flare_error());
}
