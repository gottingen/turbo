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
// File: log/internal/strip.h
// -----------------------------------------------------------------------------
//

#ifndef TURBO_LOG_INTERNAL_STRIP_H_
#define TURBO_LOG_INTERNAL_STRIP_H_

#include "turbo/base/log_severity.h"
#include "turbo/log/internal/log_message.h"
#include "turbo/log/internal/nullstream.h"

// `TURBO_LOGGING_INTERNAL_LOG_*` evaluates to a temporary `LogMessage` object or
// to a related object with a compatible API but different behavior.  This set
// of defines comes in three flavors: vanilla, plus two variants that strip some
// logging in subtly different ways for subtly different reasons (see below).
#if defined(STRIP_LOG) && STRIP_LOG
#define TURBO_LOGGING_INTERNAL_LOG_INFO ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_WARNING ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_ERROR ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_FATAL ::turbo::log_internal::NullStreamFatal()
#define TURBO_LOGGING_INTERNAL_LOG_QFATAL ::turbo::log_internal::NullStreamFatal()
#define TURBO_LOGGING_INTERNAL_LOG_DFATAL \
  ::turbo::log_internal::NullStreamMaybeFatal(::turbo::kLogDebugFatal)
#define TURBO_LOGGING_INTERNAL_LOG_LEVEL(severity) \
  ::turbo::log_internal::NullStreamMaybeFatal(log_internal_severity)
#define TURBO_LOG_INTERNAL_CHECK(failure_message) TURBO_LOGGING_INTERNAL_LOG_FATAL
#define TURBO_LOG_INTERNAL_QCHECK(failure_message) \
  TURBO_LOGGING_INTERNAL_LOG_QFATAL
#else  // !defined(STRIP_LOG) || !STRIP_LOG
#define TURBO_LOGGING_INTERNAL_LOG_INFO                 \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, \
                                   ::turbo::LogSeverity::kInfo)
#define TURBO_LOGGING_INTERNAL_LOG_WARNING              \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, \
                                   ::turbo::LogSeverity::kWarning)
#define TURBO_LOGGING_INTERNAL_LOG_ERROR                \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, \
                                   ::turbo::LogSeverity::kError)
#define TURBO_LOGGING_INTERNAL_LOG_FATAL \
  ::turbo::log_internal::LogMessageFatal(__FILE__, __LINE__)
#define TURBO_LOGGING_INTERNAL_LOG_QFATAL \
  ::turbo::log_internal::LogMessageQuietlyFatal(__FILE__, __LINE__)
#define TURBO_LOGGING_INTERNAL_LOG_DFATAL \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, ::turbo::kLogDebugFatal)
#define TURBO_LOGGING_INTERNAL_LOG_LEVEL(severity) \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, log_internal_severity)
// These special cases dispatch to special-case constructors that allow us to
// avoid an extra function call and shrink non-LTO binaries by a percent or so.
#define TURBO_LOG_INTERNAL_CHECK(failure_message) \
  ::turbo::log_internal::LogMessageFatal(__FILE__, __LINE__, failure_message)
#define TURBO_LOG_INTERNAL_QCHECK(failure_message)                  \
  ::turbo::log_internal::LogMessageQuietlyFatal(__FILE__, __LINE__, \
                                               failure_message)
#endif  // !defined(STRIP_LOG) || !STRIP_LOG

#endif  // TURBO_LOG_INTERNAL_STRIP_H_
