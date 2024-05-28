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
#include <tests/log/test_helpers.h>

#ifdef __Fuchsia__
#include <zircon/syscalls.h>
#endif

#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/globals.h>
#include <turbo/log/initialize.h>
#include <turbo/log/internal/globals.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// Returns false if the specified severity level is disabled by
// `TURBO_MIN_LOG_LEVEL` or `turbo::min_log_level()`.
bool LoggingEnabledAt(turbo::LogSeverity severity) {
  return severity >= kTurboMinLogLevel && severity >= turbo::min_log_level();
}

// -----------------------------------------------------------------------------
// Googletest Death Test Predicates
// -----------------------------------------------------------------------------

#if GTEST_HAS_DEATH_TEST

bool DiedOfFatal(int exit_status) {
#if defined(_WIN32)
  // Depending on NDEBUG and (configuration?) MSVC's abort either results
  // in error code 3 (SIGABRT) or error code 0x80000003 (breakpoint
  // triggered).
  return ::testing::ExitedWithCode(3)(exit_status & 0x7fffffff);
#elif defined(__Fuchsia__)
  // The Fuchsia death test implementation kill()'s the process when it detects
  // an exception, so it should exit with the corresponding code. See
  // FuchsiaDeathTest::Wait().
  return ::testing::ExitedWithCode(ZX_TASK_RETCODE_SYSCALL_KILL)(exit_status);
#elif defined(__ANDROID__) && defined(__aarch64__)
  // These are all run under a qemu config that eats died-due-to-signal exit
  // statuses.
  return true;
#else
  return ::testing::KilledBySignal(SIGABRT)(exit_status);
#endif
}

bool DiedOfQFatal(int exit_status) {
  return ::testing::ExitedWithCode(1)(exit_status);
}

#endif

// -----------------------------------------------------------------------------
// Helper for Log initialization in test
// -----------------------------------------------------------------------------

void LogTestEnvironment::SetUp() {
  if (!turbo::log_internal::IsInitialized()) {
    turbo::initialize_log();
  }
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
