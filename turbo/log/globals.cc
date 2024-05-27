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

#include <turbo/log/globals.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/atomic_hook.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/log_severity.h>
#include <turbo/hash/hash.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

// These atomics represent logging library configuration.
// Integer types are used instead of turbo::LogSeverity to ensure that a
// lock-free std::atomic is used when possible.
TURBO_CONST_INIT std::atomic<int> min_log_level{
    static_cast<int>(turbo::LogSeverityAtLeast::kInfo)};
TURBO_CONST_INIT std::atomic<int> stderrthreshold{
    static_cast<int>(turbo::LogSeverityAtLeast::kError)};
// We evaluate this value as a hash comparison to avoid having to
// hold a mutex or make a copy (to access the value of a string-typed flag) in
// very hot codepath.
TURBO_CONST_INIT std::atomic<size_t> log_backtrace_at_hash{0};
TURBO_CONST_INIT std::atomic<bool> prepend_log_prefix{true};

constexpr char kDefaultAndroidTag[] = "native";
TURBO_CONST_INIT std::atomic<const char*> android_log_tag{kDefaultAndroidTag};

TURBO_INTERNAL_ATOMIC_HOOK_ATTRIBUTES
turbo::base_internal::AtomicHook<log_internal::LoggingGlobalsListener>
    logging_globals_listener;

size_t HashSiteForLogBacktraceAt(turbo::string_view file, int line) {
  return turbo::HashOf(file, line);
}

void TriggerLoggingGlobalsListener() {
  auto* listener = logging_globals_listener.Load();
  if (listener != nullptr) listener();
}

}  // namespace

namespace log_internal {

void RawSetMinLogLevel(turbo::LogSeverityAtLeast severity) {
  min_log_level.store(static_cast<int>(severity), std::memory_order_release);
}

void RawSetStderrThreshold(turbo::LogSeverityAtLeast severity) {
  stderrthreshold.store(static_cast<int>(severity), std::memory_order_release);
}

void RawEnableLogPrefix(bool on_off) {
  prepend_log_prefix.store(on_off, std::memory_order_release);
}

void SetLoggingGlobalsListener(LoggingGlobalsListener l) {
  logging_globals_listener.Store(l);
}

}  // namespace log_internal

turbo::LogSeverityAtLeast MinLogLevel() {
  return static_cast<turbo::LogSeverityAtLeast>(
      min_log_level.load(std::memory_order_acquire));
}

void SetMinLogLevel(turbo::LogSeverityAtLeast severity) {
  log_internal::RawSetMinLogLevel(severity);
  TriggerLoggingGlobalsListener();
}

namespace log_internal {

ScopedMinLogLevel::ScopedMinLogLevel(turbo::LogSeverityAtLeast severity)
    : saved_severity_(turbo::MinLogLevel()) {
  turbo::SetMinLogLevel(severity);
}
ScopedMinLogLevel::~ScopedMinLogLevel() {
  turbo::SetMinLogLevel(saved_severity_);
}

}  // namespace log_internal

turbo::LogSeverityAtLeast StderrThreshold() {
  return static_cast<turbo::LogSeverityAtLeast>(
      stderrthreshold.load(std::memory_order_acquire));
}

void SetStderrThreshold(turbo::LogSeverityAtLeast severity) {
  log_internal::RawSetStderrThreshold(severity);
  TriggerLoggingGlobalsListener();
}

ScopedStderrThreshold::ScopedStderrThreshold(turbo::LogSeverityAtLeast severity)
    : saved_severity_(turbo::StderrThreshold()) {
  turbo::SetStderrThreshold(severity);
}

ScopedStderrThreshold::~ScopedStderrThreshold() {
  turbo::SetStderrThreshold(saved_severity_);
}

namespace log_internal {

const char* GetAndroidNativeTag() {
  return android_log_tag.load(std::memory_order_acquire);
}

}  // namespace log_internal

void SetAndroidNativeTag(const char* tag) {
  TURBO_CONST_INIT static std::atomic<const std::string*> user_log_tag(nullptr);
  TURBO_INTERNAL_CHECK(tag, "tag must be non-null.");

  const std::string* tag_str = new std::string(tag);
  TURBO_INTERNAL_CHECK(
      android_log_tag.exchange(tag_str->c_str(), std::memory_order_acq_rel) ==
          kDefaultAndroidTag,
      "SetAndroidNativeTag() must only be called once per process!");
  user_log_tag.store(tag_str, std::memory_order_relaxed);
}

namespace log_internal {

bool ShouldLogBacktraceAt(turbo::string_view file, int line) {
  const size_t flag_hash =
      log_backtrace_at_hash.load(std::memory_order_relaxed);

  return flag_hash != 0 && flag_hash == HashSiteForLogBacktraceAt(file, line);
}

}  // namespace log_internal

void SetLogBacktraceLocation(turbo::string_view file, int line) {
  log_backtrace_at_hash.store(HashSiteForLogBacktraceAt(file, line),
                              std::memory_order_relaxed);
}

void ClearLogBacktraceLocation() {
  log_backtrace_at_hash.store(0, std::memory_order_relaxed);
}

bool ShouldPrependLogPrefix() {
  return prepend_log_prefix.load(std::memory_order_acquire);
}

void EnableLogPrefix(bool on_off) {
  log_internal::RawEnableLogPrefix(on_off);
  TriggerLoggingGlobalsListener();
}

TURBO_NAMESPACE_END
}  // namespace turbo
