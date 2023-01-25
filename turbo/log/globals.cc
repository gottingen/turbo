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

#include "turbo/log/globals.h"

#include <stddef.h>
#include <stdint.h>

#include <atomic>

#include "turbo/platform/attributes.h"
#include "turbo/platform/config.h"
#include "turbo/platform/internal/atomic_hook.h"
#include "turbo/platform/log_severity.h"
#include "turbo/hash/hash.h"
#include "turbo/strings/string_view.h"

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

bool ShouldLogBacktraceAt(turbo::string_view file, int line) {
  const size_t flag_hash =
      log_backtrace_at_hash.load(std::memory_order_acquire);

  return flag_hash != 0 && flag_hash == HashSiteForLogBacktraceAt(file, line);
}

}  // namespace log_internal

void SetLogBacktraceLocation(turbo::string_view file, int line) {
  log_backtrace_at_hash.store(HashSiteForLogBacktraceAt(file, line),
                              std::memory_order_release);
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
