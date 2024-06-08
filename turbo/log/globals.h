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

#pragma once

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/vlog_config.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    //------------------------------------------------------------------------------
    //  Minimum Log Level
    //------------------------------------------------------------------------------
    //
    // Messages logged at or above this severity are directed to all registered log
    // sinks or skipped otherwise. This parameter can also be modified using
    // command line flag --minloglevel.
    // See turbo/base/log_severity.h for descriptions of severity levels.

    // min_log_level()
    //
    // Returns the value of the Minimum Log Level parameter.
    // This function is async-signal-safe.
    TURBO_MUST_USE_RESULT turbo::LogSeverityAtLeast min_log_level();

    // set_min_log_level()
    //
    // Updates the value of Minimum Log Level parameter.
    // This function is async-signal-safe.
    void set_min_log_level(turbo::LogSeverityAtLeast severity);

    namespace log_internal {

        // ScopedMinLogLevel
        //
        // RAII type used to temporarily update the Min Log Level parameter.
        class ScopedMinLogLevel final {
        public:
            explicit ScopedMinLogLevel(turbo::LogSeverityAtLeast severity);

            ScopedMinLogLevel(const ScopedMinLogLevel &) = delete;

            ScopedMinLogLevel &operator=(const ScopedMinLogLevel &) = delete;

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

    // stderr_threshold()
    //
    // Returns the value of the Stderr Threshold parameter.
    // This function is async-signal-safe.
    TURBO_MUST_USE_RESULT turbo::LogSeverityAtLeast stderr_threshold();

    // set_stderr_threshold()
    //
    // Updates the Stderr Threshold parameter.
    // This function is async-signal-safe.
    void set_stderr_threshold(turbo::LogSeverityAtLeast severity);

    inline void set_stderr_threshold(turbo::LogSeverity severity) {
        turbo::set_stderr_threshold(static_cast<turbo::LogSeverityAtLeast>(severity));
    }

    // ScopedStderrThreshold
    //
    // RAII type used to temporarily update the Stderr Threshold parameter.
    class ScopedStderrThreshold final {
    public:
        explicit ScopedStderrThreshold(turbo::LogSeverityAtLeast severity);

        ScopedStderrThreshold(const ScopedStderrThreshold &) = delete;

        ScopedStderrThreshold &operator=(const ScopedStderrThreshold &) = delete;

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
        TURBO_MUST_USE_RESULT bool ShouldLogBacktraceAt(std::string_view file,
                                                        int line);
    }  // namespace log_internal

    // set_log_backtrace_location()
    //
    // Sets the location the backtrace should be logged at.  If the specified
    // location isn't a `LOG` statement, the effect will be the same as
    // `clear_log_backtrace_location` (but less efficient).
    void set_log_backtrace_location(std::string_view file, int line);

    // clear_log_backtrace_location()
    //
    // Clears the set location so that backtraces will no longer be logged at it.
    void clear_log_backtrace_location();

    //------------------------------------------------------------------------------
    // prepend Log Prefix
    //------------------------------------------------------------------------------
    //
    // This option tells the logging library that every logged message
    // should include the prefix (severity, date, time, PID, etc.)

    // should_prepend_log_prefix()
    //
    // Returns the value of the prepend Log Prefix option.
    // This function is async-signal-safe.
    TURBO_MUST_USE_RESULT bool should_prepend_log_prefix();

    // enable_log_prefix()
    //
    // Updates the value of the prepend Log Prefix option.
    // This function is async-signal-safe.
    void enable_log_prefix(bool on_off);

    //------------------------------------------------------------------------------
    // Set Global VLOG Level
    //------------------------------------------------------------------------------
    //
    // Sets the global `(TURBO_)VLOG(_IS_ON)` level to `log_level`.  This level is
    // applied to any sites whose filename doesn't match any `module_pattern`.
    // Returns the prior value.
    inline int set_global_vlog_level(int log_level) {
        return turbo::log_internal::UpdateGlobalVLogLevel(log_level);
    }

    //------------------------------------------------------------------------------
    // Set VLOG Level
    //------------------------------------------------------------------------------
    //
    // Sets `(TURBO_)VLOG(_IS_ON)` level for `module_pattern` to `log_level`.  This
    // allows programmatic control of what is normally set by the --vmodule flag.
    // Returns the level that previously applied to `module_pattern`.
    inline int set_vlog_level(std::string_view module_pattern, int log_level) {
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

    // set_android_native_tag()
    //
    // Stores a copy of the string pointed to by `tag` and uses it as the Android
    // logging tag thereafter. `tag` must not be null.
    // This function must not be called more than once!
    void set_android_native_tag(const char *tag);

    namespace log_internal {
        // GetAndroidNativeTag()
        //
        // Returns the configured Android logging tag.
        const char *GetAndroidNativeTag();
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
}  // namespace turbo
