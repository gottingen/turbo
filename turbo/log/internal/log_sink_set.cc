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

#include "turbo/log/internal/log_sink_set.h"

#ifndef TURBO_HAVE_THREAD_LOCAL
#include <pthread.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <vector>

#include "turbo/base/attributes.h"
#include "turbo/base/call_once.h"
#include "turbo/base/config.h"
#include "turbo/base/internal/raw_logging.h"
#include "turbo/base/log_severity.h"
#include "turbo/base/thread_annotations.h"
#include "turbo/cleanup/cleanup.h"
#include "turbo/log/globals.h"
#include "turbo/log/internal/config.h"
#include "turbo/log/internal/globals.h"
#include "turbo/log/log_entry.h"
#include "turbo/log/log_sink.h"
#include "turbo/strings/string_view.h"
#include "turbo/synchronization/mutex.h"
#include "turbo/types/span.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
namespace {

// Returns a mutable reference to a thread-local variable that should be true if
// a globally-registered `LogSink`'s `Send()` is currently being invoked on this
// thread.
bool& ThreadIsLoggingStatus() {
#ifdef TURBO_HAVE_THREAD_LOCAL
  TURBO_CONST_INIT thread_local bool thread_is_logging = false;
  return thread_is_logging;
#else
  TURBO_CONST_INIT static pthread_key_t thread_is_logging_key;
  static const bool unused = [] {
    if (pthread_key_create(&thread_is_logging_key, [](void* data) {
          delete reinterpret_cast<bool*>(data);
        })) {
      perror("pthread_key_create failed!");
      abort();
    }
    return true;
  }();
  (void)unused;  // Fixes -wunused-variable warning
  bool* thread_is_logging_ptr =
      reinterpret_cast<bool*>(pthread_getspecific(thread_is_logging_key));

  if (TURBO_PREDICT_FALSE(!thread_is_logging_ptr)) {
    thread_is_logging_ptr = new bool{false};
    if (pthread_setspecific(thread_is_logging_key, thread_is_logging_ptr)) {
      perror("pthread_setspecific failed");
      abort();
    }
  }
  return *thread_is_logging_ptr;
#endif
}

class StderrLogSink final : public LogSink {
 public:
  ~StderrLogSink() override = default;

  void Send(const turbo::LogEntry& entry) override {
    if (entry.log_severity() < turbo::StderrThreshold() &&
        turbo::log_internal::IsInitialized()) {
      return;
    }

    TURBO_CONST_INIT static turbo::once_flag warn_if_not_initialized;
    turbo::call_once(warn_if_not_initialized, []() {
      if (turbo::log_internal::IsInitialized()) return;
      const char w[] =
          "WARNING: All log messages before turbo::InitializeLog() is called"
          " are written to STDERR\n";
      turbo::log_internal::WriteToStderr(w, turbo::LogSeverity::kWarning);
    });

    if (!entry.stacktrace().empty()) {
      turbo::log_internal::WriteToStderr(entry.stacktrace(),
                                        entry.log_severity());
    } else {
      // TODO(b/226937039): do this outside else condition once we avoid
      // ReprintFatalMessage
      turbo::log_internal::WriteToStderr(
          entry.text_message_with_prefix_and_newline(), entry.log_severity());
    }
  }
};

#if defined(__ANDROID__)
class AndroidLogSink final : public LogSink {
 public:
  ~AndroidLogSink() override = default;

  void Send(const turbo::LogEntry& entry) override {
    const int level = AndroidLogLevel(entry);
    // TODO(b/37587197): make the tag ("native") configurable.
    __android_log_write(level, "native",
                        entry.text_message_with_prefix_and_newline_c_str());
    if (entry.log_severity() == turbo::LogSeverity::kFatal)
      __android_log_write(ANDROID_LOG_FATAL, "native", "terminating.\n");
  }

 private:
  static int AndroidLogLevel(const turbo::LogEntry& entry) {
    switch (entry.log_severity()) {
      case turbo::LogSeverity::kFatal:
        return ANDROID_LOG_FATAL;
      case turbo::LogSeverity::kError:
        return ANDROID_LOG_ERROR;
      case turbo::LogSeverity::kWarning:
        return ANDROID_LOG_WARN;
      default:
        if (entry.verbosity() >= 2) return ANDROID_LOG_VERBOSE;
        if (entry.verbosity() == 1) return ANDROID_LOG_DEBUG;
        return ANDROID_LOG_INFO;
    }
  }
};
#endif  // !defined(__ANDROID__)

#if defined(_WIN32)
class WindowsDebuggerLogSink final : public LogSink {
 public:
  ~WindowsDebuggerLogSink() override = default;

  void Send(const turbo::LogEntry& entry) override {
    if (entry.log_severity() < turbo::StderrThreshold() &&
        turbo::log_internal::IsInitialized()) {
      return;
    }
    ::OutputDebugStringA(entry.text_message_with_prefix_and_newline_c_str());
  }
};
#endif  // !defined(_WIN32)

class GlobalLogSinkSet final {
 public:
  GlobalLogSinkSet() {
#if defined(__myriad2__) || defined(__Fuchsia__)
    // myriad2 and Fuchsia do not log to stderr by default.
#else
    static StderrLogSink* stderr_log_sink = new StderrLogSink;
    AddLogSink(stderr_log_sink);
#endif
#ifdef __ANDROID__
    static AndroidLogSink* android_log_sink = new AndroidLogSink;
    AddLogSink(android_log_sink);
#endif
#if defined(_WIN32)
    static WindowsDebuggerLogSink* debugger_log_sink =
        new WindowsDebuggerLogSink;
    AddLogSink(debugger_log_sink);
#endif  // !defined(_WIN32)
  }

  void LogToSinks(const turbo::LogEntry& entry,
                  turbo::Span<turbo::LogSink*> extra_sinks, bool extra_sinks_only)
      TURBO_LOCKS_EXCLUDED(guard_) {
    SendToSinks(entry, extra_sinks);

    if (!extra_sinks_only) {
      if (ThreadIsLoggingToLogSink()) {
        turbo::log_internal::WriteToStderr(
            entry.text_message_with_prefix_and_newline(), entry.log_severity());
      } else {
        turbo::ReaderMutexLock global_sinks_lock(&guard_);
        ThreadIsLoggingStatus() = true;
        // Ensure the "thread is logging" status is reverted upon leaving the
        // scope even in case of exceptions.
        auto status_cleanup =
            turbo::MakeCleanup([] { ThreadIsLoggingStatus() = false; });
        SendToSinks(entry, turbo::MakeSpan(sinks_));
      }
    }
  }

  void AddLogSink(turbo::LogSink* sink) TURBO_LOCKS_EXCLUDED(guard_) {
    {
      turbo::WriterMutexLock global_sinks_lock(&guard_);
      auto pos = std::find(sinks_.begin(), sinks_.end(), sink);
      if (pos == sinks_.end()) {
        sinks_.push_back(sink);
        return;
      }
    }
    TURBO_INTERNAL_LOG(FATAL, "Duplicate log sinks are not supported");
  }

  void RemoveLogSink(turbo::LogSink* sink) TURBO_LOCKS_EXCLUDED(guard_) {
    {
      turbo::WriterMutexLock global_sinks_lock(&guard_);
      auto pos = std::find(sinks_.begin(), sinks_.end(), sink);
      if (pos != sinks_.end()) {
        sinks_.erase(pos);
        return;
      }
    }
    TURBO_INTERNAL_LOG(FATAL, "Mismatched log sink being removed");
  }

  void FlushLogSinks() TURBO_LOCKS_EXCLUDED(guard_) {
    if (ThreadIsLoggingToLogSink()) {
      // The thread_local condition demonstrates that we're already holding the
      // lock in order to iterate over `sinks_` for dispatch.  The thread-safety
      // annotations don't know this, so we use `TURBO_NO_THREAD_SAFETY_ANALYSIS`
      guard_.AssertReaderHeld();
      FlushLogSinksLocked();
    } else {
      turbo::ReaderMutexLock global_sinks_lock(&guard_);
      // In case if LogSink::Flush overload decides to log
      ThreadIsLoggingStatus() = true;
      // Ensure the "thread is logging" status is reverted upon leaving the
      // scope even in case of exceptions.
      auto status_cleanup =
          turbo::MakeCleanup([] { ThreadIsLoggingStatus() = false; });
      FlushLogSinksLocked();
    }
  }

 private:
  void FlushLogSinksLocked() TURBO_SHARED_LOCKS_REQUIRED(guard_) {
    for (turbo::LogSink* sink : sinks_) {
      sink->Flush();
    }
  }

  // Helper routine for LogToSinks.
  static void SendToSinks(const turbo::LogEntry& entry,
                          turbo::Span<turbo::LogSink*> sinks) {
    for (turbo::LogSink* sink : sinks) {
      sink->Send(entry);
    }
  }

  using LogSinksSet = std::vector<turbo::LogSink*>;
  turbo::Mutex guard_;
  LogSinksSet sinks_ TURBO_GUARDED_BY(guard_);
};

// Returns reference to the global LogSinks set.
GlobalLogSinkSet& GlobalSinks() {
  static GlobalLogSinkSet* global_sinks = new GlobalLogSinkSet;
  return *global_sinks;
}

}  // namespace

bool ThreadIsLoggingToLogSink() { return ThreadIsLoggingStatus(); }

void LogToSinks(const turbo::LogEntry& entry,
                turbo::Span<turbo::LogSink*> extra_sinks, bool extra_sinks_only) {
  log_internal::GlobalSinks().LogToSinks(entry, extra_sinks, extra_sinks_only);
}

void AddLogSink(turbo::LogSink* sink) {
  log_internal::GlobalSinks().AddLogSink(sink);
}

void RemoveLogSink(turbo::LogSink* sink) {
  log_internal::GlobalSinks().RemoveLogSink(sink);
}

void FlushLogSinks() { log_internal::GlobalSinks().FlushLogSinks(); }

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
