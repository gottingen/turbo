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
// File: log/globals.h
// -----------------------------------------------------------------------------
//
// This header declares global logging library configuration knobs.

#ifndef TURBO_LOG_GLOBALS_H_
#define TURBO_LOG_GLOBALS_H_

#include "turbo/base/log_severity.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
//  Minimum Log Level
//------------------------------------------------------------------------------
//
// Messages logged at or above this severity are directed to all registered log
// sinks or skipped otherwise. This parameter can also be modified using
// command line flag --minloglevel.
// See turbo/base/log_severity.h for descriptions of severity levels.

// MinLogLevel()
//
// Returns the value of the Minimum Log Level parameter.
// This function is async-signal-safe.
TURBO_MUST_USE_RESULT turbo::LogSeverityAtLeast MinLogLevel();

// SetMinLogLevel()
//
// Updates the value of Minimum Log Level parameter.
// This function is async-signal-safe.
void SetMinLogLevel(turbo::LogSeverityAtLeast severity);

namespace log_internal {

// ScopedMinLogLevel
//
// RAII type used to temporarily update the Min Log Level parameter.
class ScopedMinLogLevel final {
 public:
  explicit ScopedMinLogLevel(turbo::LogSeverityAtLeast severity);
  ScopedMinLogLevel(const ScopedMinLogLevel&) = delete;
  ScopedMinLogLevel& operator=(const ScopedMinLogLevel&) = delete;
  ~ScopedMinLogLevel();

 private:
  turbo::LogSeverityAtLeast saved_severity_;
};

}  // namespace log_internal

//------------------------------------------------------------------------------
// Stderr Threshold
//------------------------------------------------------------------------------
//
// Messages logged at or above this level are directed to stderr in
// addition to other registered log sinks. This parameter can also be modified
// using command line flag --stderrthreshold.
// See turbo/base/log_severity.h for descriptions of severity levels.

// StderrThreshold()
//
// Returns the value of the Stderr Threshold parameter.
// This function is async-signal-safe.
TURBO_MUST_USE_RESULT turbo::LogSeverityAtLeast StderrThreshold();

// SetStderrThreshold()
//
// Updates the Stderr Threshold parameter.
// This function is async-signal-safe.
void SetStderrThreshold(turbo::LogSeverityAtLeast severity);
inline void SetStderrThreshold(turbo::LogSeverity severity) {
  turbo::SetStderrThreshold(static_cast<turbo::LogSeverityAtLeast>(severity));
}

// ScopedStderrThreshold
//
// RAII type used to temporarily update the Stderr Threshold parameter.
class ScopedStderrThreshold final {
 public:
  explicit ScopedStderrThreshold(turbo::LogSeverityAtLeast severity);
  ScopedStderrThreshold(const ScopedStderrThreshold&) = delete;
  ScopedStderrThreshold& operator=(const ScopedStderrThreshold&) = delete;
  ~ScopedStderrThreshold();

 private:
  turbo::LogSeverityAtLeast saved_severity_;
};

//------------------------------------------------------------------------------
// Log Backtrace At
//------------------------------------------------------------------------------
//
// Users can request backtrace to be logged at specific locations, specified
// by file and line number.

// ShouldLogBacktraceAt()
//
// Returns true if we should log a backtrace at the specified location.
namespace log_internal {
TURBO_MUST_USE_RESULT bool ShouldLogBacktraceAt(turbo::string_view file,
                                               int line);
}  // namespace log_internal

// SetLogBacktraceLocation()
//
// Sets the location the backtrace should be logged at.
void SetLogBacktraceLocation(turbo::string_view file, int line);

//------------------------------------------------------------------------------
// Prepend Log Prefix
//------------------------------------------------------------------------------
//
// This option tells the logging library that every logged message
// should include the prefix (severity, date, time, PID, etc.)

// ShouldPrependLogPrefix()
//
// Returns the value of the Prepend Log Prefix option.
// This function is async-signal-safe.
TURBO_MUST_USE_RESULT bool ShouldPrependLogPrefix();

// EnableLogPrefix()
//
// Updates the value of the Prepend Log Prefix option.
// This function is async-signal-safe.
void EnableLogPrefix(bool on_off);

namespace log_internal {

using LoggingGlobalsListener = void (*)();
void SetLoggingGlobalsListener(LoggingGlobalsListener l);

// Internal implementation for the setter routines. These are used
// to break circular dependencies between flags and globals. Each "Raw"
// routine corresponds to the non-"Raw" counterpart and used to set the
// configuration parameter directly without calling back to the listener.
void RawSetMinLogLevel(turbo::LogSeverityAtLeast severity);
void RawSetStderrThreshold(turbo::LogSeverityAtLeast severity);
void RawEnableLogPrefix(bool on_off);

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_GLOBALS_H_
