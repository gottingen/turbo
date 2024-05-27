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
// File: log/globals.h
// -----------------------------------------------------------------------------
//
// This header declares global logging library configuration knobs.

#ifndef TURBO_LOG_GLOBALS_H_
#define TURBO_LOG_GLOBALS_H_

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/vlog_config.h>
#include <turbo/strings/string_view.h>

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
// Users can request an existing `LOG` statement, specified by file and line
// number, to also include a backtrace when logged.

// ShouldLogBacktraceAt()
//
// Returns true if we should log a backtrace at the specified location.
namespace log_internal {
TURBO_MUST_USE_RESULT bool ShouldLogBacktraceAt(turbo::string_view file,
                                               int line);
}  // namespace log_internal

// SetLogBacktraceLocation()
//
// Sets the location the backtrace should be logged at.  If the specified
// location isn't a `LOG` statement, the effect will be the same as
// `ClearLogBacktraceLocation` (but less efficient).
void SetLogBacktraceLocation(turbo::string_view file, int line);

// ClearLogBacktraceLocation()
//
// Clears the set location so that backtraces will no longer be logged at it.
void ClearLogBacktraceLocation();

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

//------------------------------------------------------------------------------
// Set Global VLOG Level
//------------------------------------------------------------------------------
//
// Sets the global `(TURBO_)VLOG(_IS_ON)` level to `log_level`.  This level is
// applied to any sites whose filename doesn't match any `module_pattern`.
// Returns the prior value.
inline int SetGlobalVLogLevel(int log_level) {
  return turbo::log_internal::UpdateGlobalVLogLevel(log_level);
}

//------------------------------------------------------------------------------
// Set VLOG Level
//------------------------------------------------------------------------------
//
// Sets `(TURBO_)VLOG(_IS_ON)` level for `module_pattern` to `log_level`.  This
// allows programmatic control of what is normally set by the --vmodule flag.
// Returns the level that previously applied to `module_pattern`.
inline int SetVLogLevel(turbo::string_view module_pattern, int log_level) {
  return turbo::log_internal::PrependVModule(module_pattern, log_level);
}

//------------------------------------------------------------------------------
// Configure Android Native Log Tag
//------------------------------------------------------------------------------
//
// The logging library forwards to the Android system log API when built for
// Android.  That API takes a string "tag" value in addition to a message and
// severity level.  The tag is used to identify the source of messages and to
// filter them.  This library uses the tag "native" by default.

// SetAndroidNativeTag()
//
// Stores a copy of the string pointed to by `tag` and uses it as the Android
// logging tag thereafter. `tag` must not be null.
// This function must not be called more than once!
void SetAndroidNativeTag(const char* tag);

namespace log_internal {
// GetAndroidNativeTag()
//
// Returns the configured Android logging tag.
const char* GetAndroidNativeTag();
}  // namespace log_internal

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
