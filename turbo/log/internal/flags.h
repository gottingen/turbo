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
// File: log/log_flags.h
// -----------------------------------------------------------------------------
//
// This header declares set of flags which can be used to configure Turbo
// Logging library behaviour at runtime.

#ifndef TURBO_LOG_INTERNAL_FLAGS_H_
#define TURBO_LOG_INTERNAL_FLAGS_H_

#include <string>

#include <turbo/flags/declare.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// These flags should not be used in C++ code to access logging library
// configuration knobs. Use interfaces defined in turbo/log/globals.h
// instead. It is still ok to use these flags on a command line.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Log messages at this severity or above are sent to stderr in *addition* to
// `LogSink`s.  Defaults to `ERROR`.  See log_severity.h for numeric values of
// severity levels.
TURBO_DECLARE_FLAG(int, stderrthreshold);

// Log messages at this severity or above are logged; others are discarded.
// Defaults to `INFO`, i.e. log all severities.  See log_severity.h for numeric
// values of severity levels.
TURBO_DECLARE_FLAG(int, minloglevel);

// If specified in the form file:linenum, any messages logged from a matching
// location will also include a backtrace.
TURBO_DECLARE_FLAG(std::string, log_backtrace_at);

// If true, the log prefix (severity, date, time, PID, etc.) is prepended to
// each message logged. Defaults to true.
TURBO_DECLARE_FLAG(bool, log_prefix);

// Global log verbosity level. Default is 0.
TURBO_DECLARE_FLAG(int, v);

// Per-module log verbosity level. By default is empty and is unused.
TURBO_DECLARE_FLAG(std::string, vmodule);

#endif  // TURBO_LOG_INTERNAL_FLAGS_H_
