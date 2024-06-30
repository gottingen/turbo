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
// File: log/flags.h
// -----------------------------------------------------------------------------
//

#pragma once
#include <string>
#include <turbo/flags/declare.h>
// The Turbo Logging library supports the following command line flags to
// configure logging behavior at runtime:
//
// --stderr_threshold=<value>
// Log messages at or above this threshold level are copied to stderr.
//
// --min_log_level=<value>
// Messages logged at a lower level than this are discarded and don't actually
// get logged anywhere.
//
// --backtrace_log_at=<file:linenum>
// Emit a backtrace (stack trace) when logging at file:linenum.
//
// To use these commandline flags, the //turbo/log:flags library must be
// explicitly linked, and turbo::parse_command_line() must be called before the
// call to turbo::initialize_log().
//
// To configure the Log library programmatically, use the interfaces defined in
// turbo/log/globals.h.


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// These flags should not be used in C++ code to access logging library
// configuration knobs. Use interfaces defined in turbo/log/globals.h
// instead. It is still ok to use these flags on a command line.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Log messages at this severity or above are sent to stderr in *addition* to
// `LogSink`s.  Defaults to `ERROR`.  See log_severity.h for numeric values of
// severity levels.
TURBO_DECLARE_FLAG(int, stderr_threshold);

// Log messages at this severity or above are logged; others are discarded.
// Defaults to `INFO`, i.e. log all severities.  See log_severity.h for numeric
// values of severity levels.
TURBO_DECLARE_FLAG(int, min_log_level);

// If specified in the form file:linenum, any messages logged from a matching
// location will also include a backtrace.
TURBO_DECLARE_FLAG(std::string, backtrace_log_at);

// If true, the log prefix (severity, date, time, PID, etc.) is prepended to
// each message logged. Defaults to true.
TURBO_DECLARE_FLAG(bool, log_with_prefix);

// Global log verbosity level. Default is 0.
TURBO_DECLARE_FLAG(int, verbosity);

// Per-module log verbosity level. By default is empty and is unused.
TURBO_DECLARE_FLAG(std::string, vlog_module);

// Log file name. If specified, log messages are written to this file.
TURBO_DECLARE_FLAG(std::string, log_base_filename);

// Log file rotation options. used by daily log rotation.
TURBO_DECLARE_FLAG(int, log_rotation_hour);

// Log file rotation options. used by daily log rotation,
// and hourly log rotation.
TURBO_DECLARE_FLAG(int, log_rotation_minute);

// Log file flush options. check if file has been
// removed by other process, and reopen the file.
TURBO_DECLARE_FLAG(int, log_check_interval_s);

// Log file truncate options. truncate the log file
TURBO_DECLARE_FLAG(bool, log_truncate);

// Log file max files options. max files to keep
// used by daily log rotation, and hourly log rotation.
// and rotating log file.
TURBO_DECLARE_FLAG(int, log_max_files);

// max file size in MB for rotating log file.
// used by rotating log file.
TURBO_DECLARE_FLAG(int, log_max_file_size);

// Log type, 0: console log
// 1: daily log file
// 2: hourly log file
// 3: rotating log file

// usually for debug purpose, use 0 dump log to console,
// for production, use 1, 2, 3 as you need.

TURBO_DECLARE_FLAG(int, log_type);