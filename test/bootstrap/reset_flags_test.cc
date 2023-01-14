
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/bootstrap/flags.h"
#include "turbo/bootstrap/bootstrap.h"
#include "gflags/gflags.h"
#include "testing/gtest_wrap.h"

DEFINE_bool(test, true, "");

TURBO_RESET_FLAGS(test, false);

namespace turbo {

    TEST(OverrideFlag, All) { EXPECT_FALSE(FLAGS_test); }

}  // namespace turbo::init

int main(int argc, char **argv) {
    turbo::bootstrap_init(argc, argv);
    return ::RUN_ALL_TESTS();
}
