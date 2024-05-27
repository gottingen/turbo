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
// File: log/log_entry.h
// -----------------------------------------------------------------------------
//
// This header declares `class turbo::LogEntry`, which represents a log record as
// passed to `LogSink::Send`. Data returned by pointer or by reference or by
// `turbo::string_view` must be copied if they are needed after the lifetime of
// the `turbo::LogEntry`.

#ifndef TURBO_LOG_LOG_ENTRY_H_
#define TURBO_LOG_LOG_ENTRY_H_

#include <cstddef>
#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/config.h>
#include <turbo/strings/string_view.h>
#include <turbo/time/time.h>
#include <turbo/types/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace log_internal {
// Test only friend.
class LogEntryTestPeer;
class LogMessage;
}  // namespace log_internal

// LogEntry
//
// Represents a single entry in a log, i.e., one `LOG` statement or failed
// `CHECK`.
//
// `LogEntry` is thread-compatible.
class LogEntry final {
 public:
  using tid_t = log_internal::Tid;

  // For non-verbose log entries, `verbosity()` returns `kNoVerbosityLevel`.
  static constexpr int kNoVerbosityLevel = -1;
  static constexpr int kNoVerboseLevel = -1;  // TO BE removed

  // Pass `LogEntry` by reference, and do not store it as its state does not
  // outlive the call to `LogSink::Send()`.
  LogEntry(const LogEntry&) = delete;
  LogEntry& operator=(const LogEntry&) = delete;

  // Source file and line where the log message occurred.  Taken from `__FILE__`
  // and `__LINE__` unless overridden by `LOG(...).AtLocation(...)`.
  //
  // Take special care not to use the values returned by `source_filename()` and
  // `source_basename()` after the lifetime of the entry.  This is always
  // incorrect, but it will often work in practice because they usually point
  // into a statically allocated character array obtained from `__FILE__`.
  // Statements like `LOG(INFO).AtLocation(std::string(...), ...)` will expose
  // the bug.  If you need the data later, you must copy them.
  turbo::string_view source_filename() const TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return full_filename_;
  }
  turbo::string_view source_basename() const TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return base_filename_;
  }
  int source_line() const { return line_; }

  // LogEntry::prefix()
  //
  // True unless the metadata prefix was suppressed once by
  // `LOG(...).NoPrefix()` or globally by `turbo::EnableLogPrefix(false)`.
  // Implies `text_message_with_prefix() == text_message()`.
  bool prefix() const { return prefix_; }

  // LogEntry::log_severity()
  //
  // Returns this entry's severity.  For `LOG`, taken from the first argument;
  // for `CHECK`, always `turbo::LogSeverity::kFatal`.
  turbo::LogSeverity log_severity() const { return severity_; }

  // LogEntry::verbosity()
  //
  // Returns this entry's verbosity, or `kNoVerbosityLevel` for a non-verbose
  // entry. Taken from the argument to `VLOG` or from
  // `LOG(...).WithVerbosity(...)`.
  int verbosity() const { return verbose_level_; }

  // LogEntry::timestamp()
  //
  // Returns the time at which this entry was written.  Captured during
  // evaluation of `LOG`, but can be overridden by
  // `LOG(...).WithTimestamp(...)`.
  //
  // Take care not to rely on timestamps increasing monotonically, or even to
  // rely on timestamps having any particular relationship with reality (since
  // they can be overridden).
  turbo::Time timestamp() const { return timestamp_; }

  // LogEntry::tid()
  //
  // Returns the ID of the thread that wrote this entry.  Captured during
  // evaluation of `LOG`, but can be overridden by `LOG(...).WithThreadID(...)`.
  //
  // Take care not to *rely* on reported thread IDs as they can be overridden as
  // specified above.
  tid_t tid() const { return tid_; }

  // Text-formatted version of the log message.  An underlying buffer holds
  // these contiguous data:
  //
  // * A prefix formed by formatting metadata (timestamp, filename, line number,
  //   etc.)
  //   The prefix may be empty - see `LogEntry::prefix()` - and may rarely be
  //   truncated if the metadata are very long.
  // * The streamed data
  //   The data may be empty if nothing was streamed, or may be truncated to fit
  //   the buffer.
  // * A newline
  // * A nul terminator
  //
  // The newline and nul terminator will be present even if the prefix and/or
  // data are truncated.
  //
  // These methods give access to the most commonly useful substrings of the
  // buffer's contents.  Other combinations can be obtained with substring
  // arithmetic.
  //
  // The buffer does not outlive the entry; if you need the data later, you must
  // copy them.
  turbo::string_view text_message_with_prefix_and_newline() const
      TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return turbo::string_view(
        text_message_with_prefix_and_newline_and_nul_.data(),
        text_message_with_prefix_and_newline_and_nul_.size() - 1);
  }
  turbo::string_view text_message_with_prefix() const
      TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return turbo::string_view(
        text_message_with_prefix_and_newline_and_nul_.data(),
        text_message_with_prefix_and_newline_and_nul_.size() - 2);
  }
  turbo::string_view text_message_with_newline() const
      TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return turbo::string_view(
        text_message_with_prefix_and_newline_and_nul_.data() + prefix_len_,
        text_message_with_prefix_and_newline_and_nul_.size() - prefix_len_ - 1);
  }
  turbo::string_view text_message() const TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return turbo::string_view(
        text_message_with_prefix_and_newline_and_nul_.data() + prefix_len_,
        text_message_with_prefix_and_newline_and_nul_.size() - prefix_len_ - 2);
  }
  const char* text_message_with_prefix_and_newline_c_str() const
      TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return text_message_with_prefix_and_newline_and_nul_.data();
  }

  // Returns a serialized protobuf holding the operands streamed into this
  // log message.  The message definition is not yet published.
  //
  // The buffer does not outlive the entry; if you need the data later, you must
  // copy them.
  turbo::string_view encoded_message() const TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return encoding_;
  }

  // LogEntry::stacktrace()
  //
  // Optional stacktrace, e.g. for `FATAL` logs and failed `CHECK`s.
  //
  // Fatal entries are dispatched to each sink twice: first with all data and
  // metadata but no stacktrace, and then with the stacktrace.  This is done
  // because stacktrace collection is sometimes slow and fallible, and it's
  // critical to log enough information to diagnose the failure even if the
  // stacktrace collection hangs.
  //
  // The buffer does not outlive the entry; if you need the data later, you must
  // copy them.
  turbo::string_view stacktrace() const TURBO_ATTRIBUTE_LIFETIME_BOUND {
    return stacktrace_;
  }

 private:
  LogEntry() = default;

  turbo::string_view full_filename_;
  turbo::string_view base_filename_;
  int line_;
  bool prefix_;
  turbo::LogSeverity severity_;
  int verbose_level_;  // >=0 for `VLOG`, etc.; otherwise `kNoVerbosityLevel`.
  turbo::Time timestamp_;
  tid_t tid_;
  turbo::Span<const char> text_message_with_prefix_and_newline_and_nul_;
  size_t prefix_len_;
  turbo::string_view encoding_;
  std::string stacktrace_;

  friend class log_internal::LogEntryTestPeer;
  friend class log_internal::LogMessage;
};

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_LOG_ENTRY_H_
