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

#include "turbo/log/internal/log_impl.h"

#define TURBO_LOG(severity) TURBO_LOG_IMPL(_##severity)
#define TURBO_PLOG(severity) TURBO_PLOG_IMPL(_##severity)
#define TURBO_DLOG(severity) TURBO_DLOG_IMPL(_##severity)

#define TURBO_LOG_IF(severity, condition) \
  TURBO_LOG_IF_IMPL(_##severity, condition)
#define TURBO_PLOG_IF(severity, condition) \
  TURBO_PLOG_IF_IMPL(_##severity, condition)
#define TURBO_DLOG_IF(severity, condition) \
  TURBO_DLOG_IF_IMPL(_##severity, condition)

#define TURBO_LOG_EVERY_N(severity, n) TURBO_LOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_LOG_FIRST_N(severity, n) TURBO_LOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_LOG_EVERY_POW_2(severity) TURBO_LOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_LOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_LOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_PLOG_EVERY_N(severity, n) TURBO_PLOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_PLOG_FIRST_N(severity, n) TURBO_PLOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_PLOG_EVERY_POW_2(severity) TURBO_PLOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_PLOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_PLOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_DLOG_EVERY_N(severity, n) TURBO_DLOG_EVERY_N_IMPL(_##severity, n)
#define TURBO_DLOG_FIRST_N(severity, n) TURBO_DLOG_FIRST_N_IMPL(_##severity, n)
#define TURBO_DLOG_EVERY_POW_2(severity) TURBO_DLOG_EVERY_POW_2_IMPL(_##severity)
#define TURBO_DLOG_EVERY_N_SEC(severity, n_seconds) \
  TURBO_DLOG_EVERY_N_SEC_IMPL(_##severity, n_seconds)

#define TURBO_LOG_IF_EVERY_N(severity, condition, n) \
  TURBO_LOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_LOG_IF_FIRST_N(severity, condition, n) \
  TURBO_LOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_LOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_LOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_LOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_LOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#define TURBO_PLOG_IF_EVERY_N(severity, condition, n) \
  TURBO_PLOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_PLOG_IF_FIRST_N(severity, condition, n) \
  TURBO_PLOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_PLOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_PLOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_PLOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_PLOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#define TURBO_DLOG_IF_EVERY_N(severity, condition, n) \
  TURBO_DLOG_IF_EVERY_N_IMPL(_##severity, condition, n)
#define TURBO_DLOG_IF_FIRST_N(severity, condition, n) \
  TURBO_DLOG_IF_FIRST_N_IMPL(_##severity, condition, n)
#define TURBO_DLOG_IF_EVERY_POW_2(severity, condition) \
  TURBO_DLOG_IF_EVERY_POW_2_IMPL(_##severity, condition)
#define TURBO_DLOG_IF_EVERY_N_SEC(severity, condition, n_seconds) \
  TURBO_DLOG_IF_EVERY_N_SEC_IMPL(_##severity, condition, n_seconds)

#endif  // TURBO_LOG_TURBO_LOG_H_
