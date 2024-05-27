//
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

#include <turbo/log/globals.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/globals.h>
#include <tests/log/test_helpers.h>
#include <turbo/log/log.h>
#include <tests/log/scoped_mock_log.h>

namespace {
using ::testing::_;
using ::testing::StrEq;

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

constexpr static turbo::LogSeverityAtLeast DefaultMinLogLevel() {
  return turbo::LogSeverityAtLeast::kInfo;
}
constexpr static turbo::LogSeverityAtLeast DefaultStderrThreshold() {
  return turbo::LogSeverityAtLeast::kError;
}

TEST(TestGlobals, MinLogLevel) {
  EXPECT_EQ(turbo::MinLogLevel(), DefaultMinLogLevel());
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kError);
  EXPECT_EQ(turbo::MinLogLevel(), turbo::LogSeverityAtLeast::kError);
  turbo::SetMinLogLevel(DefaultMinLogLevel());
}

TEST(TestGlobals, ScopedMinLogLevel) {
  EXPECT_EQ(turbo::MinLogLevel(), DefaultMinLogLevel());
  {
    turbo::log_internal::ScopedMinLogLevel scoped_stderr_threshold(
        turbo::LogSeverityAtLeast::kError);
    EXPECT_EQ(turbo::MinLogLevel(), turbo::LogSeverityAtLeast::kError);
  }
  EXPECT_EQ(turbo::MinLogLevel(), DefaultMinLogLevel());
}

TEST(TestGlobals, StderrThreshold) {
  EXPECT_EQ(turbo::StderrThreshold(), DefaultStderrThreshold());
  turbo::SetStderrThreshold(turbo::LogSeverityAtLeast::kError);
  EXPECT_EQ(turbo::StderrThreshold(), turbo::LogSeverityAtLeast::kError);
  turbo::SetStderrThreshold(DefaultStderrThreshold());
}

TEST(TestGlobals, ScopedStderrThreshold) {
  EXPECT_EQ(turbo::StderrThreshold(), DefaultStderrThreshold());
  {
    turbo::ScopedStderrThreshold scoped_stderr_threshold(
        turbo::LogSeverityAtLeast::kError);
    EXPECT_EQ(turbo::StderrThreshold(), turbo::LogSeverityAtLeast::kError);
  }
  EXPECT_EQ(turbo::StderrThreshold(), DefaultStderrThreshold());
}

TEST(TestGlobals, LogBacktraceAt) {
  EXPECT_FALSE(turbo::log_internal::ShouldLogBacktraceAt("some_file.cc", 111));
  turbo::SetLogBacktraceLocation("some_file.cc", 111);
  EXPECT_TRUE(turbo::log_internal::ShouldLogBacktraceAt("some_file.cc", 111));
  EXPECT_FALSE(
      turbo::log_internal::ShouldLogBacktraceAt("another_file.cc", 222));
}

TEST(TestGlobals, LogPrefix) {
  EXPECT_TRUE(turbo::ShouldPrependLogPrefix());
  turbo::EnableLogPrefix(false);
  EXPECT_FALSE(turbo::ShouldPrependLogPrefix());
  turbo::EnableLogPrefix(true);
  EXPECT_TRUE(turbo::ShouldPrependLogPrefix());
}

TEST(TestGlobals, SetGlobalVLogLevel) {
  EXPECT_EQ(turbo::SetGlobalVLogLevel(42), 0);
  EXPECT_EQ(turbo::SetGlobalVLogLevel(1337), 42);
  // Restore the value since it affects the default unset module value for
  // `SetVLogLevel()`.
  EXPECT_EQ(turbo::SetGlobalVLogLevel(0), 1337);
}

TEST(TestGlobals, SetVLogLevel) {
  EXPECT_EQ(turbo::SetVLogLevel("setvloglevel", 42), 0);
  EXPECT_EQ(turbo::SetVLogLevel("setvloglevel", 1337), 42);
  EXPECT_EQ(turbo::SetVLogLevel("othersetvloglevel", 50), 0);
}

TEST(TestGlobals, AndroidLogTag) {
  // Verify invalid tags result in a check failure.
  EXPECT_DEATH_IF_SUPPORTED(turbo::SetAndroidNativeTag(nullptr), ".*");

  // Verify valid tags applied.
  EXPECT_THAT(turbo::log_internal::GetAndroidNativeTag(), StrEq("native"));
  turbo::SetAndroidNativeTag("test_tag");
  EXPECT_THAT(turbo::log_internal::GetAndroidNativeTag(), StrEq("test_tag"));

  // Verify that additional calls (more than 1) result in a check failure.
  EXPECT_DEATH_IF_SUPPORTED(turbo::SetAndroidNativeTag("test_tag_fail"), ".*");
}

TEST(TestExitOnDFatal, OffTest) {
  // Turn off...
  turbo::log_internal::SetExitOnDFatal(false);
  EXPECT_FALSE(turbo::log_internal::ExitOnDFatal());

  // We don't die.
  {
    turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

    // LOG(DFATAL) has severity FATAL if debugging, but is
    // downgraded to ERROR if not debugging.
    EXPECT_CALL(log, Log(turbo::kLogDebugFatal, _, "This should not be fatal"));

    log.StartCapturingLogs();
    LOG(DFATAL) << "This should not be fatal";
  }
}

#if GTEST_HAS_DEATH_TEST
TEST(TestDeathWhileExitOnDFatal, OnTest) {
  turbo::log_internal::SetExitOnDFatal(true);
  EXPECT_TRUE(turbo::log_internal::ExitOnDFatal());

  // Death comes on little cats' feet.
  EXPECT_DEBUG_DEATH({ LOG(DFATAL) << "This should be fatal in debug mode"; },
                     "This should be fatal in debug mode");
}
#endif

}  // namespace
