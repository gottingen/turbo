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
// File: log/internal/nullstream.h
// -----------------------------------------------------------------------------
//
// Classes `NullStream`, `NullStreamMaybeFatal ` and `NullStreamFatal`
// implement a subset of the `LogMessage` API and are used instead when logging
// of messages has been disabled.

#ifndef TURBO_LOG_INTERNAL_NULLSTREAM_H_
#define TURBO_LOG_INTERNAL_NULLSTREAM_H_

#ifdef _WIN32
#include <cstdlib>
#else
#include <unistd.h>
#endif
#include <ios>
#include <ostream>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// A `NullStream` implements the API of `LogMessage` (a few methods and
// `operator<<`) but does nothing.  All methods are defined inline so the
// compiler can eliminate the whole instance and discard anything that's
// streamed in.
class NullStream {
 public:
  NullStream& AtLocation(std::string_view, int) { return *this; }
  template <typename SourceLocationType>
  NullStream& AtLocation(SourceLocationType) {
    return *this;
  }
  NullStream& NoPrefix() { return *this; }
  NullStream& WithVerbosity(int) { return *this; }
  template <typename TimeType>
  NullStream& WithTimestamp(TimeType) {
    return *this;
  }
  template <typename Tid>
  NullStream& WithThreadID(Tid) {
    return *this;
  }
  template <typename LogEntryType>
  NullStream& WithMetadataFrom(const LogEntryType&) {
    return *this;
  }
  NullStream& WithPerror() { return *this; }
  template <typename LogSinkType>
  NullStream& ToSinkAlso(LogSinkType*) {
    return *this;
  }
  template <typename LogSinkType>
  NullStream& ToSinkOnly(LogSinkType*) {
    return *this;
  }
  template <typename LogSinkType>
  NullStream& OutputToSink(LogSinkType*, bool) {
    return *this;
  }
  NullStream& InternalStream() { return *this; }
};
template <typename T>
inline NullStream& operator<<(NullStream& str, const T&) {
  return str;
}
inline NullStream& operator<<(NullStream& str,
                              std::ostream& (*)(std::ostream& os)) {
  return str;
}
inline NullStream& operator<<(NullStream& str,
                              std::ios_base& (*)(std::ios_base& os)) {
  return str;
}

// `NullStreamMaybeFatal` implements the process termination semantics of
// `LogMessage`, which is used for `DFATAL` severity and expression-defined
// severity e.g. `LOG(LEVEL(HowBadIsIt()))`.  Like `LogMessage`, it terminates
// the process when destroyed if the passed-in severity equals `FATAL`.
class NullStreamMaybeFatal final : public NullStream {
 public:
  explicit NullStreamMaybeFatal(turbo::LogSeverity severity)
      : fatal_(severity == turbo::LogSeverity::kFatal) {}
  ~NullStreamMaybeFatal() {
    if (fatal_) {
      _exit(1);
    }
  }

 private:
  bool fatal_;
};

// `NullStreamFatal` implements the process termination semantics of
// `LogMessageFatal`, which means it always terminates the process.  `DFATAL`
// and expression-defined severity use `NullStreamMaybeFatal` above.
class NullStreamFatal final : public NullStream {
 public:
  NullStreamFatal() = default;
  // TURBO_ATTRIBUTE_NORETURN doesn't seem to work on destructors with msvc, so
  // disable msvc's warning about the d'tor never returning.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4722)
#endif
  TURBO_ATTRIBUTE_NORETURN ~NullStreamFatal() { _exit(1); }
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_GLOBALS_H_
