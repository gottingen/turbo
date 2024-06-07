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
// File: log/internal/test_actions.h
// -----------------------------------------------------------------------------
//
// This file declares Googletest's actions used in the Turbo Logging library
// unit tests.

#ifndef TURBO_LOG_INTERNAL_TEST_ACTIONS_H_
#define TURBO_LOG_INTERNAL_TEST_ACTIONS_H_

#include <iostream>
#include <ostream>
#include <string>

#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/log_entry.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// These actions are used by the child process in a death test.
//
// Expectations set in the child cannot cause test failure in the parent
// directly.  Instead, the child can use these actions with
// `EXPECT_CALL`/`WillOnce` and `ON_CALL`/`WillByDefault` (for unexpected calls)
// to write messages to stderr that the parent can match against.
struct WriteToStderr final {
  explicit WriteToStderr(std::string_view m) : message(m) {}
  std::string message;

  template <typename... Args>
  void operator()(const Args&...) const {
    std::cerr << message << std::endl;
  }
};

struct WriteToStderrWithFilename final {
  explicit WriteToStderrWithFilename(std::string_view m) : message(m) {}

  std::string message;

  void operator()(const turbo::LogEntry& entry) const;
};

struct WriteEntryToStderr final {
  explicit WriteEntryToStderr(std::string_view m) : message(m) {}

  std::string message = "";

  void operator()(const turbo::LogEntry& entry) const;
  void operator()(turbo::LogSeverity, std::string_view,
                  std::string_view) const;
};

// See the documentation for `DeathTestValidateExpectations` above.
// `DeathTestExpectedLogging` should be used once in a given death test, and the
// applicable severity level is the one that should be passed to
// `DeathTestValidateExpectations`.
inline WriteEntryToStderr DeathTestExpectedLogging() {
  return WriteEntryToStderr{"Mock received expected entry:"};
}

// `DeathTestUnexpectedLogging` should be used zero or more times to mark
// messages that should not hit the logs as the process dies.
inline WriteEntryToStderr DeathTestUnexpectedLogging() {
  return WriteEntryToStderr{"Mock received unexpected entry:"};
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_TEST_ACTIONS_H_
