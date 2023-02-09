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
//
// -----------------------------------------------------------------------------
// File: log/log_sink_registry.h
// -----------------------------------------------------------------------------
//
// This header declares APIs to operate on global set of registered log sinks.

#ifndef TURBO_LOG_LOG_SINK_REGISTRY_H_
#define TURBO_LOG_LOG_SINK_REGISTRY_H_

#include "turbo/log/internal/log_sink_set.h"
#include "turbo/log/log_sink.h"
#include "turbo/platform/port.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

// AddLogSink(), RemoveLogSink()
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
inline void AddLogSink(turbo::LogSink* sink) { log_internal::AddLogSink(sink); }
inline void RemoveLogSink(turbo::LogSink* sink) {
  log_internal::RemoveLogSink(sink);
}

// FlushLogSinks()
//
// Calls `turbo::LogSink::Flush` on all registered sinks.
//
// Do not call this inside `turbo::LogSink::Send`.
inline void FlushLogSinks() { log_internal::FlushLogSinks(); }

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_LOG_SINK_REGISTRY_H_
