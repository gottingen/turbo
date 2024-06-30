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

#include <turbo/log/flags.h>

#include <stddef.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/marshalling.h>
#include <turbo/flags/validator.h>
#include <turbo/log/globals.h>
#include <turbo/log/internal/config.h>
#include <turbo/log/internal/vlog_config.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/string_view.h>
#include <turbo/container/flat_hash_set.h>

namespace turbo {
    namespace log_internal {
        namespace {

            void SyncLoggingFlags() {
                turbo::set_flag(&FLAGS_min_log_level, static_cast<int>(turbo::min_log_level()));
                turbo::set_flag(&FLAGS_log_with_prefix, turbo::should_prepend_log_prefix());
            }

            bool RegisterSyncLoggingFlags() {
                log_internal::SetLoggingGlobalsListener(&SyncLoggingFlags);
                return true;
            }

            TURBO_ATTRIBUTE_UNUSED const bool unused = RegisterSyncLoggingFlags();

            template<typename T>
            T GetFromEnv(const char *varname, T dflt) {
                const char *val = ::getenv(varname);
                if (val != nullptr) {
                    std::string err;
                    TURBO_INTERNAL_CHECK(turbo::parse_flag(val, &dflt, &err), err.c_str());
                }
                return dflt;
            }

            constexpr turbo::LogSeverityAtLeast StderrThresholdDefault() {
                return turbo::LogSeverityAtLeast::kError;
            }

        }  // namespace
    }  // namespace log_internal
}  // namespace turbo

static turbo::flat_hash_set<int> LogSeverityAtLeastSet = {
        static_cast<int>(turbo::LogSeverityAtLeast::kInfo),
        static_cast<int>(turbo::LogSeverityAtLeast::kWarning),
        static_cast<int>(turbo::LogSeverityAtLeast::kError),
        static_cast<int>(turbo::LogSeverityAtLeast::kFatal),
        static_cast<int>(turbo::LogSeverityAtLeast::kInfinity),
};

TURBO_FLAG(int, stderr_threshold,
           static_cast<int>(turbo::log_internal::StderrThresholdDefault()),
           "Log messages at or above this threshold level are copied to stderr.")
        .on_validate(turbo::InSetValidator<int, LogSeverityAtLeastSet>::validate)
        .on_update([]() noexcept {
            turbo::log_internal::RawSetStderrThreshold(
                    static_cast<turbo::LogSeverityAtLeast>(
                            turbo::get_flag(FLAGS_stderr_threshold)));
        });

TURBO_FLAG(int, min_log_level, static_cast<int>(turbo::LogSeverityAtLeast::kInfo),
           "Messages logged at a lower level than this don't actually "
           "get logged anywhere")
        .on_validate(turbo::InSetValidator<int, LogSeverityAtLeastSet>::validate)
        .on_update([]() noexcept {
            turbo::log_internal::RawSetMinLogLevel(
                    static_cast<turbo::LogSeverityAtLeast>(
                            turbo::get_flag(FLAGS_min_log_level)));
        });

TURBO_FLAG(std::string, backtrace_log_at, "",
           "Emit a backtrace when logging at file:linenum.")
.on_update([]() noexcept {
    const std::string backtrace_log_at =
            turbo::get_flag(FLAGS_backtrace_log_at);
    if (backtrace_log_at.empty()) {
        turbo::clear_log_backtrace_location();
        return;
    }

    const size_t last_colon = backtrace_log_at.rfind(':');
    if (last_colon == backtrace_log_at.npos) {
        turbo::clear_log_backtrace_location();
        return;
    }

    const std::string_view file =
            std::string_view(backtrace_log_at).substr(0, last_colon);
    int line;
    if (!turbo::simple_atoi(
            std::string_view(backtrace_log_at).substr(last_colon + 1),
            &line)) {
        turbo::clear_log_backtrace_location();
        return;
    }
    turbo::set_log_backtrace_location(file, line);
});

TURBO_FLAG(bool, log_with_prefix, true,
           "prepend the log prefix to the start of each log line")
.on_update([]() noexcept {
    turbo::log_internal::RawEnableLogPrefix(turbo::get_flag(FLAGS_log_with_prefix));
});

TURBO_FLAG(int, verbosity, 0,
           "Show all VLOG(m) messages for m <= this. Overridable by --vlog_module.")
        .on_validate(turbo::GtValidator<int, 0>::validate)
        .on_update([]() noexcept {
            turbo::log_internal::UpdateGlobalVLogLevel(turbo::get_flag(FLAGS_verbosity));
        });

TURBO_FLAG(
        std::string, vlog_module, "",
        "per-module log verbosity level."
        " Argument is a comma-separated list of <module name>=<log level>."
        " <module name> is a glob pattern, matched against the filename base"
        " (that is, name ignoring .cc/.h./-inl.h)."
        " A pattern without slashes matches just the file name portion, otherwise"
        " the whole file path below the workspace root"
        " (still without .cc/.h./-inl.h) is matched."
        " ? and * in the glob pattern match any single or sequence of characters"
        " respectively including slashes."
        " <log level> overrides any value given by --verbosity.")
.on_update([]() noexcept {
    turbo::log_internal::UpdateVModule(turbo::get_flag(FLAGS_vlog_module));
});

TURBO_FLAG(std::string, log_base_filename, "",
           "The base filename for the log files. like /path/to/log_file.log");

TURBO_FLAG(int, log_rotation_hour, 2, "The hour to rotate the log file.");

TURBO_FLAG(int, log_rotation_minute, 30, "The minute to rotate the log file.");

TURBO_FLAG(int, log_check_interval_s, 60, "The interval to check the log file.");

TURBO_FLAG(bool, log_truncate, false, "Truncate the log file.");

TURBO_FLAG(int, log_max_files, 0, "The max files to keep.");

TURBO_FLAG(int, log_max_file_size, 100, "The max file size to rotate. unit is MB.");

TURBO_FLAG(int, log_type, 0, "The log type corresponding to LogSinkType."
                             " 0: console log"
                             " 1: daily log file"
                             " 2: hourly log file"
                             " 3: rotating log file");
