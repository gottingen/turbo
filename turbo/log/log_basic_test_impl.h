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

// The testcases in this file are expected to pass or be skipped with any value
// of TURBO_MIN_LOG_LEVEL

#ifndef TURBO_LOG_LOG_BASIC_TEST_IMPL_H_
#define TURBO_LOG_LOG_BASIC_TEST_IMPL_H_

// Verify that both sets of macros behave identically by parameterizing the
// entire test file.
#ifndef TURBO_TEST_LOG
#error TURBO_TEST_LOG must be defined for these tests to work.
#endif

#include <cerrno>
#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/platform/internal/sysinfo.h"
#include "turbo/base/log_severity.h"
#include "turbo/log/globals.h"
#include "turbo/log/internal/test_actions.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/internal/test_matchers.h"
#include "turbo/log/log_entry.h"
#include "turbo/log/scoped_mock_log.h"

namespace turbo_log_internal {
#if GTEST_HAS_DEATH_TEST
using ::turbo::log_internal::DeathTestExpectedLogging;
using ::turbo::log_internal::DeathTestUnexpectedLogging;
using ::turbo::log_internal::DeathTestValidateExpectations;
using ::turbo::log_internal::DiedOfFatal;
using ::turbo::log_internal::DiedOfQFatal;
#endif
using ::turbo::log_internal::LoggingEnabledAt;
using ::turbo::log_internal::LogSeverity;
using ::turbo::log_internal::Prefix;
using ::turbo::log_internal::SourceBasename;
using ::turbo::log_internal::SourceFilename;
using ::turbo::log_internal::SourceLine;
using ::turbo::log_internal::Stacktrace;
using ::turbo::log_internal::TextMessage;
using ::turbo::log_internal::ThreadID;
using ::turbo::log_internal::TimestampInMatchWindow;
using ::turbo::log_internal::Verbosity;
using ::testing::AnyNumber;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::IsTrue;

class BasicLogTest : public testing::TestWithParam<turbo::LogSeverityAtLeast> {};

std::string ThresholdName(
    testing::TestParamInfo<turbo::LogSeverityAtLeast> severity) {
  std::stringstream ostr;
  ostr << severity.param;
  return ostr.str().substr(
      severity.param == turbo::LogSeverityAtLeast::kInfinity ? 0 : 2);
}

INSTANTIATE_TEST_SUITE_P(WithParam, BasicLogTest,
                         testing::Values(turbo::LogSeverityAtLeast::kInfo,
                                         turbo::LogSeverityAtLeast::kWarning,
                                         turbo::LogSeverityAtLeast::kError,
                                         turbo::LogSeverityAtLeast::kFatal,
                                         turbo::LogSeverityAtLeast::kInfinity),
                         ThresholdName);

TEST_P(BasicLogTest, Info) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int log_line = __LINE__ + 1;
  auto do_log = [] { TURBO_TEST_LOG(INFO) << "hello world"; };

  if (LoggingEnabledAt(turbo::LogSeverity::kInfo)) {
    EXPECT_CALL(
        test_sink,
        Send(AllOf(SourceFilename(Eq(__FILE__)),
                   SourceBasename(Eq("log_basic_test_impl.h")),
                   SourceLine(Eq(log_line)), Prefix(IsTrue()),
                   LogSeverity(Eq(turbo::LogSeverity::kInfo)),
                   TimestampInMatchWindow(),
                   ThreadID(Eq(turbo::base_internal::GetTID())),
                   TextMessage(Eq("hello world")),
                   Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                   ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                      literal: "hello world"
                                                    })pb")),
                   Stacktrace(IsEmpty()))));
  }

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_P(BasicLogTest, Warning) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int log_line = __LINE__ + 1;
  auto do_log = [] { TURBO_TEST_LOG(WARNING) << "hello world"; };

  if (LoggingEnabledAt(turbo::LogSeverity::kWarning)) {
    EXPECT_CALL(
        test_sink,
        Send(AllOf(SourceFilename(Eq(__FILE__)),
                   SourceBasename(Eq("log_basic_test_impl.h")),
                   SourceLine(Eq(log_line)), Prefix(IsTrue()),
                   LogSeverity(Eq(turbo::LogSeverity::kWarning)),
                   TimestampInMatchWindow(),
                   ThreadID(Eq(turbo::base_internal::GetTID())),
                   TextMessage(Eq("hello world")),
                   Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                   ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                      literal: "hello world"
                                                    })pb")),
                   Stacktrace(IsEmpty()))));
  }

  test_sink.StartCapturingLogs();
  do_log();
}

TEST_P(BasicLogTest, Error) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  const int log_line = __LINE__ + 1;
  auto do_log = [] { TURBO_TEST_LOG(ERROR) << "hello world"; };

  if (LoggingEnabledAt(turbo::LogSeverity::kError)) {
    EXPECT_CALL(
        test_sink,
        Send(AllOf(SourceFilename(Eq(__FILE__)),
                   SourceBasename(Eq("log_basic_test_impl.h")),
                   SourceLine(Eq(log_line)), Prefix(IsTrue()),
                   LogSeverity(Eq(turbo::LogSeverity::kError)),
                   TimestampInMatchWindow(),
                   ThreadID(Eq(turbo::base_internal::GetTID())),
                   TextMessage(Eq("hello world")),
                   Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                   ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                      literal: "hello world"
                                                    })pb")),
                   Stacktrace(IsEmpty()))));
  }

  test_sink.StartCapturingLogs();
  do_log();
}

#if GTEST_HAS_DEATH_TEST
using BasicLogDeathTest = BasicLogTest;

INSTANTIATE_TEST_SUITE_P(WithParam, BasicLogDeathTest,
                         testing::Values(turbo::LogSeverityAtLeast::kInfo,
                                         turbo::LogSeverityAtLeast::kFatal,
                                         turbo::LogSeverityAtLeast::kInfinity),
                         ThresholdName);

TEST_P(BasicLogDeathTest, Fatal) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  const int log_line = __LINE__ + 1;
  auto do_log = [] { TURBO_TEST_LOG(FATAL) << "hello world"; };

  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink(
            turbo::MockLogDefault::kDisallowUnexpected);

        EXPECT_CALL(test_sink, Send)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());

        ::testing::InSequence s;

        // Note the logic in DeathTestValidateExpectations() caters for the case
        // of logging being disabled at FATAL level.

        if (LoggingEnabledAt(turbo::LogSeverity::kFatal)) {
          // The first call without the stack trace.
          EXPECT_CALL(
              test_sink,
              Send(AllOf(SourceFilename(Eq(__FILE__)),
                         SourceBasename(Eq("log_basic_test_impl.h")),
                         SourceLine(Eq(log_line)), Prefix(IsTrue()),
                         LogSeverity(Eq(turbo::LogSeverity::kFatal)),
                         TimestampInMatchWindow(),
                         ThreadID(Eq(turbo::base_internal::GetTID())),
                         TextMessage(Eq("hello world")),
                         Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")),
                         Stacktrace(IsEmpty()))))
              .WillOnce(DeathTestExpectedLogging());

          // The second call with the stack trace.
          EXPECT_CALL(
              test_sink,
              Send(AllOf(SourceFilename(Eq(__FILE__)),
                         SourceBasename(Eq("log_basic_test_impl.h")),
                         SourceLine(Eq(log_line)), Prefix(IsTrue()),
                         LogSeverity(Eq(turbo::LogSeverity::kFatal)),
                         TimestampInMatchWindow(),
                         ThreadID(Eq(turbo::base_internal::GetTID())),
                         TextMessage(Eq("hello world")),
                         Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")),
                         Stacktrace(Not(IsEmpty())))))
              .WillOnce(DeathTestExpectedLogging());
        }

        test_sink.StartCapturingLogs();
        do_log();
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_P(BasicLogDeathTest, QFatal) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  const int log_line = __LINE__ + 1;
  auto do_log = [] { TURBO_TEST_LOG(QFATAL) << "hello world"; };

  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink(
            turbo::MockLogDefault::kDisallowUnexpected);

        EXPECT_CALL(test_sink, Send)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());

        if (LoggingEnabledAt(turbo::LogSeverity::kFatal)) {
          EXPECT_CALL(
              test_sink,
              Send(AllOf(SourceFilename(Eq(__FILE__)),
                         SourceBasename(Eq("log_basic_test_impl.h")),
                         SourceLine(Eq(log_line)), Prefix(IsTrue()),
                         LogSeverity(Eq(turbo::LogSeverity::kFatal)),
                         TimestampInMatchWindow(),
                         ThreadID(Eq(turbo::base_internal::GetTID())),
                         TextMessage(Eq("hello world")),
                         Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")),
                         Stacktrace(IsEmpty()))))
              .WillOnce(DeathTestExpectedLogging());
        }

        test_sink.StartCapturingLogs();
        do_log();
      },
      DiedOfQFatal, DeathTestValidateExpectations());
}
#endif

TEST_P(BasicLogTest, Level) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  for (auto severity : {turbo::LogSeverity::kInfo, turbo::LogSeverity::kWarning,
                        turbo::LogSeverity::kError}) {
    turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

    const int log_line = __LINE__ + 2;
    auto do_log = [severity] {
      TURBO_TEST_LOG(LEVEL(severity)) << "hello world";
    };

    if (LoggingEnabledAt(severity)) {
      EXPECT_CALL(
          test_sink,
          Send(AllOf(SourceFilename(Eq(__FILE__)),
                     SourceBasename(Eq("log_basic_test_impl.h")),
                     SourceLine(Eq(log_line)), Prefix(IsTrue()),
                     LogSeverity(Eq(severity)), TimestampInMatchWindow(),
                     ThreadID(Eq(turbo::base_internal::GetTID())),
                     TextMessage(Eq("hello world")),
                     Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                     ENCODED_MESSAGE(EqualsProto(R"pb(value {
                                                        literal: "hello world"
                                                      })pb")),
                     Stacktrace(IsEmpty()))));
    }
    test_sink.StartCapturingLogs();
    do_log();
  }
}

#if GTEST_HAS_DEATH_TEST
TEST_P(BasicLogDeathTest, Level) {
  // TODO(b/242568884): re-enable once bug is fixed.
  // turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  // Ensure that `severity` is not a compile-time constant to prove that
  // `LOG(LEVEL(severity))` works regardless:
  auto volatile severity = turbo::LogSeverity::kFatal;

  const int log_line = __LINE__ + 1;
  auto do_log = [severity] { TURBO_TEST_LOG(LEVEL(severity)) << "hello world"; };

  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink(
            turbo::MockLogDefault::kDisallowUnexpected);

        EXPECT_CALL(test_sink, Send)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());

        ::testing::InSequence s;

        if (LoggingEnabledAt(turbo::LogSeverity::kFatal)) {
          EXPECT_CALL(
              test_sink,
              Send(AllOf(SourceFilename(Eq(__FILE__)),
                         SourceBasename(Eq("log_basic_test_impl.h")),
                         SourceLine(Eq(log_line)), Prefix(IsTrue()),
                         LogSeverity(Eq(turbo::LogSeverity::kFatal)),
                         TimestampInMatchWindow(),
                         ThreadID(Eq(turbo::base_internal::GetTID())),
                         TextMessage(Eq("hello world")),
                         Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")),
                         Stacktrace(IsEmpty()))))
              .WillOnce(DeathTestExpectedLogging());

          EXPECT_CALL(
              test_sink,
              Send(AllOf(SourceFilename(Eq(__FILE__)),
                         SourceBasename(Eq("log_basic_test_impl.h")),
                         SourceLine(Eq(log_line)), Prefix(IsTrue()),
                         LogSeverity(Eq(turbo::LogSeverity::kFatal)),
                         TimestampInMatchWindow(),
                         ThreadID(Eq(turbo::base_internal::GetTID())),
                         TextMessage(Eq("hello world")),
                         Verbosity(Eq(turbo::LogEntry::kNoVerbosityLevel)),
                         ENCODED_MESSAGE(EqualsProto(
                             R"pb(value { literal: "hello world" })pb")),
                         Stacktrace(Not(IsEmpty())))))
              .WillOnce(DeathTestExpectedLogging());
        }

        test_sink.StartCapturingLogs();
        do_log();
      },
      DiedOfFatal, DeathTestValidateExpectations());
}
#endif

TEST_P(BasicLogTest, LevelClampsNegativeValues) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  if (!LoggingEnabledAt(turbo::LogSeverity::kInfo)) {
    GTEST_SKIP() << "This test cases required INFO log to be enabled";
    return;
  }

  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(LogSeverity(Eq(turbo::LogSeverity::kInfo))));

  test_sink.StartCapturingLogs();
  TURBO_TEST_LOG(LEVEL(-1)) << "hello world";
}

TEST_P(BasicLogTest, LevelClampsLargeValues) {
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(GetParam());

  if (!LoggingEnabledAt(turbo::LogSeverity::kError)) {
    GTEST_SKIP() << "This test cases required ERROR log to be enabled";
    return;
  }

  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Send(LogSeverity(Eq(turbo::LogSeverity::kError))));

  test_sink.StartCapturingLogs();
  TURBO_TEST_LOG(LEVEL(static_cast<int>(turbo::LogSeverity::kFatal) + 1))
      << "hello world";
}

TEST(ErrnoPreservationTest, InSeverityExpression) {
  errno = 77;
  int saved_errno;
  TURBO_TEST_LOG(LEVEL((saved_errno = errno, turbo::LogSeverity::kInfo)));
  EXPECT_THAT(saved_errno, Eq(77));
}

TEST(ErrnoPreservationTest, InStreamedExpression) {
  if (!LoggingEnabledAt(turbo::LogSeverity::kInfo)) {
    GTEST_SKIP() << "This test cases required INFO log to be enabled";
    return;
  }

  errno = 77;
  int saved_errno = 0;
  TURBO_TEST_LOG(INFO) << (saved_errno = errno, "hello world");
  EXPECT_THAT(saved_errno, Eq(77));
}

TEST(ErrnoPreservationTest, AfterStatement) {
  errno = 77;
  TURBO_TEST_LOG(INFO);
  const int saved_errno = errno;
  EXPECT_THAT(saved_errno, Eq(77));
}

// Tests that using a variable/parameter in a logging statement suppresses
// unused-variable/parameter warnings.
// -----------------------------------------------------------------------
class UnusedVariableWarningCompileTest {
  // These four don't prove anything unless `TURBO_MIN_LOG_LEVEL` is greater than
  // `kInfo`.
  static void LoggedVariable() {
    const int x = 0;
    TURBO_TEST_LOG(INFO) << x;
  }
  static void LoggedParameter(const int x) { TURBO_TEST_LOG(INFO) << x; }
  static void SeverityVariable() {
    const int x = 0;
    TURBO_TEST_LOG(LEVEL(x)) << "hello world";
  }
  static void SeverityParameter(const int x) {
    TURBO_TEST_LOG(LEVEL(x)) << "hello world";
  }
};

}  // namespace turbo_log_internal

#endif  // TURBO_LOG_LOG_BASIC_TEST_IMPL_H_
