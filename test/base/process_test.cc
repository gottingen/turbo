
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/system/process.h"
#include "flare/base/errno.h"
#include "flare/strings/ends_with.h"
#include "testing/gtest_wrap.h"

namespace flare {
    extern int read_command_output_through_clone(std::ostream &, const char *);

    extern int read_command_output_through_popen(std::ostream &, const char *);
}

namespace {

    class PopenTest : public testing::Test {
    };

    TEST(PopenTest, posix_popen) {
        std::ostringstream oss;
        int rc = flare::read_command_output_through_popen(oss, "echo \"Hello World\"");
        ASSERT_EQ(0, rc) << flare_error(errno);
        ASSERT_EQ("Hello World\n", oss.str());

        oss.str("");
        rc = flare::read_command_output_through_popen(oss, "exit 1");
        EXPECT_EQ(1, rc) << flare_error(errno);
        ASSERT_TRUE(oss.str().empty()) << oss.str();
        oss.str("");
        rc = flare::read_command_output_through_popen(oss, "kill -9 $$");
        ASSERT_EQ(-1, rc);
        ASSERT_EQ(errno, ECHILD);
        ASSERT_TRUE(flare::ends_with(oss.str(), "was killed by signal 9"));
        oss.str("");
        rc = flare::read_command_output_through_popen(oss, "kill -15 $$");
        ASSERT_EQ(-1, rc);
        ASSERT_EQ(errno, ECHILD);
        ASSERT_TRUE(flare::ends_with(oss.str(), "was killed by signal 15"));
    }

#if defined(FLARE_PLATFORM_LINUX)

    TEST(PopenTest, clone) {
        std::ostringstream oss;
        int rc = flare::read_command_output_through_clone(oss, "echo \"Hello World\"");
        ASSERT_EQ(0, rc) << flare_error(errno);
        ASSERT_EQ("Hello World\n", oss.str());

        oss.str("");
        rc = flare::read_command_output_through_clone(oss, "exit 1");
        ASSERT_EQ(1, rc) << flare_error(errno);
        ASSERT_TRUE(oss.str().empty()) << oss.str();
        oss.str("");
        rc = flare::read_command_output_through_clone(oss, "kill -9 $$");
        ASSERT_EQ(-1, rc);
        ASSERT_EQ(errno, ECHILD);
        ASSERT_TRUE(flare::ends_with(oss.str(), "was killed by signal 9"));
        oss.str("");
        rc = flare::read_command_output_through_clone(oss, "kill -15 $$");
        ASSERT_EQ(-1, rc);
        ASSERT_EQ(errno, ECHILD);
        ASSERT_TRUE(flare::ends_with(oss.str(), "was killed by signal 15"));

        oss.str("");
        ASSERT_EQ(0, flare::read_command_output_through_clone(oss, "for i in `seq 1 100000`; do echo -n '=' ; done"));
        ASSERT_EQ(100000u, oss.str().length());
        std::string expected;
        expected.resize(100000, '=');
        ASSERT_EQ(expected, oss.str());
    }

    struct CounterArg {
        volatile int64_t counter;
        volatile bool stop;
    };

    static void* counter_thread(void* args) {
        CounterArg* ca = (CounterArg*)args;
        while (!ca->stop) {
            ++ca->counter;
        }
        return nullptr;
    }

    static int fork_thread(void* arg) {
        usleep(100 * 1000);
        _exit(0);
    }

    const int CHILD_STACK_SIZE = 64 * 1024;

    TEST(PopenTest, does_vfork_suspend_all_threads) {
        pthread_t tid;
        CounterArg ca = { 0 , false };
        ASSERT_EQ(0, pthread_create(&tid, nullptr, counter_thread, &ca));
        usleep(100 * 1000);
        char* child_stack_mem = (char*)malloc(CHILD_STACK_SIZE);
        void* child_stack = child_stack_mem + CHILD_STACK_SIZE;
        const int64_t counter_before_fork = ca.counter;
        pid_t cpid = clone(fork_thread, child_stack, CLONE_VFORK, nullptr);
        const int64_t counter_after_fork = ca.counter;
        usleep(100 * 1000);
        const int64_t counter_after_sleep = ca.counter;
        int ws;
        ca.stop = true;
        pthread_join(tid, nullptr);
        std::cout << "bc=" << counter_before_fork << " ac=" << counter_after_fork
                  << " as=" << counter_after_sleep
                  << std::endl;
        ASSERT_EQ(cpid, waitpid(cpid, &ws, __WALL));
    }

#endif  // FLARE_PLATFORM_LINUX

}
