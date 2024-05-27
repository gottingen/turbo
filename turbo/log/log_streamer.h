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
// File: log/log_streamer.h
// -----------------------------------------------------------------------------
//
// This header declares the class `LogStreamer` and convenience functions to
// construct LogStreamer objects with different associated log severity levels.

#pragma once

#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/turbo_log.h>
#include <turbo/strings/internal/ostringstream.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/optional.h>
#include <turbo/utility/utility.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// LogStreamer
//
// Although you can stream into `LOG(INFO)`, you can't pass it into a function
// that takes a `std::ostream` parameter. `LogStreamer::stream()` provides a
// `std::ostream` that buffers everything that's streamed in.  The buffer's
// contents are logged as if by `LOG` when the `LogStreamer` is destroyed.
// If nothing is streamed in, an empty message is logged.  If the specified
// severity is `turbo::LogSeverity::kFatal`, the program will be terminated when
// the `LogStreamer` is destroyed regardless of whether any data were streamed
// in.
//
// Factory functions corresponding to the `turbo::LogSeverity` enumerators
// are provided for convenience; if the desired severity is variable, invoke the
// constructor directly.
//
// LogStreamer is movable, but not copyable.
//
// Examples:
//
//   ShaveYakAndWriteToStream(
//       yak, turbo::LogInfoStreamer(__FILE__, __LINE__).stream());
//
//   {
//     // This logs a single line containing data streamed by all three function
//     // calls.
//     turbo::LogStreamer streamer(turbo::LogSeverity::kInfo, __FILE__, __LINE__);
//     ShaveYakAndWriteToStream(yak1, streamer.stream());
//     streamer.stream() << " ";
//     ShaveYakAndWriteToStream(yak2, streamer.stream());
//     streamer.stream() << " ";
//     ShaveYakAndWriteToStreamPointer(yak3, &streamer.stream());
//   }
class LogStreamer final {
 public:
  // LogStreamer::LogStreamer()
  //
  // Creates a LogStreamer with a given `severity` that will log a message
  // attributed to the given `file` and `line`.
  explicit LogStreamer(turbo::LogSeverity severity, turbo::string_view file,
                       int line)
      : severity_(severity),
        line_(line),
        file_(file),
        stream_(turbo::in_place, &buf_) {
    // To match `LOG`'s defaults:
    stream_->setf(std::ios_base::showbase | std::ios_base::boolalpha);
  }

  // A moved-from `turbo::LogStreamer` does not `LOG` when destroyed,
  // and a program that streams into one has undefined behavior.
  LogStreamer(LogStreamer&& that) noexcept
      : severity_(that.severity_),
        line_(that.line_),
        file_(std::move(that.file_)),
        buf_(std::move(that.buf_)),
        stream_(std::move(that.stream_)) {
    if (stream_.has_value()) stream_->str(&buf_);
    that.stream_.reset();
  }
  LogStreamer& operator=(LogStreamer&& that) {
    TURBO_LOG_IF(LEVEL(severity_), stream_).AtLocation(file_, line_) << buf_;
    severity_ = that.severity_;
    file_ = std::move(that.file_);
    line_ = that.line_;
    buf_ = std::move(that.buf_);
    stream_ = std::move(that.stream_);
    if (stream_.has_value()) stream_->str(&buf_);
    that.stream_.reset();
    return *this;
  }

  // LogStreamer::~LogStreamer()
  //
  // Logs this LogStreamer's buffered content as if by LOG.
  ~LogStreamer() {
    TURBO_LOG_IF(LEVEL(severity_), stream_.has_value()).AtLocation(file_, line_)
        << buf_;
  }

  // LogStreamer::stream()
  //
  // Returns the `std::ostream` to use to write into this LogStreamer' internal
  // buffer.
  std::ostream& stream() { return *stream_; }

 private:
  turbo::LogSeverity severity_;
  int line_;
  std::string file_;
  std::string buf_;
  // A disengaged `stream_` indicates a moved-from `LogStreamer` that should not
  // `LOG` upon destruction.
  turbo::optional<turbo::strings_internal::OStringStream> stream_;
};

// LogInfoStreamer()
//
// Returns a LogStreamer that writes at level LogSeverity::kInfo.
inline LogStreamer LogInfoStreamer(turbo::string_view file, int line) {
  return turbo::LogStreamer(turbo::LogSeverity::kInfo, file, line);
}

// LogWarningStreamer()
//
// Returns a LogStreamer that writes at level LogSeverity::kWarning.
inline LogStreamer LogWarningStreamer(turbo::string_view file, int line) {
  return turbo::LogStreamer(turbo::LogSeverity::kWarning, file, line);
}

// LogErrorStreamer()
//
// Returns a LogStreamer that writes at level LogSeverity::kError.
inline LogStreamer LogErrorStreamer(turbo::string_view file, int line) {
  return turbo::LogStreamer(turbo::LogSeverity::kError, file, line);
}

// LogFatalStreamer()
//
// Returns a LogStreamer that writes at level LogSeverity::kFatal.
//
// The program will be terminated when this `LogStreamer` is destroyed,
// regardless of whether any data were streamed in.
inline LogStreamer LogFatalStreamer(turbo::string_view file, int line) {
  return turbo::LogStreamer(turbo::LogSeverity::kFatal, file, line);
}

// LogDebugFatalStreamer()
//
// Returns a LogStreamer that writes at level LogSeverity::kLogDebugFatal.
//
// In debug mode, the program will be terminated when this `LogStreamer` is
// destroyed, regardless of whether any data were streamed in.
inline LogStreamer LogDebugFatalStreamer(turbo::string_view file, int line) {
  return turbo::LogStreamer(turbo::kLogDebugFatal, file, line);
}

TURBO_NAMESPACE_END
}  // namespace turbo
