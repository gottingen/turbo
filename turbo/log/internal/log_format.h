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
// File: log/internal/log_format.h
// -----------------------------------------------------------------------------
//
// This file declares routines implementing formatting of log message and log
// prefix.

#ifndef TURBO_LOG_INTERNAL_LOG_FORMAT_H_
#define TURBO_LOG_INTERNAL_LOG_FORMAT_H_

#include <stddef.h>

#include <string>

#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/config.h>
#include <turbo/strings/string_view.h>
#include <turbo/time/civil_time.h>
#include <turbo/time/time.h>
#include <turbo/types/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

enum class PrefixFormat {
  kNotRaw,
  kRaw,
};

// Formats log message based on provided data.
std::string FormatLogMessage(turbo::LogSeverity severity,
                             turbo::CivilSecond civil_second,
                             turbo::Duration subsecond, log_internal::Tid tid,
                             turbo::string_view basename, int line,
                             PrefixFormat format, turbo::string_view message);

// Formats various entry metadata into a text string meant for use as a
// prefix on a log message string.  Writes into `buf`, advances `buf` to point
// at the remainder of the buffer (i.e. past any written bytes), and returns the
// number of bytes written.
//
// In addition to calling `buf->remove_prefix()` (or the equivalent), this
// function may also do `buf->remove_suffix(buf->size())` in cases where no more
// bytes (i.e. no message data) should be written into the buffer.  For example,
// if the prefix ought to be:
//   I0926 09:00:00.000000 1234567 foo.cc:123]
// `buf` is too small, the function might fill the whole buffer:
//   I0926 09:00:00.000000 1234
// (note the apparrently incorrect thread ID), or it might write less:
//   I0926 09:00:00.000000
// In this case, it might also empty `buf` prior to returning to prevent
// message data from being written into the space where a reader would expect to
// see a thread ID.
size_t FormatLogPrefix(turbo::LogSeverity severity, turbo::Time timestamp,
                       log_internal::Tid tid, turbo::string_view basename,
                       int line, PrefixFormat format, turbo::Span<char>& buf);

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_LOG_FORMAT_H_
