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

#ifndef TURBO_LOG_INTERNAL_LOG_IMPL_H_
#define TURBO_LOG_INTERNAL_LOG_IMPL_H_

#include <turbo/log/turbo_vlog_is_on.h>
#include <turbo/log/internal/conditions.h>
#include <turbo/log/internal/log_message.h>
#include <turbo/log/internal/strip.h>

// TURBO_LOG()
#define TURBO_LOG_INTERNAL_LOG_IMPL(severity)             \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_PLOG()
#define TURBO_LOG_INTERNAL_PLOG_IMPL(severity)              \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true)   \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream() \
          .WithPerror()

// TURBO_DLOG()
#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_IMPL(severity)            \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#else
#define TURBO_LOG_INTERNAL_DLOG_IMPL(severity)             \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, false) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#endif

// The `switch` ensures that this expansion is the begnning of a statement (as
// opposed to an expression). The use of both `case 0` and `default` is to
// suppress a compiler warning.
#define TURBO_LOG_INTERNAL_VLOG_IMPL(verbose_level)                         \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level)) \
  case 0:                                                                  \
  default:                                                                 \
    TURBO_LOG_INTERNAL_LOG_IF_IMPL(                                         \
        _INFO, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))       \
        .WithVerbosity(turbo_logging_internal_verbose_level)

#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DVLOG_IMPL(verbose_level)                        \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level)) \
  case 0:                                                                  \
  default:                                                                 \
    TURBO_LOG_INTERNAL_DLOG_IF_IMPL(                                         \
        _INFO, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))       \
        .WithVerbosity(turbo_logging_internal_verbose_level)
#else
#define TURBO_LOG_INTERNAL_DVLOG_IMPL(verbose_level)                           \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level))    \
  case 0:                                                                     \
  default:                                                                    \
    TURBO_LOG_INTERNAL_DLOG_IF_IMPL(                                            \
        _INFO, false && TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level)) \
        .WithVerbosity(turbo_logging_internal_verbose_level)
#endif

#define TURBO_LOG_INTERNAL_LOG_IF_IMPL(severity, condition)    \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#define TURBO_LOG_INTERNAL_PLOG_IF_IMPL(severity, condition)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()    \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_IF_IMPL(severity, condition)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#else
#define TURBO_LOG_INTERNAL_DLOG_IF_IMPL(severity, condition)              \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, false && (condition)) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#endif

// TURBO_LOG_EVERY_N
#define TURBO_LOG_INTERNAL_LOG_EVERY_N_IMPL(severity, n)            \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_FIRST_N
#define TURBO_LOG_INTERNAL_LOG_FIRST_N_IMPL(severity, n)            \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_EVERY_POW_2
#define TURBO_LOG_INTERNAL_LOG_EVERY_POW_2_IMPL(severity)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_EVERY_N_SEC
#define TURBO_LOG_INTERNAL_LOG_EVERY_N_SEC_IMPL(severity, n_seconds)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryNSec, n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_PLOG_EVERY_N_IMPL(severity, n)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_FIRST_N_IMPL(severity, n)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_EVERY_POW_2_IMPL(severity)          \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_EVERY_N_SEC_IMPL(severity, n_seconds)          \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryNSec, n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()                    \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_EVERY_N_IMPL(severity, n) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true)       \
  (EveryN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_FIRST_N_IMPL(severity, n) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true)       \
  (FirstN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_EVERY_POW_2_IMPL(severity) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true)        \
  (EveryPow2) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_EVERY_N_SEC_IMPL(severity, n_seconds) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true)                   \
  (EveryNSec, n_seconds) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#else  // def NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_EVERY_N_IMPL(severity, n) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false)      \
  (EveryN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_FIRST_N_IMPL(severity, n) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false)      \
  (FirstN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_EVERY_POW_2_IMPL(severity) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false)       \
  (EveryPow2) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_EVERY_N_SEC_IMPL(severity, n_seconds) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false)                  \
  (EveryNSec, n_seconds) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#endif  // def NDEBUG

#define TURBO_LOG_INTERNAL_VLOG_EVERY_N_IMPL(verbose_level, n)                \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level))   \
  case 0:                                                                    \
  default:                                                                   \
    TURBO_LOG_INTERNAL_CONDITION_INFO(                                        \
        STATEFUL, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))      \
  (EveryN, n) TURBO_LOGGING_INTERNAL_LOG_INFO.InternalStream().WithVerbosity( \
      turbo_logging_internal_verbose_level)

#define TURBO_LOG_INTERNAL_VLOG_FIRST_N_IMPL(verbose_level, n)                \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level))   \
  case 0:                                                                    \
  default:                                                                   \
    TURBO_LOG_INTERNAL_CONDITION_INFO(                                        \
        STATEFUL, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))      \
  (FirstN, n) TURBO_LOGGING_INTERNAL_LOG_INFO.InternalStream().WithVerbosity( \
      turbo_logging_internal_verbose_level)

#define TURBO_LOG_INTERNAL_VLOG_EVERY_POW_2_IMPL(verbose_level)               \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level))   \
  case 0:                                                                    \
  default:                                                                   \
    TURBO_LOG_INTERNAL_CONDITION_INFO(                                        \
        STATEFUL, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))      \
  (EveryPow2) TURBO_LOGGING_INTERNAL_LOG_INFO.InternalStream().WithVerbosity( \
      turbo_logging_internal_verbose_level)

#define TURBO_LOG_INTERNAL_VLOG_EVERY_N_SEC_IMPL(verbose_level, n_seconds)  \
  switch (const int turbo_logging_internal_verbose_level = (verbose_level)) \
  case 0:                                                                  \
  default:                                                                 \
    TURBO_LOG_INTERNAL_CONDITION_INFO(                                      \
        STATEFUL, TURBO_VLOG_IS_ON(turbo_logging_internal_verbose_level))    \
  (EveryNSec, n_seconds) TURBO_LOGGING_INTERNAL_LOG_INFO.InternalStream()   \
      .WithVerbosity(turbo_logging_internal_verbose_level)

#define TURBO_LOG_INTERNAL_LOG_IF_EVERY_N_IMPL(severity, condition, n)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_LOG_IF_FIRST_N_IMPL(severity, condition, n)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_LOG_IF_EVERY_POW_2_IMPL(severity, condition)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_LOG_IF_EVERY_N_SEC_IMPL(severity, condition,  \
                                                  n_seconds)            \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_PLOG_IF_EVERY_N_IMPL(severity, condition, n)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_IF_FIRST_N_IMPL(severity, condition, n)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_IF_EVERY_POW_2_IMPL(severity, condition) \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_LOG_INTERNAL_PLOG_IF_EVERY_N_SEC_IMPL(severity, condition, \
                                                   n_seconds)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_IMPL(severity, condition, n)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_FIRST_N_IMPL(severity, condition, n)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_POW_2_IMPL(severity, condition) \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_SEC_IMPL(severity, condition, \
                                                   n_seconds)           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#else  // def NDEBUG
#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_IMPL(severity, condition, n)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_FIRST_N_IMPL(severity, condition, n)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      FirstN, n) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_POW_2_IMPL(severity, condition)  \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryPow2) TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()

#define TURBO_LOG_INTERNAL_DLOG_IF_EVERY_N_SEC_IMPL(severity, condition,  \
                                                   n_seconds)            \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryNSec, n_seconds)                                              \
      TURBO_LOGGING_INTERNAL_DLOG##severity.InternalStream()
#endif  // def NDEBUG

#endif  // TURBO_LOG_INTERNAL_LOG_IMPL_H_
