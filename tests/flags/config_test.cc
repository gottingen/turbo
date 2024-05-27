// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/flags/config.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include <gtest/gtest.h>

#ifndef TURBO_FLAGS_STRIP_NAMES
#error TURBO_FLAGS_STRIP_NAMES is not defined
#endif

#ifndef TURBO_FLAGS_STRIP_HELP
#error TURBO_FLAGS_STRIP_HELP is not defined
#endif

namespace {

// Test that TURBO_FLAGS_STRIP_NAMES and TURBO_FLAGS_STRIP_HELP are configured how
// we expect them to be configured by default. If you override this
// configuration, this test will fail, but the code should still be safe to use.
TEST(FlagsConfigTest, Test) {
#if defined(__ANDROID__)
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 1);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 1);
#elif defined(__myriad2__)
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 0);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 0);
#elif defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 1);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 1);
#elif defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 1);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 1);
#elif defined(__APPLE__)
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 0);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 0);
#elif defined(_WIN32)
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 0);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 0);
#elif defined(__linux__)
  EXPECT_EQ(TURBO_FLAGS_STRIP_NAMES, 0);
  EXPECT_EQ(TURBO_FLAGS_STRIP_HELP, 0);
#endif
}

}  // namespace
