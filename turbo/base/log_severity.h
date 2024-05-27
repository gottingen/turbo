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

#ifndef TURBO_BASE_LOG_SEVERITY_H_
#define TURBO_BASE_LOG_SEVERITY_H_

#include <array>
#include <ostream>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// turbo::LogSeverity
//
// Four severity levels are defined. Logging APIs should terminate the program
// when a message is logged at severity `kFatal`; the other levels have no
// special semantics.
//
// Values other than the four defined levels (e.g. produced by `static_cast`)
// are valid, but their semantics when passed to a function, macro, or flag
// depend on the function, macro, or flag. The usual behavior is to normalize
// such values to a defined severity level, however in some cases values other
// than the defined levels are useful for comparison.
//
// Example:
//
//   // Effectively disables all logging:
//   SetMinLogLevel(static_cast<turbo::LogSeverity>(100));
//
// Turbo flags may be defined with type `LogSeverity`. Dependency layering
// constraints require that the `turbo_parse_flag()` overload be declared and
// defined in the flags library itself rather than here. The `turbo_unparse_flag()`
// overload is defined there as well for consistency.
//
// turbo::LogSeverity Flag String Representation
//
// An `turbo::LogSeverity` has a string representation used for parsing
// command-line flags based on the enumerator name (e.g. `kFatal`) or
// its unprefixed name (without the `k`) in any case-insensitive form. (E.g.
// "FATAL", "fatal" or "Fatal" are all valid.) Unparsing such flags produces an
// unprefixed string representation in all caps (e.g. "FATAL") or an integer.
//
// Additionally, the parser accepts arbitrary integers (as if the type were
// `int`).
//
// Examples:
//
//   --my_log_level=kInfo
//   --my_log_level=INFO
//   --my_log_level=info
//   --my_log_level=0
//
// `DFATAL` and `kLogDebugFatal` are similarly accepted.
//
// Unparsing a flag produces the same result as `turbo::LogSeverityName()` for
// the standard levels and a base-ten integer otherwise.
enum class LogSeverity : int {
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

// LogSeverities()
//
// Returns an iterable of all standard `turbo::LogSeverity` values, ordered from
// least to most severe.
constexpr std::array<turbo::LogSeverity, 4> LogSeverities() {
  return {{turbo::LogSeverity::kInfo, turbo::LogSeverity::kWarning,
           turbo::LogSeverity::kError, turbo::LogSeverity::kFatal}};
}

// `turbo::kLogDebugFatal` equals `turbo::LogSeverity::kFatal` in debug builds
// (i.e. when `NDEBUG` is not defined) and `turbo::LogSeverity::kError`
// otherwise.  Avoid ODR-using this variable as it has internal linkage and thus
// distinct storage in different TUs.
#ifdef NDEBUG
static constexpr turbo::LogSeverity kLogDebugFatal = turbo::LogSeverity::kError;
#else
static constexpr turbo::LogSeverity kLogDebugFatal = turbo::LogSeverity::kFatal;
#endif

// LogSeverityName()
//
// Returns the all-caps string representation (e.g. "INFO") of the specified
// severity level if it is one of the standard levels and "UNKNOWN" otherwise.
constexpr const char* LogSeverityName(turbo::LogSeverity s) {
  switch (s) {
    case turbo::LogSeverity::kInfo: return "INFO";
    case turbo::LogSeverity::kWarning: return "WARNING";
    case turbo::LogSeverity::kError: return "ERROR";
    case turbo::LogSeverity::kFatal: return "FATAL";
  }
  return "UNKNOWN";
}

// NormalizeLogSeverity()
//
// Values less than `kInfo` normalize to `kInfo`; values greater than `kFatal`
// normalize to `kError` (**NOT** `kFatal`).
constexpr turbo::LogSeverity NormalizeLogSeverity(turbo::LogSeverity s) {
  turbo::LogSeverity n = s;
  if (n < turbo::LogSeverity::kInfo) n = turbo::LogSeverity::kInfo;
  if (n > turbo::LogSeverity::kFatal) n = turbo::LogSeverity::kError;
  return n;
}
constexpr turbo::LogSeverity NormalizeLogSeverity(int s) {
  return turbo::NormalizeLogSeverity(static_cast<turbo::LogSeverity>(s));
}

// operator<<
//
// The exact representation of a streamed `turbo::LogSeverity` is deliberately
// unspecified; do not rely on it.
std::ostream& operator<<(std::ostream& os, turbo::LogSeverity s);

// Enums representing a lower bound for LogSeverity. APIs that only operate on
// messages of at least a certain level (for example, `SetMinLogLevel()`) use
// this type to specify that level. turbo::LogSeverityAtLeast::kInfinity is
// a level above all threshold levels and therefore no log message will
// ever meet this threshold.
enum class LogSeverityAtLeast : int {
  kInfo = static_cast<int>(turbo::LogSeverity::kInfo),
  kWarning = static_cast<int>(turbo::LogSeverity::kWarning),
  kError = static_cast<int>(turbo::LogSeverity::kError),
  kFatal = static_cast<int>(turbo::LogSeverity::kFatal),
  kInfinity = 1000,
};

std::ostream& operator<<(std::ostream& os, turbo::LogSeverityAtLeast s);

// Enums representing an upper bound for LogSeverity. APIs that only operate on
// messages of at most a certain level (for example, buffer all messages at or
// below a certain level) use this type to specify that level.
// turbo::LogSeverityAtMost::kNegativeInfinity is a level below all threshold
// levels and therefore will exclude all log messages.
enum class LogSeverityAtMost : int {
  kNegativeInfinity = -1000,
  kInfo = static_cast<int>(turbo::LogSeverity::kInfo),
  kWarning = static_cast<int>(turbo::LogSeverity::kWarning),
  kError = static_cast<int>(turbo::LogSeverity::kError),
  kFatal = static_cast<int>(turbo::LogSeverity::kFatal),
};

std::ostream& operator<<(std::ostream& os, turbo::LogSeverityAtMost s);

#define COMPOP(op1, op2, T)                                         \
  constexpr bool operator op1(turbo::T lhs, turbo::LogSeverity rhs) { \
    return static_cast<turbo::LogSeverity>(lhs) op1 rhs;             \
  }                                                                 \
  constexpr bool operator op2(turbo::LogSeverity lhs, turbo::T rhs) { \
    return lhs op2 static_cast<turbo::LogSeverity>(rhs);             \
  }

// Comparisons between `LogSeverity` and `LogSeverityAtLeast`/
// `LogSeverityAtMost` are only supported in one direction.
// Valid checks are:
//   LogSeverity >= LogSeverityAtLeast
//   LogSeverity < LogSeverityAtLeast
//   LogSeverity <= LogSeverityAtMost
//   LogSeverity > LogSeverityAtMost
COMPOP(>, <, LogSeverityAtLeast)
COMPOP(<=, >=, LogSeverityAtLeast)
COMPOP(<, >, LogSeverityAtMost)
COMPOP(>=, <=, LogSeverityAtMost)
#undef COMPOP

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_LOG_SEVERITY_H_
