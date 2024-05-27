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

#include <turbo/log/log_sink.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <tests/log/test_actions.h>
#include <tests/log/test_helpers.h>
#include <tests/log/test_matchers.h>
#include <turbo/log/log.h>
#include <turbo/log/log_sink_registry.h>
#include <tests/log/scoped_mock_log.h>
#include <turbo/strings/string_view.h>

namespace {

using ::turbo::log_internal::DeathTestExpectedLogging;
using ::turbo::log_internal::DeathTestUnexpectedLogging;
using ::turbo::log_internal::DeathTestValidateExpectations;
using ::turbo::log_internal::DiedOfFatal;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::InSequence;

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

// Tests for global log sink registration.
// ---------------------------------------

TEST(LogSinkRegistryTest, AddLogSink) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  InSequence s;
  EXPECT_CALL(test_sink, Log(_, _, "hello world")).Times(0);
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, __FILE__, "Test : 42"));
  EXPECT_CALL(test_sink,
              Log(turbo::LogSeverity::kWarning, __FILE__, "Danger ahead"));
  EXPECT_CALL(test_sink,
              Log(turbo::LogSeverity::kError, __FILE__, "This is an error"));

  LOG(INFO) << "hello world";
  test_sink.StartCapturingLogs();

  LOG(INFO) << "Test : " << 42;
  LOG(WARNING) << "Danger" << ' ' << "ahead";
  LOG(ERROR) << "This is an error";

  test_sink.StopCapturingLogs();
  LOG(INFO) << "Goodby world";
}

TEST(LogSinkRegistryTest, MultipleLogSinks) {
  turbo::ScopedMockLog test_sink1(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog test_sink2(turbo::MockLogDefault::kDisallowUnexpected);

  ::testing::InSequence seq;
  EXPECT_CALL(test_sink1, Log(turbo::LogSeverity::kInfo, _, "First")).Times(1);
  EXPECT_CALL(test_sink2, Log(turbo::LogSeverity::kInfo, _, "First")).Times(0);

  EXPECT_CALL(test_sink1, Log(turbo::LogSeverity::kInfo, _, "Second")).Times(1);
  EXPECT_CALL(test_sink2, Log(turbo::LogSeverity::kInfo, _, "Second")).Times(1);

  EXPECT_CALL(test_sink1, Log(turbo::LogSeverity::kInfo, _, "Third")).Times(0);
  EXPECT_CALL(test_sink2, Log(turbo::LogSeverity::kInfo, _, "Third")).Times(1);

  LOG(INFO) << "Before first";

  test_sink1.StartCapturingLogs();
  LOG(INFO) << "First";

  test_sink2.StartCapturingLogs();
  LOG(INFO) << "Second";

  test_sink1.StopCapturingLogs();
  LOG(INFO) << "Third";

  test_sink2.StopCapturingLogs();
  LOG(INFO) << "Fourth";
}

TEST(LogSinkRegistrationDeathTest, DuplicateSinkRegistration) {
  ASSERT_DEATH_IF_SUPPORTED(
      {
        turbo::ScopedMockLog sink;
        sink.StartCapturingLogs();
        turbo::AddLogSink(&sink.UseAsLocalSink());
      },
      HasSubstr("Duplicate log sinks"));
}

TEST(LogSinkRegistrationDeathTest, MismatchSinkRemoval) {
  ASSERT_DEATH_IF_SUPPORTED(
      {
        turbo::ScopedMockLog sink;
        turbo::RemoveLogSink(&sink.UseAsLocalSink());
      },
      HasSubstr("Mismatched log sink"));
}

// Tests for log sink semantic.
// ---------------------------------------

TEST(LogSinkTest, FlushSinks) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Flush()).Times(2);

  test_sink.StartCapturingLogs();

  turbo::FlushLogSinks();
  turbo::FlushLogSinks();
}

TEST(LogSinkDeathTest, DeathInSend) {
  class FatalSendSink : public turbo::LogSink {
   public:
    void Send(const turbo::LogEntry&) override { LOG(FATAL) << "goodbye world"; }
  };

  FatalSendSink sink;
  EXPECT_EXIT({ LOG(INFO).ToSinkAlso(&sink) << "hello world"; }, DiedOfFatal,
              _);
}

// Tests for explicit log sink redirection.
// ---------------------------------------

TEST(LogSinkTest, ToSinkAlso) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog another_sink(turbo::MockLogDefault::kDisallowUnexpected);
  EXPECT_CALL(test_sink, Log(_, _, "hello world"));
  EXPECT_CALL(another_sink, Log(_, _, "hello world"));

  test_sink.StartCapturingLogs();
  LOG(INFO).ToSinkAlso(&another_sink.UseAsLocalSink()) << "hello world";
}

TEST(LogSinkTest, ToSinkOnly) {
  turbo::ScopedMockLog another_sink(turbo::MockLogDefault::kDisallowUnexpected);
  EXPECT_CALL(another_sink, Log(_, _, "hello world"));
  LOG(INFO).ToSinkOnly(&another_sink.UseAsLocalSink()) << "hello world";
}

TEST(LogSinkTest, ToManySinks) {
  turbo::ScopedMockLog sink1(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog sink2(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog sink3(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog sink4(turbo::MockLogDefault::kDisallowUnexpected);
  turbo::ScopedMockLog sink5(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(sink3, Log(_, _, "hello world"));
  EXPECT_CALL(sink4, Log(_, _, "hello world"));
  EXPECT_CALL(sink5, Log(_, _, "hello world"));

  LOG(INFO)
          .ToSinkAlso(&sink1.UseAsLocalSink())
          .ToSinkAlso(&sink2.UseAsLocalSink())
          .ToSinkOnly(&sink3.UseAsLocalSink())
          .ToSinkAlso(&sink4.UseAsLocalSink())
          .ToSinkAlso(&sink5.UseAsLocalSink())
      << "hello world";
}

class ReentrancyTest : public ::testing::Test {
 protected:
  ReentrancyTest() = default;
  enum class LogMode : int { kNormal, kToSinkAlso, kToSinkOnly };

  class ReentrantSendLogSink : public turbo::LogSink {
   public:
    explicit ReentrantSendLogSink(turbo::LogSeverity severity,
                                  turbo::LogSink* sink, LogMode mode)
        : severity_(severity), sink_(sink), mode_(mode) {}
    explicit ReentrantSendLogSink(turbo::LogSeverity severity)
        : ReentrantSendLogSink(severity, nullptr, LogMode::kNormal) {}

    void Send(const turbo::LogEntry&) override {
      switch (mode_) {
        case LogMode::kNormal:
          LOG(LEVEL(severity_)) << "The log is coming from *inside the sink*.";
          break;
        case LogMode::kToSinkAlso:
          LOG(LEVEL(severity_)).ToSinkAlso(sink_)
              << "The log is coming from *inside the sink*.";
          break;
        case LogMode::kToSinkOnly:
          LOG(LEVEL(severity_)).ToSinkOnly(sink_)
              << "The log is coming from *inside the sink*.";
          break;
        default:
          LOG(FATAL) << "Invalid mode " << static_cast<int>(mode_);
      }
    }

   private:
    turbo::LogSeverity severity_;
    turbo::LogSink* sink_;
    LogMode mode_;
  };

  static turbo::string_view LogAndReturn(turbo::LogSeverity severity,
                                        turbo::string_view to_log,
                                        turbo::string_view to_return) {
    LOG(LEVEL(severity)) << to_log;
    return to_return;
  }
};

TEST_F(ReentrancyTest, LogFunctionThatLogs) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  InSequence seq;
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "hello"));
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "world"));
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kWarning, _, "danger"));
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "here"));

  test_sink.StartCapturingLogs();
  LOG(INFO) << LogAndReturn(turbo::LogSeverity::kInfo, "hello", "world");
  LOG(INFO) << LogAndReturn(turbo::LogSeverity::kWarning, "danger", "here");
}

TEST_F(ReentrancyTest, RegisteredLogSinkThatLogsInSend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  ReentrantSendLogSink renentrant_sink(turbo::LogSeverity::kInfo);
  EXPECT_CALL(test_sink, Log(_, _, "hello world"));

  test_sink.StartCapturingLogs();
  turbo::AddLogSink(&renentrant_sink);
  LOG(INFO) << "hello world";
  turbo::RemoveLogSink(&renentrant_sink);
}

TEST_F(ReentrancyTest, AlsoLogSinkThatLogsInSend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kInfo);
  EXPECT_CALL(test_sink, Log(_, _, "hello world"));
  EXPECT_CALL(test_sink,
              Log(_, _, "The log is coming from *inside the sink*."));

  test_sink.StartCapturingLogs();
  LOG(INFO).ToSinkAlso(&reentrant_sink) << "hello world";
}

TEST_F(ReentrancyTest, RegisteredAlsoLogSinkThatLogsInSend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kInfo);
  EXPECT_CALL(test_sink, Log(_, _, "hello world"));
  // We only call into the test_log sink once with this message, since the
  // second time log statement is run we are in "ThreadIsLogging" mode and all
  // the log statements are redirected into stderr.
  EXPECT_CALL(test_sink,
              Log(_, _, "The log is coming from *inside the sink*."));

  test_sink.StartCapturingLogs();
  turbo::AddLogSink(&reentrant_sink);
  LOG(INFO).ToSinkAlso(&reentrant_sink) << "hello world";
  turbo::RemoveLogSink(&reentrant_sink);
}

TEST_F(ReentrancyTest, OnlyLogSinkThatLogsInSend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kInfo);
  EXPECT_CALL(test_sink,
              Log(_, _, "The log is coming from *inside the sink*."));

  test_sink.StartCapturingLogs();
  LOG(INFO).ToSinkOnly(&reentrant_sink) << "hello world";
}

TEST_F(ReentrancyTest, RegisteredOnlyLogSinkThatLogsInSend) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);
  ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kInfo);
  EXPECT_CALL(test_sink,
              Log(_, _, "The log is coming from *inside the sink*."));

  test_sink.StartCapturingLogs();
  turbo::AddLogSink(&reentrant_sink);
  LOG(INFO).ToSinkOnly(&reentrant_sink) << "hello world";
  turbo::RemoveLogSink(&reentrant_sink);
}

using ReentrancyDeathTest = ReentrancyTest;

TEST_F(ReentrancyDeathTest, LogFunctionThatLogsFatal) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;

        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink, Log(_, _, "hello"))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        LOG(INFO) << LogAndReturn(turbo::LogSeverity::kFatal, "hello", "world");
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_F(ReentrancyDeathTest, RegisteredLogSinkThatLogsFatalInSend) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;
        ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kFatal);
        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink, Log(_, _, "hello world"))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        turbo::AddLogSink(&reentrant_sink);
        LOG(INFO) << "hello world";
        // No need to call RemoveLogSink - process is dead at this point.
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_F(ReentrancyDeathTest, AlsoLogSinkThatLogsFatalInSend) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;
        ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kFatal);

        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink, Log(_, _, "hello world"))
            .WillOnce(DeathTestExpectedLogging());
        EXPECT_CALL(test_sink,
                    Log(_, _, "The log is coming from *inside the sink*."))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        LOG(INFO).ToSinkAlso(&reentrant_sink) << "hello world";
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_F(ReentrancyDeathTest, RegisteredAlsoLogSinkThatLogsFatalInSend) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;
        ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kFatal);
        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink, Log(_, _, "hello world"))
            .WillOnce(DeathTestExpectedLogging());
        EXPECT_CALL(test_sink,
                    Log(_, _, "The log is coming from *inside the sink*."))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        turbo::AddLogSink(&reentrant_sink);
        LOG(INFO).ToSinkAlso(&reentrant_sink) << "hello world";
        // No need to call RemoveLogSink - process is dead at this point.
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_F(ReentrancyDeathTest, OnlyLogSinkThatLogsFatalInSend) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;
        ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kFatal);
        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink,
                    Log(_, _, "The log is coming from *inside the sink*."))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        LOG(INFO).ToSinkOnly(&reentrant_sink) << "hello world";
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

TEST_F(ReentrancyDeathTest, RegisteredOnlyLogSinkThatLogsFatalInSend) {
  EXPECT_EXIT(
      {
        turbo::ScopedMockLog test_sink;
        ReentrantSendLogSink reentrant_sink(turbo::LogSeverity::kFatal);
        EXPECT_CALL(test_sink, Log)
            .Times(AnyNumber())
            .WillRepeatedly(DeathTestUnexpectedLogging());
        EXPECT_CALL(test_sink,
                    Log(_, _, "The log is coming from *inside the sink*."))
            .WillOnce(DeathTestExpectedLogging());

        test_sink.StartCapturingLogs();
        turbo::AddLogSink(&reentrant_sink);
        LOG(INFO).ToSinkOnly(&reentrant_sink) << "hello world";
        // No need to call RemoveLogSink - process is dead at this point.
      },
      DiedOfFatal, DeathTestValidateExpectations());
}

}  // namespace
