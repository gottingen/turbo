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
// File: log/turbo_log.h
// -----------------------------------------------------------------------------
//
// This header declares a family of `TURBO_LOG` macros as alternative spellings
// for macros in `log.h`.
//
// Basic invocation looks like this:
//
//   TURBO_LOG(INFO) << "Found " << num_cookies << " cookies";
//
// Most `TURBO_LOG` macros take a severity level argument. The severity levels
// are `INFO`, `WARNING`, `ERROR`, and `FATAL`.
//
// For full documentation, see comments in `log.h`, which includes full
// reference documentation on use of the equivalent `LOG` macro and has an
// identical set of macros without the TURBO_* prefix.

#ifndef TURBO_LOG_TURBO_LOG_H_
#define TURBO_LOG_TURBO_LOG_H_

#include <turbo/log/internal/log_impl.h>

#define TURBO_LOG(severity) TURBO_LOG_INTERNAL_LOG_IMPL(_##severity)
#define TURBO_PLOG(severity) TURBO_LOG_INTERNAL_PLOG_IMPL(_##severity)
#define TURBO_DLOG(severity) TURBO_LOG_INTERNAL_DLOG_IMPL(_##severity)

#define TURBO_VLOG(verbose_level) TURBO_LOG_INTERNAL_VLOG_IMPL(verbose_level)
#define TURBO_DVLOG(verbose_level) TURBO_LOG_INTERNAL_DVLOG_IMPL(verbose_level)

#define TURBO_LOG_IF(severity, condition) \
  TURBO_LOG_INTERNAL_LOG_IF_IMPL(_##severity, condition)
#define TURBO_PLOG_IF(severity, condition) \
  TURBO_LOG_INTERNAL_PLOG_IF_IMPL(_##severity, condition)
#define TURBO_DLOG_IF(severity, condition) \
  TURBO_LOG_INTERNAL_DLOG_IF_IMPL(_##severity, condition)

#define TURBO_LOG_EVERY_N(severity, n) \
  TURBO_LOG_INTERNAL_LOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_LOG_FIRST_N(severity, n) \
  TURBO_LOG_INTERNAL_LOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_LOG_EVERY_POW_2(severity) \
  TURBO_LOG_INTERNAL_LOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_LOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_LOG_INTERNAL_LOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_PLOG_EVERY_N(severity, n) \
  TURBO_LOG_INTERNAL_PLOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_PLOG_FIRST_N(severity, n) \
  TURBO_LOG_INTERNAL_PLOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_PLOG_EVERY_POW_2(severity) \
  TURBO_LOG_INTERNAL_PLOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_PLOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_LOG_INTERNAL_PLOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_DLOG_EVERY_N(severity, n) \
  TURBO_LOG_INTERNAL_DLOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_DLOG_FIRST_N(severity, n) \
  TURBO_LOG_INTERNAL_DLOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_DLOG_EVERY_POW_2(severity) \
  TURBO_LOG_INTERNAL_DLOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_DLOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_LOG_INTERNAL_DLOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_VLOG_EVERY_N(verbose_level, n) \
  TURBO_LOG_INTERNAL_VLOG_EVERY_N_IMPL(verbose_level, n)
#define TURBO_VLOG_FIRST_N(verbose_level, n) \
  TURBO_LOG_INTERNAL_VLOG_FIRST_N_IMPL(verbose_level, n)
#define TURBO_VLOG_EVERY_POW_2(verbose_level, n) \
  TURBO_LOG_INTERNAL_VLOG_EVERY_POW_2_IMPL(verbose_level, n)
#define TURBO_VLOG_EVERY_N_SEC(verbose_level, n) \
  TURBO_LOG_INTERNAL_VLOG_EVERY_N_SEC_IMPL(verbose_level, n)

#define TURBO_LOG_IF_EVERY_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_LOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_LOG_IF_FIRST_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_LOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_LOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_LOG_INTERNAL_LOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_LOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_LOG_INTERNAL_LOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#define TURBO_PLOG_IF_EVERY_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_PLOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_PLOG_IF_FIRST_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_PLOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_PLOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_LOG_INTERNAL_PLOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_PLOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_LOG_INTERNAL_PLOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#define TURBO_DLOG_IF_EVERY_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_DLOG_IF_FIRST_N(severity, condition, n) \
  TURBO_LOG_INTERNAL_DLOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_DLOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_LOG_INTERNAL_DLOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_DLOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#endif  // TURBO_LOG_TURBO_LOG_H_
