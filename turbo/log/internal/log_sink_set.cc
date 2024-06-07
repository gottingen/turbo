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

#include <turbo/log/internal/log_sink_set.h>

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

#include <turbo/base/attributes.h>
#include <turbo/base/call_once.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/log_severity.h>
#include <turbo/base/no_destructor.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/bootstrap/cleanup.h>
#include <turbo/log/globals.h>
#include <turbo/log/internal/config.h>
#include <turbo/log/internal/globals.h>
#include <turbo/log/log_entry.h>
#include <turbo/log/log_sink.h>
#include <turbo/strings/string_view.h>
#include <turbo/synchronization/mutex.h>
#include <turbo/container/span.h>

namespace turbo::log_internal {
    namespace {

        // Returns a mutable reference to a thread-local variable that should be true if
        // a globally-registered `LogSink`'s `Send()` is currently being invoked on this
        // thread.
        bool &thread_is_logging_status() {
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

            if (TURBO_UNLIKELY(!thread_is_logging_ptr)) {
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

            void Send(const turbo::LogEntry &entry) override {
                if (entry.log_severity() < turbo::stderr_threshold() &&
                    turbo::log_internal::IsInitialized()) {
                    return;
                }

                TURBO_CONST_INIT static turbo::once_flag warn_if_not_initialized;
                turbo::call_once(warn_if_not_initialized, []() {
                    if (turbo::log_internal::IsInitialized()) return;
                    const char w[] =
                            "WARNING: All log messages before turbo::initialize_log() is called"
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
            const char* const tag = GetAndroidNativeTag();
            __android_log_write(level, tag,
                                entry.text_message_with_prefix_and_newline_c_str());
            if (entry.log_severity() == turbo::LogSeverity::kFatal)
              __android_log_write(ANDROID_LOG_FATAL, tag, "terminating.\n");
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
            if (entry.log_severity() < turbo::stderr_threshold() &&
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
                static turbo::NoDestructor<StderrLogSink> stderr_log_sink;
                add_log_sink(stderr_log_sink.get());
#endif
#ifdef __ANDROID__
                static turbo::NoDestructor<AndroidLogSink> android_log_sink;
                add_log_sink(android_log_sink.get());
#endif
#if defined(_WIN32)
                static turbo::NoDestructor<WindowsDebuggerLogSink> debugger_log_sink;
                add_log_sink(debugger_log_sink.get());
#endif  // !defined(_WIN32)
            }

            void log_to_sinks(const turbo::LogEntry &entry,
                            turbo::span<turbo::LogSink *> extra_sinks, bool extra_sinks_only)
            TURBO_LOCKS_EXCLUDED(guard_) {
                SendToSinks(entry, extra_sinks);

                if (!extra_sinks_only) {
                    if (thread_is_logging_to_log_sink()) {
                        turbo::log_internal::WriteToStderr(
                                entry.text_message_with_prefix_and_newline(), entry.log_severity());
                    } else {
                        turbo::ReaderMutexLock global_sinks_lock(&guard_);
                        thread_is_logging_status() = true;
                        // Ensure the "thread is logging" status is reverted upon leaving the
                        // scope even in case of exceptions.
                        auto status_cleanup =
                                turbo::MakeCleanup([] { thread_is_logging_status() = false; });
                        SendToSinks(entry, turbo::MakeSpan(sinks_));
                    }
                }
            }

            void add_log_sink(turbo::LogSink *sink) TURBO_LOCKS_EXCLUDED(guard_) {
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

            void remove_log_sink(turbo::LogSink *sink) TURBO_LOCKS_EXCLUDED(guard_) {
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

            void flush_log_sinks() TURBO_LOCKS_EXCLUDED(guard_) {
                if (thread_is_logging_to_log_sink()) {
                    // The thread_local condition demonstrates that we're already holding the
                    // lock in order to iterate over `sinks_` for dispatch.  The thread-safety
                    // annotations don't know this, so we use `TURBO_NO_THREAD_SAFETY_ANALYSIS`
                    guard_.AssertReaderHeld();
                    FlushLogSinksLocked();
                } else {
                    turbo::ReaderMutexLock global_sinks_lock(&guard_);
                    // In case if LogSink::Flush overload decides to log
                    thread_is_logging_status() = true;
                    // Ensure the "thread is logging" status is reverted upon leaving the
                    // scope even in case of exceptions.
                    auto status_cleanup =
                            turbo::MakeCleanup([] { thread_is_logging_status() = false; });
                    FlushLogSinksLocked();
                }
            }

        private:
            void FlushLogSinksLocked() TURBO_SHARED_LOCKS_REQUIRED(guard_) {
                for (turbo::LogSink *sink: sinks_) {
                    sink->Flush();
                }
            }

            // Helper routine for log_to_sinks.
            static void SendToSinks(const turbo::LogEntry &entry,
                                    turbo::span<turbo::LogSink *> sinks) {
                for (turbo::LogSink *sink: sinks) {
                    sink->Send(entry);
                }
            }

            using LogSinksSet = std::vector<turbo::LogSink *>;
            turbo::Mutex guard_;
            LogSinksSet sinks_ TURBO_GUARDED_BY(guard_);
        };

        // Returns reference to the global LogSinks set.
        GlobalLogSinkSet &GlobalSinks() {
            static turbo::NoDestructor<GlobalLogSinkSet> global_sinks;
            return *global_sinks;
        }

    }  // namespace

    bool thread_is_logging_to_log_sink() { return thread_is_logging_status(); }

    void log_to_sinks(const turbo::LogEntry &entry,
                    turbo::span<turbo::LogSink *> extra_sinks, bool extra_sinks_only) {
        log_internal::GlobalSinks().log_to_sinks(entry, extra_sinks, extra_sinks_only);
    }

    void add_log_sink(turbo::LogSink *sink) {
        log_internal::GlobalSinks().add_log_sink(sink);
    }

    void remove_log_sink(turbo::LogSink *sink) {
        log_internal::GlobalSinks().remove_log_sink(sink);
    }

    void flush_log_sinks() { log_internal::GlobalSinks().flush_log_sinks(); }

}  // namespace turbo::log_internal
