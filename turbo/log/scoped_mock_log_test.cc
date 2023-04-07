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

#include "turbo/log/scoped_mock_log.h"

#include <memory>
#include <thread>  // NOLINT(build/c++11)

#include "turbo/base/log_severity.h"
#include "turbo/log/globals.h"
#include "turbo/log/internal/test_helpers.h"
#include "turbo/log/internal/test_matchers.h"
#include "turbo/log/log.h"
#include "turbo/memory/memory.h"
#include "turbo/platform/port.h"
#include "turbo/strings/match.h"
#include "turbo/strings/string_piece.h"
#include "turbo/concurrent/barrier.h"
#include "turbo/concurrent/latch.h"
#include "gmock/gmock.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

namespace {

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Lt;
using ::testing::Truly;
using turbo::log_internal::SourceBasename;
using turbo::log_internal::SourceFilename;
using turbo::log_internal::SourceLine;
using turbo::log_internal::TextMessageWithPrefix;
using turbo::log_internal::ThreadID;

auto* test_env TURBO_MAYBE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

#if GTEST_HAS_DEATH_TEST
TEST(ScopedMockLogDeathTest,
     StartCapturingLogsCannotBeCalledWhenAlreadyCapturing) {
  EXPECT_DEATH(
      {
        turbo::ScopedMockLog log;
        log.StartCapturingLogs();
        log.StartCapturingLogs();
      },
      "StartCapturingLogs");
}

TEST(ScopedMockLogDeathTest, StopCapturingLogsCannotBeCalledWhenNotCapturing) {
  EXPECT_DEATH(
      {
        turbo::ScopedMockLog log;
        log.StopCapturingLogs();
      },
      "StopCapturingLogs");
}
#endif

// Tests that ScopedMockLog intercepts LOG()s when it's alive.
TEST(ScopedMockLogTest, LogMockCatchAndMatchStrictExpectations) {
  turbo::ScopedMockLog log;

  // The following expectations must match in the order they appear.
  InSequence s;
  EXPECT_CALL(log,
              Log(turbo::LogSeverity::kWarning, HasSubstr(__FILE__), "Danger."));
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "Working...")).Times(2);
  EXPECT_CALL(log, Log(turbo::LogSeverity::kError, _, "Bad!!"));

  log.StartCapturingLogs();
  LOG(WARNING) << "Danger.";
  LOG(INFO) << "Working...";
  LOG(INFO) << "Working...";
  LOG(ERROR) << "Bad!!";
}

TEST(ScopedMockLogTest, LogMockCatchAndMatchSendExpectations) {
  turbo::ScopedMockLog log;

  EXPECT_CALL(
      log,
      Send(AllOf(SourceFilename(Eq("/my/very/very/very_long_source_file.cc")),
                 SourceBasename(Eq("very_long_source_file.cc")),
                 SourceLine(Eq(777)), ThreadID(Eq(turbo::LogEntry::tid_t{1234})),
                 TextMessageWithPrefix(Truly([](turbo::string_piece msg) {
                   return turbo::EndsWith(
                       msg, " very_long_source_file.cc:777] Info message");
                 })))));

  log.StartCapturingLogs();
  LOG(INFO)
          .AtLocation("/my/very/very/very_long_source_file.cc", 777)
          .WithThreadID(1234)
      << "Info message";
}

TEST(ScopedMockLogTest, ScopedMockLogCanBeNice) {
  turbo::ScopedMockLog log;

  InSequence s;
  EXPECT_CALL(log,
              Log(turbo::LogSeverity::kWarning, HasSubstr(__FILE__), "Danger."));
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "Working...")).Times(2);
  EXPECT_CALL(log, Log(turbo::LogSeverity::kError, _, "Bad!!"));

  log.StartCapturingLogs();

  // Any number of these are OK.
  LOG(INFO) << "Info message.";
  // Any number of these are OK.
  LOG(WARNING).AtLocation("SomeOtherFile.cc", 100) << "Danger ";

  LOG(WARNING) << "Danger.";

  // Any number of these are OK.
  LOG(INFO) << "Info message.";
  // Any number of these are OK.
  LOG(WARNING).AtLocation("SomeOtherFile.cc", 100) << "Danger ";

  LOG(INFO) << "Working...";

  // Any number of these are OK.
  LOG(INFO) << "Info message.";
  // Any number of these are OK.
  LOG(WARNING).AtLocation("SomeOtherFile.cc", 100) << "Danger ";

  LOG(INFO) << "Working...";

  // Any number of these are OK.
  LOG(INFO) << "Info message.";
  // Any number of these are OK.
  LOG(WARNING).AtLocation("SomeOtherFile.cc", 100) << "Danger ";

  LOG(ERROR) << "Bad!!";

  // Any number of these are OK.
  LOG(INFO) << "Info message.";
  // Any number of these are OK.
  LOG(WARNING).AtLocation("SomeOtherFile.cc", 100) << "Danger ";
}

// Tests that ScopedMockLog generates a test failure if a message is logged
// that is not expected (here, that means ERROR or FATAL).
TEST(ScopedMockLogTest, RejectsUnexpectedLogs) {
  EXPECT_NONFATAL_FAILURE(
      {
        turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);
        // Any INFO and WARNING messages are permitted.
        EXPECT_CALL(log, Log(Lt(turbo::LogSeverity::kError), _, _))
            .Times(AnyNumber());
        log.StartCapturingLogs();
        LOG(INFO) << "Ignored";
        LOG(WARNING) << "Ignored";
        LOG(ERROR) << "Should not be ignored";
      },
      "Should not be ignored");
}

TEST(ScopedMockLogTest, CapturesLogsAfterStartCapturingLogs) {
  turbo::SetStderrThreshold(turbo::LogSeverityAtLeast::kInfinity);
  turbo::ScopedMockLog log;

  // The ScopedMockLog object shouldn't see these LOGs, as it hasn't
  // started capturing LOGs yet.
  LOG(INFO) << "Ignored info";
  LOG(WARNING) << "Ignored warning";
  LOG(ERROR) << "Ignored error";

  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "Expected info"));
  log.StartCapturingLogs();

  // Only this LOG will be seen by the ScopedMockLog.
  LOG(INFO) << "Expected info";
}

TEST(ScopedMockLogTest, DoesNotCaptureLogsAfterStopCapturingLogs) {
  turbo::ScopedMockLog log;
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, "Expected info"));

  log.StartCapturingLogs();

  // This LOG should be seen by the ScopedMockLog.
  LOG(INFO) << "Expected info";

  log.StopCapturingLogs();

  // The ScopedMockLog object shouldn't see these LOGs, as it has
  // stopped capturing LOGs.
  LOG(INFO) << "Ignored info";
  LOG(WARNING) << "Ignored warning";
  LOG(ERROR) << "Ignored error";
}

// Tests that all messages are intercepted regardless of issuing thread. The
// purpose of this test is NOT to exercise thread-safety.
TEST(ScopedMockLogTest, LogFromMultipleThreads) {
  turbo::ScopedMockLog log;

  // We don't establish an order to expectations here, since the threads may
  // execute their log statements in different order.
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, __FILE__, "Thread 1"));
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, __FILE__, "Thread 2"));

  log.StartCapturingLogs();

  turbo::Barrier barrier(2);
  std::thread thread1([&barrier]() {
    barrier.Block();
    LOG(INFO) << "Thread 1";
  });
  std::thread thread2([&barrier]() {
    barrier.Block();
    LOG(INFO) << "Thread 2";
  });

  thread1.join();
  thread2.join();
}

// Tests that no sequence will be imposed on two LOG message expectations from
// different threads. This test would actually deadlock if replaced to two LOG
// statements from the same thread.
/*
TEST(ScopedMockLogTest, NoSequenceWithMultipleThreads) {
  turbo::ScopedMockLog log;

  turbo::Barrier barrier(2);
  EXPECT_CALL(log, Log(turbo::LogSeverity::kInfo, _, _))
      .Times(2)
      .WillRepeatedly([&barrier]() { barrier.Block(); });

  log.StartCapturingLogs();

  std::thread thread1([]() { LOG(INFO) << "Thread 1"; });
  std::thread thread2([]() { LOG(INFO) << "Thread 2"; });

  thread1.join();
  thread2.join();
}
*/
TEST(ScopedMockLogTsanTest,
     ScopedMockLogCanBeDeletedWhenAnotherThreadIsLogging) {
  auto log = turbo::make_unique<turbo::ScopedMockLog>();
  EXPECT_CALL(*log, Log(turbo::LogSeverity::kInfo, __FILE__, "Thread log"))
      .Times(AnyNumber());

  log->StartCapturingLogs();

  turbo::Latch logging_started(1);

  std::thread thread([&logging_started]() {
    for (int i = 0; i < 100; ++i) {
      if (i == 50) logging_started.CountDown();
      LOG(INFO) << "Thread log";
    }
  });

  logging_started.Wait();
  log.reset();
  thread.join();
}

TEST(ScopedMockLogTest, AsLocalSink) {
  turbo::ScopedMockLog log(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(log, Log(_, _, "two"));
  EXPECT_CALL(log, Log(_, _, "three"));

  LOG(INFO) << "one";
  LOG(INFO).ToSinkOnly(&log.UseAsLocalSink()) << "two";
  LOG(INFO).ToSinkAlso(&log.UseAsLocalSink()) << "three";
}

}  // namespace
