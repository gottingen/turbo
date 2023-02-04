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

#include "turbo/base/log_severity.h"
#include "turbo/log/logging.h"
#include "turbo/log/scoped_mock_log.h"
#include "turbo/platform/port.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using ::testing::_;
using ::testing::Eq;

namespace not_turbo {

class Dummy {
 public:
  Dummy() {}

 private:
  Dummy(const Dummy&) = delete;
  Dummy& operator=(const Dummy&) = delete;
};

// This line tests that local definitions of INFO, WARNING, ERROR, and
// etc don't shadow the global ones used by the logging macros.  If
// they do, the TURBO_LOG() calls in the tests won't compile, catching the
// bug.
const Dummy INFO, WARNING, ERROR, FATAL, NUM_SEVERITIES;

// These makes sure that the uses of same-named types in the
// implementation of the logging macros are fully qualified.
class string {};
class vector {};
class LogMessage {};
class LogMessageFatal {};
class LogMessageQuietlyFatal {};
class LogMessageVoidify {};
class LogSink {};
class NullStream {};
class NullStreamFatal {};

}  // namespace not_turbo

using namespace not_turbo;  // NOLINT

// Tests for TURBO_LOG(LEVEL(()).

TEST(LogHygieneTest, WorksForQualifiedSeverity) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  ::testing::InSequence seq;
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "To INFO"));
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kWarning, _, "To WARNING"));
  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kError, _, "To ERROR"));

  test_sink.StartCapturingLogs();
  // Note that TURBO_LOG(LEVEL()) expects the severity as a run-time
  // expression (as opposed to a compile-time constant).  Hence we
  // test that :: is allowed before INFO, etc.
  TURBO_LOG(LEVEL(turbo::LogSeverity::kInfo)) << "To INFO";
  TURBO_LOG(LEVEL(turbo::LogSeverity::kWarning)) << "To WARNING";
  TURBO_LOG(LEVEL(turbo::LogSeverity::kError)) << "To ERROR";
}

TEST(LogHygieneTest, WorksWithAlternativeINFOSymbol) {
  const double INFO TURBO_ATTRIBUTE_UNUSED = 7.77;
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "Hello world"));

  test_sink.StartCapturingLogs();
  TURBO_LOG(INFO) << "Hello world";
}

TEST(LogHygieneTest, WorksWithAlternativeWARNINGSymbol) {
  const double WARNING TURBO_ATTRIBUTE_UNUSED = 7.77;
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kWarning, _, "Hello world"));

  test_sink.StartCapturingLogs();
  TURBO_LOG(WARNING) << "Hello world";
}

TEST(LogHygieneTest, WorksWithAlternativeERRORSymbol) {
  const double ERROR TURBO_ATTRIBUTE_UNUSED = 7.77;
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kError, _, "Hello world"));

  test_sink.StartCapturingLogs();
  TURBO_LOG(ERROR) << "Hello world";
}

TEST(LogHygieneTest, WorksWithAlternativeLEVELSymbol) {
  const double LEVEL TURBO_ATTRIBUTE_UNUSED = 7.77;
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kError, _, "Hello world"));

  test_sink.StartCapturingLogs();
  TURBO_LOG(LEVEL(turbo::LogSeverity::kError)) << "Hello world";
}

#define INFO Bogus
#ifdef NDEBUG
constexpr bool IsOptimized = false;
#else
constexpr bool IsOptimized = true;
#endif

TEST(LogHygieneTest, WorksWithINFODefined) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "Hello world"))
      .Times(2 + (IsOptimized ? 2 : 0));

  test_sink.StartCapturingLogs();
  TURBO_LOG(INFO) << "Hello world";
  TURBO_LOG_IF(INFO, true) << "Hello world";

  TURBO_DLOG(INFO) << "Hello world";
  TURBO_DLOG_IF(INFO, true) << "Hello world";
}

#undef INFO

#define _INFO Bogus
TEST(LogHygieneTest, WorksWithUnderscoreINFODefined) {
  turbo::ScopedMockLog test_sink(turbo::MockLogDefault::kDisallowUnexpected);

  EXPECT_CALL(test_sink, Log(turbo::LogSeverity::kInfo, _, "Hello world"))
      .Times(2 + (IsOptimized ? 2 : 0));

  test_sink.StartCapturingLogs();
  TURBO_LOG(INFO) << "Hello world";
  TURBO_LOG_IF(INFO, true) << "Hello world";

  TURBO_DLOG(INFO) << "Hello world";
  TURBO_LOG_IF(INFO, true) << "Hello world";
}
#undef _INFO

TEST(LogHygieneTest, ExpressionEvaluationInLEVELSeverity) {
  auto i = static_cast<int>(turbo::LogSeverity::kInfo);
  TURBO_LOG(LEVEL(++i)) << "hello world";  // NOLINT
  EXPECT_THAT(i, Eq(static_cast<int>(turbo::LogSeverity::kInfo) + 1));
}

TEST(LogHygieneTest, ExpressionEvaluationInStreamedMessage) {
  int i = 0;
  TURBO_LOG(INFO) << ++i;
  EXPECT_THAT(i, 1);
  TURBO_LOG_IF(INFO, false) << ++i;
  EXPECT_THAT(i, 1);
}

// Tests that macros are usable in unbraced switch statements.
// -----------------------------------------------------------

class UnbracedSwitchCompileTest {
  static void Log() {
    switch (0) {
      case 0:
        TURBO_LOG(INFO);
        break;
      default:
        break;
    }
  }
};

}  // namespace
