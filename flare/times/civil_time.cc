
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/times/civil_time.h"
#include <cstdlib>
#include <string>
#include "flare/strings/str_cat.h"
#include "flare/times/time.h"

namespace flare {


    namespace {

        // Since a civil time has a larger year range than flare::time_point (64-bit years vs
        // 64-bit seconds, respectively) we normalize years to roughly +/- 400 years
        // around the year 2400, which will produce an equivalent year in a range that
        // flare::time_point can handle.
        FLARE_FORCE_INLINE chrono_year_t
        NormalizeYear(chrono_year_t
                      year) {
            return 2400 + year % 400;
        }  // namespace

        // Formats the given chrono_second according to the given format.
        std::string FormatYearAnd(std::string_view fmt, chrono_second cs) {
            const chrono_second ncs(NormalizeYear(cs.year()), cs.month(), cs.day(),
                                    cs.hour(), cs.minute(), cs.second());
            const time_zone utc = utc_time_zone();
            // TODO(flare-team): Avoid conversion of fmt std::string.
            return string_cat(cs.year(),
                              format_time(std::string(fmt), from_chrono(ncs, utc), utc));
        }

        template<typename CivilT>
        bool ParseYearAnd(std::string_view fmt, std::string_view s, CivilT *c) {
            // Civil times support a larger year range than flare::time_point, so we need to
            // parse the year separately, normalize it, then use flare::parse_time on the
            // normalized std::string.
            const std::string ss = std::string(s);  // TODO(flare-team): Avoid conversion.
            const char *const np = ss.c_str();
            char *endp;
            errno = 0;
            const chrono_year_t y =
                    std::strtoll(np, &endp, 10);  // NOLINT(runtime/deprecated_fn)
            if (endp == np || errno == ERANGE) return false;
            const std::string norm = string_cat(NormalizeYear(y), endp);

            const time_zone utc = utc_time_zone();
            time_point t;
            if (parse_time(string_cat("%Y", fmt), norm, utc, &t, nullptr)) {
                const auto cs = to_chrono_second(t, utc);
                *c = CivilT(y, cs.month(), cs.day(), cs.hour(), cs.minute(), cs.second());
                return true;
            }

            return false;
        }

// Tries to parse the type as a CivilT1, but then assigns the result to the
// argument of type CivilT2.
        template<typename CivilT1, typename CivilT2>
        bool ParseAs(std::string_view s, CivilT2 *c) {
            CivilT1 t1;
            if (parse_chrono_time(s, &t1)) {
                *c = CivilT2(t1);
                return true;
            }
            return false;
        }

        template<typename CivilT>
        bool ParseLenient(std::string_view s, CivilT *c) {
            // A fastpath for when the given std::string data parses exactly into the given
            // type T (e.g., s="YYYY-MM-DD" and CivilT=chrono_day).
            if (parse_chrono_time(s, c)) return true;
            // Try parsing as each of the 6 types, trying the most common types first
            // (based on csearch results).
            if (ParseAs<chrono_day>(s, c)) return true;
            if (ParseAs<chrono_second>(s, c)) return true;
            if (ParseAs<chrono_hour>(s, c)) return true;
            if (ParseAs<chrono_month>(s, c)) return true;
            if (ParseAs<chrono_minute>(s, c)) return true;
            if (ParseAs<chrono_year>(s, c)) return true;
            return false;
        }
    }  // namespace

    std::string format_chrono_time(chrono_second c) {
        return FormatYearAnd("-%m-%dT%H:%M:%S", c);
    }

    std::string format_chrono_time(chrono_minute c) {
        return FormatYearAnd("-%m-%dT%H:%M", c);
    }

    std::string format_chrono_time(chrono_hour c) {
        return FormatYearAnd("-%m-%dT%H", c);
    }

    std::string format_chrono_time(chrono_day c) { return FormatYearAnd("-%m-%d", c); }

    std::string format_chrono_time(chrono_month c) { return FormatYearAnd("-%m", c); }

    std::string format_chrono_time(chrono_year c) { return FormatYearAnd("", c); }

    bool parse_chrono_time(std::string_view s, chrono_second *c) {
        return ParseYearAnd("-%m-%dT%H:%M:%S", s, c);
    }

    bool parse_chrono_time(std::string_view s, chrono_minute *c) {
        return ParseYearAnd("-%m-%dT%H:%M", s, c);
    }

    bool parse_chrono_time(std::string_view s, chrono_hour *c) {
        return ParseYearAnd("-%m-%dT%H", s, c);
    }

    bool parse_chrono_time(std::string_view s, chrono_day *c) {
        return ParseYearAnd("-%m-%d", s, c);
    }

    bool parse_chrono_time(std::string_view s, chrono_month *c) {
        return ParseYearAnd("-%m", s, c);
    }

    bool parse_chrono_time(std::string_view s, chrono_year *c) {
        return ParseYearAnd("", s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_second *c) {
        return ParseLenient(s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_minute *c) {
        return ParseLenient(s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_hour *c) {
        return ParseLenient(s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_day *c) {
        return ParseLenient(s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_month *c) {
        return ParseLenient(s, c);
    }

    bool ParseLenientCivilTime(std::string_view s, chrono_year *c) {
        return ParseLenient(s, c);
    }

    namespace times_internal {

        std::ostream &operator<<(std::ostream &os, chrono_year y) {
            return os << format_chrono_time(y);
        }

        std::ostream &operator<<(std::ostream &os, chrono_month m) {
            return os << format_chrono_time(m);
        }

        std::ostream &operator<<(std::ostream &os, chrono_day d) {
            return os << format_chrono_time(d);
        }

        std::ostream &operator<<(std::ostream &os, chrono_hour h) {
            return os << format_chrono_time(h);
        }

        std::ostream &operator<<(std::ostream &os, chrono_minute m) {
            return os << format_chrono_time(m);
        }

        std::ostream &operator<<(std::ostream &os, chrono_second s) {
            return os << format_chrono_time(s);
        }

    }  // namespace times_internal


}  // namespace flare
