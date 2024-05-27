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
// File: log/vlog_is_on.h
// -----------------------------------------------------------------------------
//
// This header defines the `VLOG_IS_ON()` macro that controls the
// variable-verbosity conditional logging.
//
// It's used by `VLOG` in log.h, or it can also be used directly like this:
//
//   if (VLOG_IS_ON(2)) {
//     foo_server.RecomputeStatisticsExpensive();
//     LOG(INFO) << foo_server.LastStatisticsAsString();
//   }
//
// Each source file has an effective verbosity level that's a non-negative
// integer computed from the `--vmodule` and `--v` flags.
// `VLOG_IS_ON(n)` is true, and `VLOG(n)` logs, if that effective verbosity
// level is greater than or equal to `n`.
//
// `--vmodule` takes a comma-delimited list of key=value pairs.  Each key is a
// pattern matched against filenames, and the values give the effective severity
// level applied to matching files.  '?' and '*' characters in patterns are
// interpreted as single-character and zero-or-more-character wildcards.
// Patterns including a slash character are matched against full pathnames,
// while those without are matched against basenames only.  One suffix (i.e. the
// last . and everything after it) is stripped from each filename prior to
// matching, as is the special suffix "-inl".
//
// Files are matched against globs in `--vmodule` in order, and the first match
// determines the verbosity level.
//
// Files which do not match any pattern in `--vmodule` use the value of `--v` as
// their effective verbosity level.  The default is 0.
//
// SetVLogLevel helper function is provided to do limited dynamic control over
// V-logging by appending to `--vmodule`. Because these go at the beginning of
// the list, they take priority over any globs previously added.
//
// Resetting --vmodule will override all previous modifications to `--vmodule`,
// including via SetVLogLevel.

#ifndef TURBO_LOG_VLOG_IS_ON_H_
#define TURBO_LOG_VLOG_IS_ON_H_

#include <turbo/log/turbo_vlog_is_on.h>  // IWYU pragma: export

// IWYU pragma: private, include "turbo/log/log.h"

// Each VLOG_IS_ON call site gets its own VLogSite that registers with the
// global linked list of sites to asynchronously update its verbosity level on
// changes to --v or --vmodule. The verbosity can also be set by manually
// calling SetVLogLevel.
//
// VLOG_IS_ON is not async signal safe, but it is guaranteed not to allocate
// new memory.
#define VLOG_IS_ON(verbose_level) TURBO_VLOG_IS_ON(verbose_level)

#endif  // TURBO_LOG_VLOG_IS_ON_H_
