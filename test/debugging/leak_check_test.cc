
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include <string>

#include "testing/gtest_wrap.h"
#include "turbo/log/logging.h"
#include "turbo/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, DetectLeakSanitizer) {
#ifdef TURBO_EXPECT_LEAK_SANITIZER
        EXPECT_TRUE(turbo::debugging::have_leak_sanitizer());
#else
        EXPECT_FALSE(turbo::debugging::have_leak_sanitizer());
#endif
    }

    TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
        auto foo = turbo::debugging::ignore_leak(new std::string("some ignored leaked string"));
        TURBO_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

    TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
        turbo::debugging::leak_check_disabler disabler;
        auto foo = new std::string("some std::string leaked while checks are disabled");
        TURBO_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

}  // namespace
