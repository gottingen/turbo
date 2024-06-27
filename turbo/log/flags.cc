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

#include <turbo/log/internal/flags.h>

#include <stddef.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/marshalling.h>
#include <turbo/log/globals.h>
#include <turbo/log/internal/config.h>
#include <turbo/log/internal/vlog_config.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/string_view.h>

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

TURBO_FLAG(int, stderr_threshold,
           static_cast<int>(turbo::log_internal::StderrThresholdDefault()),
           "Log messages at or above this threshold level are copied to stderr.")
.on_update([]() noexcept {
    turbo::log_internal::RawSetStderrThreshold(
            static_cast<turbo::LogSeverityAtLeast>(
                    turbo::get_flag(FLAGS_stderr_threshold)));
});

TURBO_FLAG(int, min_log_level, static_cast<int>(turbo::LogSeverityAtLeast::kInfo),
           "Messages logged at a lower level than this don't actually "
           "get logged anywhere")
        .on_validate([](std::string_view value, std::string *err) noexcept -> bool {
            int level;
            auto r = turbo::parse_flag(value, &level, err);
            if (!r) {
                return false;
            }
            if (level < static_cast<int>(turbo::LogSeverityAtLeast::kInfo) ||
                level > static_cast<int>(turbo::LogSeverityAtLeast::kFatal)) {
                *err = "min_log_level must be in the range INFO(0) to FATAL(3)";
                return false;
            }
            return true;
        })
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
.on_validate([](std::string_view value, std::string *err) noexcept -> bool {
    int level;
    auto r = turbo::parse_flag(value, &level, err);
    if (!r) {
        return false;
    }
    if (level < 0) {
        *err = "verbosity must be non-negative";
        return false;
    }
    return true;
})
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
