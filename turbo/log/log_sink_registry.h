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
// File: log/log_sink_registry.h
// -----------------------------------------------------------------------------
//
// This header declares APIs to operate on global set of registered log sinks.

#pragma once

#include <turbo/base/config.h>
#include <turbo/log/internal/log_sink_set.h>
#include <turbo/log/log_sink.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    // add_log_sink(), remove_log_sink()
    //
    // Adds or removes a `turbo::LogSink` as a consumer of logging data.
    //
    // These functions are thread-safe.
    //
    // It is an error to attempt to add a sink that's already registered or to
    // attempt to remove one that isn't.
    //
    // To avoid unbounded recursion, dispatch to registered `turbo::LogSink`s is
    // disabled per-thread while running the `Send()` method of registered
    // `turbo::LogSink`s.  Affected messages are dispatched to a special internal
    // sink instead which writes them to `stderr`.
    //
    // Do not call these inside `turbo::LogSink::Send`.
    inline void add_log_sink(turbo::LogSink *sink) { log_internal::add_log_sink(sink); }

    inline void remove_log_sink(turbo::LogSink *sink) {
        log_internal::remove_log_sink(sink);
    }

    // flush_log_sinks()
    //
    // Calls `turbo::LogSink::Flush` on all registered sinks.
    //
    // Do not call this inside `turbo::LogSink::Send`.
    inline void flush_log_sinks() { log_internal::flush_log_sinks(); }

    TURBO_NAMESPACE_END
}  // namespace turbo
