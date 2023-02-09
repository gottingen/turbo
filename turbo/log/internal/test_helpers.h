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
//
// -----------------------------------------------------------------------------
// File: log/internal/test_helpers.h
// -----------------------------------------------------------------------------
//
// This file declares testing helpers for the logging library.

#ifndef TURBO_LOG_INTERNAL_TEST_HELPERS_H_
#define TURBO_LOG_INTERNAL_TEST_HELPERS_H_

#include "turbo/base/log_severity.h"
#include "turbo/log/globals.h"
#include "turbo/platform/port.h"
#include "gtest/gtest.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// `TURBO_MIN_LOG_LEVEL` can't be used directly since it is not always defined.
constexpr auto kTurboMinLogLevel =
#ifdef TURBO_MIN_LOG_LEVEL
    static_cast<turbo::LogSeverityAtLeast>(TURBO_MIN_LOG_LEVEL);
#else
    turbo::LogSeverityAtLeast::kInfo;
#endif

// Returns false if the specified severity level is disabled by
// `TURBO_MIN_LOG_LEVEL` or `turbo::MinLogLevel()`.
bool LoggingEnabledAt(turbo::LogSeverity severity);

// -----------------------------------------------------------------------------
// Googletest Death Test Predicates
// -----------------------------------------------------------------------------

#if GTEST_HAS_DEATH_TEST

bool DiedOfFatal(int exit_status);
bool DiedOfQFatal(int exit_status);

#endif

// -----------------------------------------------------------------------------
// Helper for Log inititalization in test
// -----------------------------------------------------------------------------

class LogTestEnvironment : public ::testing::Environment {
 public:
  ~LogTestEnvironment() override = default;

  void SetUp() override;
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_TEST_HELPERS_H_
