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

#include "turbo/log/internal/flags.h"

#include <string>

#include "turbo/base/log_severity.h"
#include "turbo/flags/flag.h"
#include "turbo/flags/reflection.h"
#include "turbo/log/globals.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/internal/test_matchers.h"
#include "turbo/log/log.h"
#include "turbo/log/scoped_mock_log.h"
#include "turbo/platform/port.h"
#include "turbo/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using ::turbo::log_internal::TextMessage;

using ::testing::HasSubstr;
using ::testing::Not;

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

constexpr static turbo::LogSeverityAtLeast DefaultStderrThreshold() {
  return turbo::LogSeverityAtLeast::kError;
}

class LogFlagsTest : public ::testing::Test {
 protected:
  turbo::FlagSaver flag_saver_;
};

// This test is disabled because it adds order dependency to the test suite.
// This order dependency is currently not fixable due to the way the
// stderrthreshold global value is out of sync with the stderrthreshold flag.
TEST_F(LogFlagsTest, DISABLED_StderrKnobsDefault) {
  EXPECT_EQ(turbo::StderrThreshold(), DefaultStderrThreshold());
}

TEST_F(LogFlagsTest, SetStderrThreshold) {
  turbo::SetFlag(&FLAGS_stderrthreshold,
                static_cast<int>(turbo::LogSeverityAtLeast::kInfo));

  EXPECT_EQ(turbo::StderrThreshold(), turbo::LogSeverityAtLeast::kInfo);

  turbo::SetFlag(&FLAGS_stderrthreshold,
                static_cast<int>(turbo::LogSeverityAtLeast::kError));

  EXPECT_EQ(turbo::StderrThreshold(), turbo::LogSeverityAtLeast::kError);
}

TEST_F(LogFlagsTest, SetMinLogLevel) {
  turbo::SetFlag(&FLAGS_minloglevel,
                static_cast<int>(turbo::LogSeverityAtLeast::kError));

  EXPECT_EQ(turbo::MinLogLevel(), turbo::LogSeverityAtLeast::kError);

  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(
      turbo::LogSeverityAtLeast::kWarning);

  EXPECT_EQ(turbo::GetFlag(FLAGS_minloglevel),
            static_cast<int>(turbo::LogSeverityAtLeast::kWarning));
}

TEST_F(LogFlagsTest, PrependLogPrefix) {
  turbo::SetFlag(&FLAGS_log_prefix, false);

  EXPECT_EQ(turbo::ShouldPrependLogPrefix(), false);

  turbo::EnableLogPrefix(true);

  EXPECT_EQ(turbo::GetFlag(FLAGS_log_prefix), true);
}

TEST_F(LogFlagsTest, EmptyBacktraceAtFlag) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  turbo::SetFlag(&FLAGS_log_backtrace_at, "");
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << "hello world";
}

TEST_F(LogFlagsTest, BacktraceAtNonsense) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  turbo::SetFlag(&FLAGS_log_backtrace_at, "gibberish");
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  LOG(INFO) << "hello world";
}

TEST_F(LogFlagsTest, BacktraceAtWrongFile) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("some_other_file.cc:", log_line));
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtWrongLine) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line + 1));
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtWholeFilename) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::SetFlag(&FLAGS_log_backtrace_at, turbo::StrCat(__FILE__, ":", log_line));
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtNonmatchingSuffix) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line, "gibberish"));
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_F(LogFlagsTest, LogsBacktrace) {
  turbo::SetMinLogLevel(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line));
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(HasSubstr("(stacktrace:"))));

  test_sink.StartCapturingLogs();
  do_log();
}

}  // namespace
