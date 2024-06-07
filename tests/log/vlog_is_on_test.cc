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

#include <turbo/log/vlog_is_on.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/log_severity.h>
#include <turbo/flags/flag.h>
#include <turbo/log/flags.h>
#include <turbo/log/globals.h>
#include <turbo/log/log.h>
#include <tests/log/scoped_mock_log.h>
#include <optional>

namespace {

using ::testing::_;

std::optional<int> MaxLogVerbosity() {
#ifdef TURBO_MAX_VLOG_VERBOSITY
  return TURBO_MAX_VLOG_VERBOSITY;
#else
  return std::nullopt;
#endif
}

std::optional<int> min_log_level() {
#ifdef TURBO_MIN_LOG_LEVEL
  return static_cast<int>(TURBO_MIN_LOG_LEVEL);
#else
  return std::nullopt;
#endif
}

TEST(VLogIsOn, GlobalWorksWithoutMaxVerbosityAndMinLogLevel) {
  if (MaxLogVerbosity().has_value() || min_log_level().has_value()) {
    GTEST_SKIP();
  }

  turbo::set_global_vlog_level(3);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "important"));

  log.StartCapturingLogs();
  VLOG(3) << "important";
  VLOG(4) << "spam";
}

TEST(VLogIsOn, FileWorksWithoutMaxVerbosityAndMinLogLevel) {
  if (MaxLogVerbosity().has_value() || min_log_level().has_value()) {
    GTEST_SKIP();
  }

  turbo::set_vlog_level("vlog_is_on_test", 3);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "important"));

  log.StartCapturingLogs();
  VLOG(3) << "important";
  VLOG(4) << "spam";
}

TEST(VLogIsOn, PatternWorksWithoutMaxVerbosityAndMinLogLevel) {
  if (MaxLogVerbosity().has_value() || min_log_level().has_value()) {
    GTEST_SKIP();
  }

  turbo::set_vlog_level("vlog_is_on*", 3);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "important"));

  log.StartCapturingLogs();
  VLOG(3) << "important";
  VLOG(4) << "spam";
}

TEST(VLogIsOn, GlobalDoesNotFilterBelowMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() < 2) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_global_vlog_level(1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "asdf"));

  log.StartCapturingLogs();
  VLOG(2) << "asdf";
}

TEST(VLogIsOn, FileDoesNotFilterBelowMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() < 2) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_vlog_level("vlog_is_on_test", 1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "asdf"));

  log.StartCapturingLogs();
  VLOG(2) << "asdf";
}

TEST(VLogIsOn, PatternDoesNotFilterBelowMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() < 2) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_vlog_level("vlog_is_on*", 1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "asdf"));

  log.StartCapturingLogs();
  VLOG(2) << "asdf";
}

TEST(VLogIsOn, GlobalFiltersAboveMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() >= 4) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_global_vlog_level(1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  log.StartCapturingLogs();
  VLOG(4) << "dfgh";
}

TEST(VLogIsOn, FileFiltersAboveMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() >= 4) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_vlog_level("vlog_is_on_test", 1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  log.StartCapturingLogs();
  VLOG(4) << "dfgh";
}

TEST(VLogIsOn, PatternFiltersAboveMaxVerbosity) {
  if (!MaxLogVerbosity().has_value() || *MaxLogVerbosity() >= 4) {
    GTEST_SKIP();
  }

  // Set an arbitrary high value to avoid filtering VLOGs in tests by default.
  turbo::set_vlog_level("vlog_is_on*", 1000);
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  log.StartCapturingLogs();
  VLOG(4) << "dfgh";
}

}  // namespace
