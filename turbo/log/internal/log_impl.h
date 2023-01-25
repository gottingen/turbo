// Copyright 2022 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_LOG_INTERNAL_LOG_IMPL_H_
#define TURBO_LOG_INTERNAL_LOG_IMPL_H_

#include "turbo/log/internal/conditions.h"
#include "turbo/log/internal/log_message.h"
#include "turbo/log/internal/strip.h"

// TURBO_LOG()
#define TURBO_LOG_IMPL(severity)                          \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_PLOG()
#define TURBO_PLOG_IMPL(severity)                           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true)   \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream() \
          .WithPerror()

// TURBO_DLOG()
#ifndef NDEBUG
#define TURBO_DLOG_IMPL(severity)                         \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, true) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#else
#define TURBO_DLOG_IMPL(severity)                          \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, false) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#endif

#define TURBO_LOG_IF_IMPL(severity, condition)                 \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#define TURBO_PLOG_IF_IMPL(severity, condition)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()    \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_DLOG_IF_IMPL(severity, condition)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, condition) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#else
#define TURBO_DLOG_IF_IMPL(severity, condition)                           \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATELESS, false && (condition)) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#endif

// TURBO_LOG_EVERY_N
#define TURBO_LOG_EVERY_N_IMPL(severity, n)                         \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_FIRST_N
#define TURBO_LOG_FIRST_N_IMPL(severity, n)                         \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_EVERY_POW_2
#define TURBO_LOG_EVERY_POW_2_IMPL(severity)                        \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

// TURBO_LOG_EVERY_N_SEC
#define TURBO_LOG_EVERY_N_SEC_IMPL(severity, n_seconds)                        \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryNSec, n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_PLOG_EVERY_N_IMPL(severity, n)                        \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_PLOG_FIRST_N_IMPL(severity, n)                        \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_PLOG_EVERY_POW_2_IMPL(severity)                       \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()         \
          .WithPerror()

#define TURBO_PLOG_EVERY_N_SEC_IMPL(severity, n_seconds)                       \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, true)(EveryNSec, n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()                    \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_DLOG_EVERY_N_IMPL(severity, n)        \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true) \
  (EveryN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_FIRST_N_IMPL(severity, n)        \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true) \
  (FirstN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_EVERY_POW_2_IMPL(severity)       \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true) \
  (EveryPow2) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_EVERY_N_SEC_IMPL(severity, n_seconds) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, true)      \
  (EveryNSec, n_seconds) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#else  // def NDEBUG
#define TURBO_DLOG_EVERY_N_IMPL(severity, n)         \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false) \
  (EveryN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_FIRST_N_IMPL(severity, n)         \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false) \
  (FirstN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_EVERY_POW_2_IMPL(severity)        \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false) \
  (EveryPow2) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_EVERY_N_SEC_IMPL(severity, n_seconds) \
  TURBO_LOG_INTERNAL_CONDITION_INFO(STATEFUL, false)     \
  (EveryNSec, n_seconds) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#endif  // def NDEBUG

#define TURBO_LOG_IF_EVERY_N_IMPL(severity, condition, n)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_IF_FIRST_N_IMPL(severity, condition, n)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_IF_EVERY_POW_2_IMPL(severity, condition)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_LOG_IF_EVERY_N_SEC_IMPL(severity, condition, n_seconds)    \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_PLOG_IF_EVERY_N_IMPL(severity, condition, n)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_PLOG_IF_FIRST_N_IMPL(severity, condition, n)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_PLOG_IF_EVERY_POW_2_IMPL(severity, condition)              \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#define TURBO_PLOG_IF_EVERY_N_SEC_IMPL(severity, condition, n_seconds)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()              \
          .WithPerror()

#ifndef NDEBUG
#define TURBO_DLOG_IF_EVERY_N_IMPL(severity, condition, n)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_FIRST_N_IMPL(severity, condition, n)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(FirstN, n) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_EVERY_POW_2_IMPL(severity, condition)              \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryPow2) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_EVERY_N_SEC_IMPL(severity, condition, n_seconds)   \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, condition)(EveryNSec, \
                                                             n_seconds) \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#else  // def NDEBUG
#define TURBO_DLOG_IF_EVERY_N_IMPL(severity, condition, n)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_FIRST_N_IMPL(severity, condition, n)                \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      FirstN, n) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_EVERY_POW_2_IMPL(severity, condition)               \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryPow2) TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()

#define TURBO_DLOG_IF_EVERY_N_SEC_IMPL(severity, condition, n_seconds)    \
  TURBO_LOG_INTERNAL_CONDITION##severity(STATEFUL, false && (condition))( \
      EveryNSec, n_seconds)                                              \
      TURBO_LOGGING_INTERNAL_LOG##severity.InternalStream()
#endif  // def NDEBUG

#endif  // TURBO_LOG_INTERNAL_LOG_IMPL_H_
