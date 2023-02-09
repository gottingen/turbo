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

#include "turbo/log/internal/flags.h"

#include <stddef.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include "turbo/base/log_severity.h"
#include "turbo/flags/flag.h"
#include "turbo/flags/marshalling.h"
#include "turbo/log/globals.h"
#include "turbo/log/internal/config.h"
#include "turbo/platform/port.h"
#include "turbo/strings/numbers.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
namespace {

void SyncLoggingFlags() {
  turbo::SetFlag(&FLAGS_minloglevel, static_cast<int>(turbo::MinLogLevel()));
  turbo::SetFlag(&FLAGS_log_prefix, turbo::ShouldPrependLogPrefix());
}

bool RegisterSyncLoggingFlags() {
  log_internal::SetLoggingGlobalsListener(&SyncLoggingFlags);
  return true;
}

TURBO_ATTRIBUTE_UNUSED const bool unused = RegisterSyncLoggingFlags();

template <typename T>
T GetFromEnv(const char* varname, T dflt) {
  const char* val = ::getenv(varname);
  if (val != nullptr) {
    std::string err;
    TURBO_INTERNAL_CHECK(turbo::ParseFlag(val, &dflt, &err), err.c_str());
  }
  return dflt;
}

constexpr turbo::LogSeverityAtLeast StderrThresholdDefault() {
  return turbo::LogSeverityAtLeast::kError;
}

}  // namespace
}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

TURBO_FLAG(int, stderrthreshold,
          static_cast<int>(turbo::log_internal::StderrThresholdDefault()),
          "Log messages at or above this threshold level are copied to stderr.")
    .OnUpdate([] {
      turbo::log_internal::RawSetStderrThreshold(
          static_cast<turbo::LogSeverityAtLeast>(
              turbo::GetFlag(FLAGS_stderrthreshold)));
    });

TURBO_FLAG(int, minloglevel, static_cast<int>(turbo::LogSeverityAtLeast::kInfo),
          "Messages logged at a lower level than this don't actually "
          "get logged anywhere")
    .OnUpdate([] {
      turbo::log_internal::RawSetMinLogLevel(
          static_cast<turbo::LogSeverityAtLeast>(
              turbo::GetFlag(FLAGS_minloglevel)));
    });

TURBO_FLAG(std::string, log_backtrace_at, "",
          "Emit a backtrace when logging at file:linenum.")
    .OnUpdate([] {
      const std::string log_backtrace_at =
          turbo::GetFlag(FLAGS_log_backtrace_at);
      if (log_backtrace_at.empty()) return;

      const size_t last_colon = log_backtrace_at.rfind(':');
      if (last_colon == log_backtrace_at.npos) return;

      const turbo::string_view file =
          turbo::string_view(log_backtrace_at).substr(0, last_colon);
      int line;
      if (turbo::SimpleAtoi(
              turbo::string_view(log_backtrace_at).substr(last_colon + 1),
              &line)) {
        turbo::SetLogBacktraceLocation(file, line);
      }
    });

TURBO_FLAG(bool, log_prefix, true,
          "Prepend the log prefix to the start of each log line")
    .OnUpdate([] {
      turbo::log_internal::RawEnableLogPrefix(turbo::GetFlag(FLAGS_log_prefix));
    });
