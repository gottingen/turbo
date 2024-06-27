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
// File: log/turbo_vlog_is_on.h
// -----------------------------------------------------------------------------
//
// This header defines the `TURBO_VLOG_IS_ON()` macro that controls the
// variable-verbosity conditional logging.
//
// It's used by `VLOG` in log.h, or it can also be used directly like this:
//
//   if (TURBO_VLOG_IS_ON(2)) {
//     foo_server.RecomputeStatisticsExpensive();
//     LOG(INFO) << foo_server.LastStatisticsAsString();
//   }
//
// Each source file has an effective verbosity level that's a non-negative
// integer computed from the `--vlog_module` and `--verbosity` flags.
// `TURBO_VLOG_IS_ON(n)` is true, and `VLOG(n)` logs, if that effective verbosity
// level is greater than or equal to `n`.
//
// `--vlog_module` takes a comma-delimited list of key=value pairs.  Each key is a
// pattern matched against filenames, and the values give the effective severity
// level applied to matching files.  '?' and '*' characters in patterns are
// interpreted as single-character and zero-or-more-character wildcards.
// Patterns including a slash character are matched against full pathnames,
// while those without are matched against basenames only.  One suffix (i.e. the
// last . and everything after it) is stripped from each filename prior to
// matching, as is the special suffix "-inl".
//
// Files are matched against globs in `--vlog_module` in order, and the first match
// determines the verbosity level.
//
// Files which do not match any pattern in `--vlog_module` use the value of `--verbosity` as
// their effective verbosity level.  The default is 0.
//
// set_vlog_level helper function is provided to do limited dynamic control over
// V-logging by appending to `--vlog_module`. Because these go at the beginning of
// the list, they take priority over any globs previously added.
//
// Resetting --vlog_module will override all previous modifications to `--vlog_module`,
// including via set_vlog_level.

#ifndef TURBO_LOG_TURBO_VLOG_IS_ON_H_
#define TURBO_LOG_TURBO_VLOG_IS_ON_H_

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/log/internal/vlog_config.h>  // IWYU pragma: export
#include <turbo/strings/string_view.h>

// IWYU pragma: private, include "turbo/log/log.h"

// This is expanded at the callsite to allow the compiler to optimize
// always-false cases out of the build.
// An TURBO_MAX_VLOG_VERBOSITY of 2 means that VLOG(3) and above should never
// log.
#ifdef TURBO_MAX_VLOG_VERBOSITY
#define TURBO_LOG_INTERNAL_MAX_LOG_VERBOSITY_CHECK(x) \
  ((x) <= TURBO_MAX_VLOG_VERBOSITY)&&
#else
#define TURBO_LOG_INTERNAL_MAX_LOG_VERBOSITY_CHECK(x)
#endif

// Each TURBO_VLOG_IS_ON call site gets its own VLogSite that registers with the
// global linked list of sites to asynchronously update its verbosity level on
// changes to --verbosity or --vlog_module. The verbosity can also be set by manually
// calling set_vlog_level.
//
// TURBO_VLOG_IS_ON is not async signal safe, but it is guaranteed not to
// allocate new memory.
#define TURBO_VLOG_IS_ON(verbose_level)                                     \
  (TURBO_LOG_INTERNAL_MAX_LOG_VERBOSITY_CHECK(verbose_level)[]()            \
       ->::turbo::log_internal::VLogSite *                                  \
   {                                                                       \
     TURBO_CONST_INIT static ::turbo::log_internal::VLogSite site(__FILE__); \
     return &site;                                                         \
   }()                                                                     \
       ->IsEnabled(verbose_level))

#endif  // TURBO_LOG_TURBO_VLOG_IS_ON_H_
