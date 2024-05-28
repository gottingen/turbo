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

#include <stdlib.h>

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/globals.h>
#include <tests/log/test_helpers.h>
#include <turbo/log/log.h>

namespace {
using ::testing::AllOf;
using ::testing::HasSubstr;

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

MATCHER_P2(HasSubstrTimes, substr, expected_count, "") {
  int count = 0;
  std::string::size_type pos = 0;
  std::string needle(substr);
  while ((pos = arg.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }

  return count == expected_count;
}

TEST(StderrLogSinkDeathTest, InfoMessagesInStderr) {
  EXPECT_DEATH_IF_SUPPORTED(
      {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfo);
        LOG(INFO) << "INFO message";
        exit(1);
      },
      "INFO message");
}

TEST(StderrLogSinkDeathTest, WarningMessagesInStderr) {
  EXPECT_DEATH_IF_SUPPORTED(
      {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfo);
        LOG(WARNING) << "WARNING message";
        exit(1);
      },
      "WARNING message");
}

TEST(StderrLogSinkDeathTest, ErrorMessagesInStderr) {
  EXPECT_DEATH_IF_SUPPORTED(
      {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfo);
        LOG(ERROR) << "ERROR message";
        exit(1);
      },
      "ERROR message");
}

TEST(StderrLogSinkDeathTest, FatalMessagesInStderr) {
  char message[] = "FATAL message";
  char stacktrace[] = "*** Check failure stack trace: ***";

  int expected_count = 1;

  EXPECT_DEATH_IF_SUPPORTED(
      {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfo);
        LOG(FATAL) << message;
      },
      AllOf(HasSubstrTimes(message, expected_count), HasSubstr(stacktrace)));
}

TEST(StderrLogSinkDeathTest, SecondaryFatalMessagesInStderr) {
  auto MessageGen = []() -> std::string {
    LOG(FATAL) << "Internal failure";
    return "External failure";
  };

  EXPECT_DEATH_IF_SUPPORTED(
      {
        turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfo);
        LOG(FATAL) << MessageGen();
      },
      "Internal failure");
}

}  // namespace
