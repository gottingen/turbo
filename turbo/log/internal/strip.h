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
// File: log/internal/strip.h
// -----------------------------------------------------------------------------
//

#ifndef TURBO_LOG_INTERNAL_STRIP_H_
#define TURBO_LOG_INTERNAL_STRIP_H_

#include <turbo/base/attributes.h>  // IWYU pragma: keep
#include <turbo/base/log_severity.h>
#include <turbo/log/internal/log_message.h>
#include <turbo/log/internal/nullstream.h>

// `TURBO_LOGGING_INTERNAL_LOG_*` evaluates to a temporary `LogMessage` object or
// to a related object with a compatible API but different behavior.  This set
// of defines comes in three flavors: vanilla, plus two variants that strip some
// logging in subtly different ways for subtly different reasons (see below).
#if defined(STRIP_LOG) && STRIP_LOG

// Attribute for marking variables used in implementation details of logging
// macros as unused, but only when `STRIP_LOG` is defined.
// With `STRIP_LOG` on, not marking them triggers `-Wunused-but-set-variable`,
// With `STRIP_LOG` off, marking them triggers `-Wused-but-marked-unused`.
//
// TODO(b/290784225): Replace this macro with attribute [[maybe_unused]] when
// Turbo stops supporting C++14.
#define TURBO_LOG_INTERNAL_ATTRIBUTE_UNUSED_IF_STRIP_LOG TURBO_ATTRIBUTE_UNUSED

#define TURBO_LOGGING_INTERNAL_LOG_INFO ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_WARNING ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_ERROR ::turbo::log_internal::NullStream()
#define TURBO_LOGGING_INTERNAL_LOG_FATAL ::turbo::log_internal::NullStreamFatal()
#define TURBO_LOGGING_INTERNAL_LOG_QFATAL ::turbo::log_internal::NullStreamFatal()
#define TURBO_LOGGING_INTERNAL_LOG_DFATAL \
  ::turbo::log_internal::NullStreamMaybeFatal(::turbo::kLogDebugFatal)
#define TURBO_LOGGING_INTERNAL_LOG_LEVEL(severity) \
  ::turbo::log_internal::NullStreamMaybeFatal(turbo_log_internal_severity)

// Fatal `DLOG`s expand a little differently to avoid being `[[noreturn]]`.
#define TURBO_LOGGING_INTERNAL_DLOG_FATAL \
  ::turbo::log_internal::NullStreamMaybeFatal(::turbo::LogSeverity::kFatal)
#define TURBO_LOGGING_INTERNAL_DLOG_QFATAL \
  ::turbo::log_internal::NullStreamMaybeFatal(::turbo::LogSeverity::kFatal)

#define TURBO_LOG_INTERNAL_CHECK(failure_message) TURBO_LOGGING_INTERNAL_LOG_FATAL
#define TURBO_LOG_INTERNAL_QCHECK(failure_message) \
  TURBO_LOGGING_INTERNAL_LOG_QFATAL

#else  // !defined(STRIP_LOG) || !STRIP_LOG

#define TURBO_LOG_INTERNAL_ATTRIBUTE_UNUSED_IF_STRIP_LOG

#define TURBO_LOGGING_INTERNAL_LOG_INFO \
  ::turbo::log_internal::LogMessage(    \
      __FILE__, __LINE__, ::turbo::log_internal::LogMessage::InfoTag{})
#define TURBO_LOGGING_INTERNAL_LOG_WARNING \
  ::turbo::log_internal::LogMessage(       \
      __FILE__, __LINE__, ::turbo::log_internal::LogMessage::WarningTag{})
#define TURBO_LOGGING_INTERNAL_LOG_ERROR \
  ::turbo::log_internal::LogMessage(     \
      __FILE__, __LINE__, ::turbo::log_internal::LogMessage::ErrorTag{})
#define TURBO_LOGGING_INTERNAL_LOG_FATAL \
  ::turbo::log_internal::LogMessageFatal(__FILE__, __LINE__)
#define TURBO_LOGGING_INTERNAL_LOG_QFATAL \
  ::turbo::log_internal::LogMessageQuietlyFatal(__FILE__, __LINE__)
#define TURBO_LOGGING_INTERNAL_LOG_DFATAL \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, ::turbo::kLogDebugFatal)
#define TURBO_LOGGING_INTERNAL_LOG_LEVEL(severity)      \
  ::turbo::log_internal::LogMessage(__FILE__, __LINE__, \
                                   turbo_log_internal_severity)

// Fatal `DLOG`s expand a little differently to avoid being `[[noreturn]]`.
#define TURBO_LOGGING_INTERNAL_DLOG_FATAL \
  ::turbo::log_internal::LogMessageDebugFatal(__FILE__, __LINE__)
#define TURBO_LOGGING_INTERNAL_DLOG_QFATAL \
  ::turbo::log_internal::LogMessageQuietlyDebugFatal(__FILE__, __LINE__)

// These special cases dispatch to special-case constructors that allow us to
// avoid an extra function call and shrink non-LTO binaries by a percent or so.
#define TURBO_LOG_INTERNAL_CHECK(failure_message) \
  ::turbo::log_internal::LogMessageFatal(__FILE__, __LINE__, failure_message)
#define TURBO_LOG_INTERNAL_QCHECK(failure_message)                  \
  ::turbo::log_internal::LogMessageQuietlyFatal(__FILE__, __LINE__, \
                                               failure_message)
#endif  // !defined(STRIP_LOG) || !STRIP_LOG

// This part of a non-fatal `DLOG`s expands the same as `LOG`.
#define TURBO_LOGGING_INTERNAL_DLOG_INFO TURBO_LOGGING_INTERNAL_LOG_INFO
#define TURBO_LOGGING_INTERNAL_DLOG_WARNING TURBO_LOGGING_INTERNAL_LOG_WARNING
#define TURBO_LOGGING_INTERNAL_DLOG_ERROR TURBO_LOGGING_INTERNAL_LOG_ERROR
#define TURBO_LOGGING_INTERNAL_DLOG_DFATAL TURBO_LOGGING_INTERNAL_LOG_DFATAL
#define TURBO_LOGGING_INTERNAL_DLOG_LEVEL TURBO_LOGGING_INTERNAL_LOG_LEVEL

#endif  // TURBO_LOG_INTERNAL_STRIP_H_
