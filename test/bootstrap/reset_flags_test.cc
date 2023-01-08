
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/bootstrap/flags.h"
#include "flare/bootstrap/bootstrap.h"
#include "gflags/gflags.h"
#include "testing/gtest_wrap.h"

DEFINE_bool(test, true, "");

FLARE_RESET_FLAGS(test, false);

namespace flare {

    TEST(OverrideFlag, All) { EXPECT_FALSE(FLAGS_test); }

}  // namespace flare::init

int main(int argc, char **argv) {
    flare::bootstrap_init(argc, argv);
    return ::RUN_ALL_TESTS();
}
