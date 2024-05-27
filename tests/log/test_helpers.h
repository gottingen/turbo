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
//
// -----------------------------------------------------------------------------
// File: log/internal/test_helpers.h
// -----------------------------------------------------------------------------
//
// This file declares testing helpers for the logging library.

#ifndef TURBO_LOG_INTERNAL_TEST_HELPERS_H_
#define TURBO_LOG_INTERNAL_TEST_HELPERS_H_

#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/globals.h>

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
// Helper for Log initialization in test
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
