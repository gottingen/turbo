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

#include <turbo/log/internal/globals.h>

#include <atomic>
#include <cstdio>

#if defined(__EMSCRIPTEN__)
#include <emscripten/console.h>
#endif

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/log_severity.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/strip.h>
#include <turbo/time/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

namespace {
// Keeps track of whether Logging initialization is finalized.
// Log messages generated before that will go to stderr.
TURBO_CONST_INIT std::atomic<bool> logging_initialized(false);

// The TimeZone used for logging. This may only be set once.
TURBO_CONST_INIT std::atomic<turbo::TimeZone*> timezone_ptr{nullptr};

// If true, the logging library will symbolize stack in fatal messages
TURBO_CONST_INIT std::atomic<bool> symbolize_stack_trace(true);

// Specifies maximum number of stack frames to report in fatal messages.
TURBO_CONST_INIT std::atomic<int> max_frames_in_stack_trace(64);

TURBO_CONST_INIT std::atomic<bool> exit_on_dfatal(true);
TURBO_CONST_INIT std::atomic<bool> suppress_sigabort_trace(false);
}  // namespace

bool IsInitialized() {
  return logging_initialized.load(std::memory_order_acquire);
}

void SetInitialized() {
  logging_initialized.store(true, std::memory_order_release);
}

void WriteToStderr(turbo::string_view message, turbo::LogSeverity severity) {
  if (message.empty()) return;
#if defined(__EMSCRIPTEN__)
  // In WebAssembly, bypass filesystem emulation via fwrite.
  // Skip a trailing newline character as emscripten_errn adds one itself.
  const auto message_minus_newline = turbo::StripSuffix(message, "\n");
  // emscripten_errn was introduced in 3.1.41 but broken in standalone mode
  // until 3.1.43.
#if TURBO_INTERNAL_EMSCRIPTEN_VERSION >= 3001043
  emscripten_errn(message_minus_newline.data(), message_minus_newline.size());
#else
  std::string null_terminated_message(message_minus_newline);
  _emscripten_err(null_terminated_message.c_str());
#endif
#else
  // Avoid using std::cerr from this module since we may get called during
  // exit code, and cerr may be partially or fully destroyed by then.
  std::fwrite(message.data(), message.size(), 1, stderr);
#endif

#if defined(_WIN64) || defined(_WIN32) || defined(_WIN16)
  // C99 requires stderr to not be fully-buffered by default (7.19.3.7), but
  // MS CRT buffers it anyway, so we must `fflush` to ensure the string hits
  // the console/file before the program dies (and takes the libc buffers
  // with it).
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/stream-i-o
  if (severity >= turbo::LogSeverity::kWarning) {
    std::fflush(stderr);
  }
#else
  // Avoid unused parameter warning in this branch.
  (void)severity;
#endif
}

void SetTimeZone(turbo::TimeZone tz) {
  turbo::TimeZone* expected = nullptr;
  turbo::TimeZone* new_tz = new turbo::TimeZone(tz);
  // timezone_ptr can only be set once, otherwise new_tz is leaked.
  if (!timezone_ptr.compare_exchange_strong(expected, new_tz,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
    TURBO_RAW_LOG(FATAL,
                 "turbo::log_internal::SetTimeZone() has already been called");
  }
}

const turbo::TimeZone* TimeZone() {
  return timezone_ptr.load(std::memory_order_acquire);
}

bool ShouldSymbolizeLogStackTrace() {
  return symbolize_stack_trace.load(std::memory_order_acquire);
}

void EnableSymbolizeLogStackTrace(bool on_off) {
  symbolize_stack_trace.store(on_off, std::memory_order_release);
}

int MaxFramesInLogStackTrace() {
  return max_frames_in_stack_trace.load(std::memory_order_acquire);
}

void SetMaxFramesInLogStackTrace(int max_num_frames) {
  max_frames_in_stack_trace.store(max_num_frames, std::memory_order_release);
}

bool ExitOnDFatal() { return exit_on_dfatal.load(std::memory_order_acquire); }

void SetExitOnDFatal(bool on_off) {
  exit_on_dfatal.store(on_off, std::memory_order_release);
}

bool SuppressSigabortTrace() {
  return suppress_sigabort_trace.load(std::memory_order_acquire);
}

bool SetSuppressSigabortTrace(bool on_off) {
  return suppress_sigabort_trace.exchange(on_off);
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
