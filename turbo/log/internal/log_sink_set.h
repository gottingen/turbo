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
// File: log/internal/log_sink_set.h
// -----------------------------------------------------------------------------

#ifndef TURBO_LOG_INTERNAL_LOG_SINK_SET_H_
#define TURBO_LOG_INTERNAL_LOG_SINK_SET_H_

#include <turbo/base/config.h>
#include <turbo/log/log_entry.h>
#include <turbo/log/log_sink.h>
#include <turbo/types/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// Returns true if a globally-registered `LogSink`'s `Send()` is currently
// being invoked on this thread.
bool ThreadIsLoggingToLogSink();

// This function may log to two sets of sinks:
//
// * If `extra_sinks_only` is true, it will dispatch only to `extra_sinks`.
//   `LogMessage::ToSinkAlso` and `LogMessage::ToSinkOnly` are used to attach
//    extra sinks to the entry.
// * Otherwise it will also log to the global sinks set. This set is managed
//   by `turbo::AddLogSink` and `turbo::RemoveLogSink`.
void LogToSinks(const turbo::LogEntry& entry,
                turbo::Span<turbo::LogSink*> extra_sinks, bool extra_sinks_only);

// Implementation for operations with log sink set.
void AddLogSink(turbo::LogSink* sink);
void RemoveLogSink(turbo::LogSink* sink);
void FlushLogSinks();

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_LOG_SINK_SET_H_
