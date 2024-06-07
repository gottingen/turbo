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
// File: log/structured.h
// -----------------------------------------------------------------------------
//
// This header declares APIs supporting structured logging, allowing log
// statements to be more easily parsed, especially by automated processes.
//
// When structured logging is in use, data streamed into a `LOG` statement are
// encoded as `Value` fields in a `logging.proto.Event` protocol buffer message.
// The individual data are exposed programmatically to `LogSink`s and to the
// user via some log reading tools which are able to query the structured data
// more usefully than would be possible if each message was a single opaque
// string.  These helpers allow user code to add additional structure to the
// data they stream.

#ifndef TURBO_LOG_STRUCTURED_H_
#define TURBO_LOG_STRUCTURED_H_

#include <ostream>

#include <turbo/base/config.h>
#include <turbo/log/internal/structured.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// LogAsLiteral()
//
// Annotates its argument as a string literal so that structured logging
// captures it as a `literal` field instead of a `str` field (the default).
// This does not affect the text representation, only the structure.
//
// Streaming `LogAsLiteral(s)` into a `std::ostream` behaves just like streaming
// `s` directly.
//
// Using `LogAsLiteral()` is occasionally appropriate and useful when proxying
// data logged from another system or another language.  For example:
//
//   void Logger::LogString(std::string_view str, turbo::LogSeverity severity,
//                          const char *file, int line) {
//     LOG(LEVEL(severity)).AtLocation(file, line) << str;
//   }
//   void Logger::LogStringLiteral(std::string_view str,
//                                 turbo::LogSeverity severity, const char *file,
//                                 int line) {
//     LOG(LEVEL(severity)).AtLocation(file, line) << turbo::LogAsLiteral(str);
//   }
inline log_internal::AsLiteralImpl LogAsLiteral(std::string_view s) {
  return log_internal::AsLiteralImpl(s);
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_STRUCTURED_H_
