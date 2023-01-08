
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include <memory>
#include "testing/gtest_wrap.h"
#include "flare/log/logging.h"
#include "flare/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, LeakMemory) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by leak_check_test.sh, which will verify a
        // failed exit code.

        char *foo = strdup("lsan should complain about this leaked string");
        FLARE_LOG(INFO) << "Should detect leaked std::string " << foo;
    }

    TEST(LeakCheckTest, LeakMemoryAfterDisablerScope) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by external_leak_check_test.sh, which will verify a
        // failed exit code.
        { flare::debugging::leak_check_disabler disabler; }
        char *foo = strdup("lsan should also complain about this leaked string");
        FLARE_LOG(INFO) << "Re-enabled leak detection.Should detect leaked std::string " << foo;
    }

}  // namespace
