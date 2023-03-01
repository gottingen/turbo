//
// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/log/globals.h"

#include <string>

#include "turbo/base/log_severity.h"
#include "turbo/log/internal/globals.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/log.h"
#include "turbo/log/scoped_mock_log.h"
#include "turbo/platform/port.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

auto* test_env TURBO_ALLOW_UNUSED = ::testing::AddGlobalTestEnvironment(
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

}  // namespace
