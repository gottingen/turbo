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

#include <turbo/log/internal/flags.h>

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/log_severity.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <turbo/log/globals.h>
#include <tests/log/test_helpers.h>
#include <tests/log/test_matchers.h>
#include <turbo/log/log.h>
#include <tests/log/scoped_mock_log.h>
#include <turbo/strings/str_cat.h>

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
  EXPECT_EQ(turbo::stderr_threshold(), DefaultStderrThreshold());
}

TEST_F(LogFlagsTest, set_stderr_threshold) {
  turbo::SetFlag(&FLAGS_stderrthreshold,
                static_cast<int>(turbo::LogSeverityAtLeast::kInfo));

  EXPECT_EQ(turbo::stderr_threshold(), turbo::LogSeverityAtLeast::kInfo);

  turbo::SetFlag(&FLAGS_stderrthreshold,
                static_cast<int>(turbo::LogSeverityAtLeast::kError));

  EXPECT_EQ(turbo::stderr_threshold(), turbo::LogSeverityAtLeast::kError);
}

TEST_F(LogFlagsTest, set_min_log_level) {
  turbo::SetFlag(&FLAGS_minloglevel,
                static_cast<int>(turbo::LogSeverityAtLeast::kError));

  EXPECT_EQ(turbo::min_log_level(), turbo::LogSeverityAtLeast::kError);

  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(
      turbo::LogSeverityAtLeast::kWarning);

  EXPECT_EQ(turbo::GetFlag(FLAGS_minloglevel),
            static_cast<int>(turbo::LogSeverityAtLeast::kWarning));
}

TEST_F(LogFlagsTest, PrependLogPrefix) {
  turbo::SetFlag(&FLAGS_log_prefix, false);

  EXPECT_EQ(turbo::should_prepend_log_prefix(), false);

  turbo::enable_log_prefix(true);

  EXPECT_EQ(turbo::GetFlag(FLAGS_log_prefix), true);
}

TEST_F(LogFlagsTest, EmptyBacktraceAtFlag) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at, "");
  LOG(INFO) << "hello world";
}

TEST_F(LogFlagsTest, BacktraceAtNonsense) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at, "gibberish");
  LOG(INFO) << "hello world";
}

TEST_F(LogFlagsTest, BacktraceAtWrongFile) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("some_other_file.cc:", log_line));
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtWrongLine) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line + 1));
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtWholeFilename) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at, turbo::StrCat(__FILE__, ":", log_line));
  do_log();
}

TEST_F(LogFlagsTest, BacktraceAtNonmatchingSuffix) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line, "gibberish"));
  do_log();
}

TEST_F(LogFlagsTest, LogsBacktrace) {
  turbo::set_min_log_level(turbo::LogSeverityAtLeast::kInfo);
  const int log_line = __LINE__ + 1;
  auto do_log = [] { LOG(INFO) << "hello world"; };
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  testing::InSequence seq;
  EXPECT_CALL(test_sink, Send(TextMessage(HasSubstr("(stacktrace:"))));
  EXPECT_CALL(test_sink, Send(TextMessage(Not(HasSubstr("(stacktrace:")))));

  test_sink.StartCapturingLogs();
  turbo::SetFlag(&FLAGS_log_backtrace_at,
                turbo::StrCat("flags_test.cc:", log_line));
  do_log();
  turbo::SetFlag(&FLAGS_log_backtrace_at, "");
  do_log();
}

}  // namespace
