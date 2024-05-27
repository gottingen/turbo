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

#include <turbo/time/civil_time.h>

#include <cstdlib>
#include <ostream>
#include <string>

#include <turbo/strings/str_cat.h>
#include <turbo/time/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

// Since a civil time has a larger year range than turbo::Time (64-bit years vs
// 64-bit seconds, respectively) we normalize years to roughly +/- 400 years
// around the year 2400, which will produce an equivalent year in a range that
// turbo::Time can handle.
inline civil_year_t NormalizeYear(civil_year_t year) {
  return 2400 + year % 400;
}

// Formats the given CivilSecond according to the given format.
std::string FormatYearAnd(string_view fmt, CivilSecond cs) {
  const CivilSecond ncs(NormalizeYear(cs.year()), cs.month(), cs.day(),
                        cs.hour(), cs.minute(), cs.second());
  const TimeZone utc = UTCTimeZone();
  return StrCat(cs.year(), FormatTime(fmt, FromCivil(ncs, utc), utc));
}

template <typename CivilT>
bool ParseYearAnd(string_view fmt, string_view s, CivilT* c) {
  // Civil times support a larger year range than turbo::Time, so we need to
  // parse the year separately, normalize it, then use turbo::ParseTime on the
  // normalized string.
  const std::string ss = std::string(s);  // TODO(turbo-team): Avoid conversion.
  const char* const np = ss.c_str();
  char* endp;
  errno = 0;
  const civil_year_t y =
      std::strtoll(np, &endp, 10);  // NOLINT(runtime/deprecated_fn)
  if (endp == np || errno == ERANGE) return false;
  const std::string norm = StrCat(NormalizeYear(y), endp);

  const TimeZone utc = UTCTimeZone();
  Time t;
  if (ParseTime(StrCat("%Y", fmt), norm, utc, &t, nullptr)) {
    const auto cs = ToCivilSecond(t, utc);
    *c = CivilT(y, cs.month(), cs.day(), cs.hour(), cs.minute(), cs.second());
    return true;
  }

  return false;
}

// Tries to parse the type as a CivilT1, but then assigns the result to the
// argument of type CivilT2.
template <typename CivilT1, typename CivilT2>
bool ParseAs(string_view s, CivilT2* c) {
  CivilT1 t1;
  if (ParseCivilTime(s, &t1)) {
    *c = CivilT2(t1);
    return true;
  }
  return false;
}

template <typename CivilT>
bool ParseLenient(string_view s, CivilT* c) {
  // A fastpath for when the given string data parses exactly into the given
  // type T (e.g., s="YYYY-MM-DD" and CivilT=CivilDay).
  if (ParseCivilTime(s, c)) return true;
  // Try parsing as each of the 6 types, trying the most common types first
  // (based on csearch results).
  if (ParseAs<CivilDay>(s, c)) return true;
  if (ParseAs<CivilSecond>(s, c)) return true;
  if (ParseAs<CivilHour>(s, c)) return true;
  if (ParseAs<CivilMonth>(s, c)) return true;
  if (ParseAs<CivilMinute>(s, c)) return true;
  if (ParseAs<CivilYear>(s, c)) return true;
  return false;
}
}  // namespace

std::string FormatCivilTime(CivilSecond c) {
  return FormatYearAnd("-%m-%d%ET%H:%M:%S", c);
}
std::string FormatCivilTime(CivilMinute c) {
  return FormatYearAnd("-%m-%d%ET%H:%M", c);
}
std::string FormatCivilTime(CivilHour c) {
  return FormatYearAnd("-%m-%d%ET%H", c);
}
std::string FormatCivilTime(CivilDay c) { return FormatYearAnd("-%m-%d", c); }
std::string FormatCivilTime(CivilMonth c) { return FormatYearAnd("-%m", c); }
std::string FormatCivilTime(CivilYear c) { return FormatYearAnd("", c); }

bool ParseCivilTime(string_view s, CivilSecond* c) {
  return ParseYearAnd("-%m-%d%ET%H:%M:%S", s, c);
}
bool ParseCivilTime(string_view s, CivilMinute* c) {
  return ParseYearAnd("-%m-%d%ET%H:%M", s, c);
}
bool ParseCivilTime(string_view s, CivilHour* c) {
  return ParseYearAnd("-%m-%d%ET%H", s, c);
}
bool ParseCivilTime(string_view s, CivilDay* c) {
  return ParseYearAnd("-%m-%d", s, c);
}
bool ParseCivilTime(string_view s, CivilMonth* c) {
  return ParseYearAnd("-%m", s, c);
}
bool ParseCivilTime(string_view s, CivilYear* c) {
  return ParseYearAnd("", s, c);
}

bool ParseLenientCivilTime(string_view s, CivilSecond* c) {
  return ParseLenient(s, c);
}
bool ParseLenientCivilTime(string_view s, CivilMinute* c) {
  return ParseLenient(s, c);
}
bool ParseLenientCivilTime(string_view s, CivilHour* c) {
  return ParseLenient(s, c);
}
bool ParseLenientCivilTime(string_view s, CivilDay* c) {
  return ParseLenient(s, c);
}
bool ParseLenientCivilTime(string_view s, CivilMonth* c) {
  return ParseLenient(s, c);
}
bool ParseLenientCivilTime(string_view s, CivilYear* c) {
  return ParseLenient(s, c);
}

namespace time_internal {

std::ostream& operator<<(std::ostream& os, CivilYear y) {
  return os << FormatCivilTime(y);
}
std::ostream& operator<<(std::ostream& os, CivilMonth m) {
  return os << FormatCivilTime(m);
}
std::ostream& operator<<(std::ostream& os, CivilDay d) {
  return os << FormatCivilTime(d);
}
std::ostream& operator<<(std::ostream& os, CivilHour h) {
  return os << FormatCivilTime(h);
}
std::ostream& operator<<(std::ostream& os, CivilMinute m) {
  return os << FormatCivilTime(m);
}
std::ostream& operator<<(std::ostream& os, CivilSecond s) {
  return os << FormatCivilTime(s);
}

bool turbo_parse_flag(string_view s, CivilSecond* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
bool turbo_parse_flag(string_view s, CivilMinute* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
bool turbo_parse_flag(string_view s, CivilHour* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
bool turbo_parse_flag(string_view s, CivilDay* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
bool turbo_parse_flag(string_view s, CivilMonth* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
bool turbo_parse_flag(string_view s, CivilYear* c, std::string*) {
  return ParseLenientCivilTime(s, c);
}
std::string turbo_unparse_flag(CivilSecond c) { return FormatCivilTime(c); }
std::string turbo_unparse_flag(CivilMinute c) { return FormatCivilTime(c); }
std::string turbo_unparse_flag(CivilHour c) { return FormatCivilTime(c); }
std::string turbo_unparse_flag(CivilDay c) { return FormatCivilTime(c); }
std::string turbo_unparse_flag(CivilMonth c) { return FormatCivilTime(c); }
std::string turbo_unparse_flag(CivilYear c) { return FormatCivilTime(c); }

}  // namespace time_internal

TURBO_NAMESPACE_END
}  // namespace turbo
