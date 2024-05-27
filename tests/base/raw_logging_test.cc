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

// This test serves primarily as a compilation test for base/raw_logging.h.
// Raw logging testing is covered by logging_unittest.cc, which is not as
// portable as this test.

#include <turbo/base/internal/raw_logging.h>

#include <tuple>

#include <gtest/gtest.h>
#include <turbo/strings/str_cat.h>

namespace {

TEST(RawLoggingCompilationTest, Log) {
  TURBO_RAW_LOG(INFO, "RAW INFO: %d", 1);
  TURBO_RAW_LOG(INFO, "RAW INFO: %d %d", 1, 2);
  TURBO_RAW_LOG(INFO, "RAW INFO: %d %d %d", 1, 2, 3);
  TURBO_RAW_LOG(INFO, "RAW INFO: %d %d %d %d", 1, 2, 3, 4);
  TURBO_RAW_LOG(INFO, "RAW INFO: %d %d %d %d %d", 1, 2, 3, 4, 5);
  TURBO_RAW_LOG(WARNING, "RAW WARNING: %d", 1);
  TURBO_RAW_LOG(ERROR, "RAW ERROR: %d", 1);
}

TEST(RawLoggingCompilationTest, PassingCheck) {
  TURBO_RAW_CHECK(true, "RAW CHECK");
}

// Not all platforms support output from raw log, so we don't verify any
// particular output for RAW check failures (expecting the empty string
// accomplishes this).  This test is primarily a compilation test, but we
// are verifying process death when EXPECT_DEATH works for a platform.
const char kExpectedDeathOutput[] = "";

TEST(RawLoggingDeathTest, FailingCheck) {
  EXPECT_DEATH_IF_SUPPORTED(TURBO_RAW_CHECK(1 == 0, "explanation"),
                            kExpectedDeathOutput);
}

TEST(RawLoggingDeathTest, LogFatal) {
  EXPECT_DEATH_IF_SUPPORTED(TURBO_RAW_LOG(FATAL, "my dog has fleas"),
                            kExpectedDeathOutput);
}

TEST(InternalLog, CompilationTest) {
  TURBO_INTERNAL_LOG(INFO, "Internal Log");
  std::string log_msg = "Internal Log";
  TURBO_INTERNAL_LOG(INFO, log_msg);

  TURBO_INTERNAL_LOG(INFO, log_msg + " 2");

  float d = 1.1f;
  TURBO_INTERNAL_LOG(INFO, turbo::StrCat("Internal log ", 3, " + ", d));
}

TEST(InternalLogDeathTest, FailingCheck) {
  EXPECT_DEATH_IF_SUPPORTED(TURBO_INTERNAL_CHECK(1 == 0, "explanation"),
                            kExpectedDeathOutput);
}

TEST(InternalLogDeathTest, LogFatal) {
  EXPECT_DEATH_IF_SUPPORTED(TURBO_INTERNAL_LOG(FATAL, "my dog has fleas"),
                            kExpectedDeathOutput);
}

}  // namespace
