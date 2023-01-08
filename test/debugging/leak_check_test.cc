
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include <string>

#include "testing/gtest_wrap.h"
#include "flare/log/logging.h"
#include "flare/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, DetectLeakSanitizer) {
#ifdef FLARE_EXPECT_LEAK_SANITIZER
        EXPECT_TRUE(flare::debugging::have_leak_sanitizer());
#else
        EXPECT_FALSE(flare::debugging::have_leak_sanitizer());
#endif
    }

    TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
        auto foo = flare::debugging::ignore_leak(new std::string("some ignored leaked string"));
        FLARE_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

    TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
        flare::debugging::leak_check_disabler disabler;
        auto foo = new std::string("some std::string leaked while checks are disabled");
        FLARE_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

}  // namespace
