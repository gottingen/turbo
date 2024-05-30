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

#include <turbo/flags/marshalling.h>

#include <stddef.h>

#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <turbo/base/config.h>
#include <turbo/base/log_severity.h>
#include <turbo/base/macros.h>
#include <turbo/numeric/int128.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/match.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_format.h>
#include <turbo/strings/str_join.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

// --------------------------------------------------------------------
// turbo_parse_flag specializations for boolean type.

bool turbo_parse_flag(turbo::string_view text, bool* dst, std::string*) {
  const char* kTrue[] = {"1", "t", "true", "y", "yes"};
  const char* kFalse[] = {"0", "f", "false", "n", "no"};
  static_assert(sizeof(kTrue) == sizeof(kFalse), "true_false_equal");

  text = turbo::trim_all(text);

  for (size_t i = 0; i < TURBO_ARRAYSIZE(kTrue); ++i) {
    if (turbo::equals_ignore_case(text, kTrue[i])) {
      *dst = true;
      return true;
    } else if (turbo::equals_ignore_case(text, kFalse[i])) {
      *dst = false;
      return true;
    }
  }
  return false;  // didn't match a legal input
}

// --------------------------------------------------------------------
// turbo_parse_flag for integral types.

// Return the base to use for parsing text as an integer.  Leading 0x
// puts us in base 16.  But leading 0 does not put us in base 8. It
// caused too many bugs when we had that behavior.
static int NumericBase(turbo::string_view text) {
  if (text.empty()) return 0;
  size_t num_start = (text[0] == '-' || text[0] == '+') ? 1 : 0;
  const bool hex = (text.size() >= num_start + 2 && text[num_start] == '0' &&
                    (text[num_start + 1] == 'x' || text[num_start + 1] == 'X'));
  return hex ? 16 : 10;
}

template <typename IntType>
inline bool ParseFlagImpl(turbo::string_view text, IntType& dst) {
  text = turbo::trim_all(text);

  return turbo::numbers_internal::safe_strtoi_base(text, &dst,
                                                  NumericBase(text));
}

bool turbo_parse_flag(turbo::string_view text, short* dst, std::string*) {
  int val;
  if (!ParseFlagImpl(text, val)) return false;
  if (static_cast<short>(val) != val)  // worked, but number out of range
    return false;
  *dst = static_cast<short>(val);
  return true;
}

bool turbo_parse_flag(turbo::string_view text, unsigned short* dst, std::string*) {
  unsigned int val;
  if (!ParseFlagImpl(text, val)) return false;
  if (static_cast<unsigned short>(val) !=
      val)  // worked, but number out of range
    return false;
  *dst = static_cast<unsigned short>(val);
  return true;
}

bool turbo_parse_flag(turbo::string_view text, int* dst, std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, unsigned int* dst, std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, long* dst, std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, unsigned long* dst, std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, long long* dst, std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, unsigned long long* dst,
                   std::string*) {
  return ParseFlagImpl(text, *dst);
}

bool turbo_parse_flag(turbo::string_view text, turbo::int128* dst, std::string*) {
  text = turbo::trim_all(text);

  // check hex
  int base = NumericBase(text);
  if (!turbo::numbers_internal::safe_strto128_base(text, dst, base)) {
    return false;
  }

  return base == 16 ? turbo::simple_hex_atoi(text, dst)
                    : turbo::simple_atoi(text, dst);
}

bool turbo_parse_flag(turbo::string_view text, turbo::uint128* dst, std::string*) {
  text = turbo::trim_all(text);

  // check hex
  int base = NumericBase(text);
  if (!turbo::numbers_internal::safe_strtou128_base(text, dst, base)) {
    return false;
  }

  return base == 16 ? turbo::simple_hex_atoi(text, dst)
                    : turbo::simple_atoi(text, dst);
}

// --------------------------------------------------------------------
// turbo_parse_flag for floating point types.

bool turbo_parse_flag(turbo::string_view text, float* dst, std::string*) {
  return turbo::simple_atof(text, dst);
}

bool turbo_parse_flag(turbo::string_view text, double* dst, std::string*) {
  return turbo::simple_atod(text, dst);
}

// --------------------------------------------------------------------
// turbo_parse_flag for strings.

bool turbo_parse_flag(turbo::string_view text, std::string* dst, std::string*) {
  dst->assign(text.data(), text.size());
  return true;
}

// --------------------------------------------------------------------
// turbo_parse_flag for vector of strings.

bool turbo_parse_flag(turbo::string_view text, std::vector<std::string>* dst,
                   std::string*) {
  // An empty flag value corresponds to an empty vector, not a vector
  // with a single, empty std::string.
  if (text.empty()) {
    dst->clear();
    return true;
  }
  *dst = turbo::StrSplit(text, ',', turbo::AllowEmpty());
  return true;
}

// --------------------------------------------------------------------
// turbo_unparse_flag specializations for various builtin flag types.

std::string Unparse(bool v) { return v ? "true" : "false"; }
std::string Unparse(short v) { return turbo::str_cat(v); }
std::string Unparse(unsigned short v) { return turbo::str_cat(v); }
std::string Unparse(int v) { return turbo::str_cat(v); }
std::string Unparse(unsigned int v) { return turbo::str_cat(v); }
std::string Unparse(long v) { return turbo::str_cat(v); }
std::string Unparse(unsigned long v) { return turbo::str_cat(v); }
std::string Unparse(long long v) { return turbo::str_cat(v); }
std::string Unparse(unsigned long long v) { return turbo::str_cat(v); }
std::string Unparse(turbo::int128 v) {
  std::stringstream ss;
  ss << v;
  return ss.str();
}
std::string Unparse(turbo::uint128 v) {
  std::stringstream ss;
  ss << v;
  return ss.str();
}

template <typename T>
std::string UnparseFloatingPointVal(T v) {
  // digits10 is guaranteed to roundtrip correctly in string -> value -> string
  // conversions, but may not be enough to represent all the values correctly.
  std::string digit10_str =
      turbo::str_format("%.*g", std::numeric_limits<T>::digits10, v);
  if (std::isnan(v) || std::isinf(v)) return digit10_str;

  T roundtrip_val = 0;
  std::string err;
  if (turbo::ParseFlag(digit10_str, &roundtrip_val, &err) &&
      roundtrip_val == v) {
    return digit10_str;
  }

  // max_digits10 is the number of base-10 digits that are necessary to uniquely
  // represent all distinct values.
  return turbo::str_format("%.*g", std::numeric_limits<T>::max_digits10, v);
}
std::string Unparse(float v) { return UnparseFloatingPointVal(v); }
std::string Unparse(double v) { return UnparseFloatingPointVal(v); }
std::string turbo_unparse_flag(turbo::string_view v) { return std::string(v); }
std::string turbo_unparse_flag(const std::vector<std::string>& v) {
  return turbo::str_join(v, ",");
}

}  // namespace flags_internal

bool turbo_parse_flag(turbo::string_view text, turbo::LogSeverity* dst,
                   std::string* err) {
  text = turbo::trim_all(text);
  if (text.empty()) {
    *err = "no value provided";
    return false;
  }
  if (turbo::equals_ignore_case(text, "dfatal")) {
    *dst = turbo::kLogDebugFatal;
    return true;
  }
  if (turbo::equals_ignore_case(text, "klogdebugfatal")) {
    *dst = turbo::kLogDebugFatal;
    return true;
  }
  if (text.front() == 'k' || text.front() == 'K') text.remove_prefix(1);
  if (turbo::equals_ignore_case(text, "info")) {
    *dst = turbo::LogSeverity::kInfo;
    return true;
  }
  if (turbo::equals_ignore_case(text, "warning")) {
    *dst = turbo::LogSeverity::kWarning;
    return true;
  }
  if (turbo::equals_ignore_case(text, "error")) {
    *dst = turbo::LogSeverity::kError;
    return true;
  }
  if (turbo::equals_ignore_case(text, "fatal")) {
    *dst = turbo::LogSeverity::kFatal;
    return true;
  }
  std::underlying_type<turbo::LogSeverity>::type numeric_value;
  if (turbo::ParseFlag(text, &numeric_value, err)) {
    *dst = static_cast<turbo::LogSeverity>(numeric_value);
    return true;
  }
  *err =
      "only integers, turbo::LogSeverity enumerators, and DFATAL are accepted";
  return false;
}

std::string turbo_unparse_flag(turbo::LogSeverity v) {
  if (v == turbo::NormalizeLogSeverity(v)) return turbo::LogSeverityName(v);
  return turbo::UnparseFlag(static_cast<int>(v));
}

TURBO_NAMESPACE_END
}  // namespace turbo
