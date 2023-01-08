
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_TIMES_INTERNAL_TIME_ZONE_POSIX_H_
#define FLARE_TIMES_INTERNAL_TIME_ZONE_POSIX_H_

#include <cstdint>
#include <string>
#include "flare/base/profile.h"

namespace flare::times_internal {

    // The date/time of the transition. The date is specified as either:
    // (J) the Nth day of the year (1 <= N <= 365), excluding leap days, or
    // (N) the Nth day of the year (0 <= N <= 365), including leap days, or
    // (M) the Nth weekday of a month (e.g., the 2nd Sunday in March).
    // The time, specified as a day offset, identifies the particular moment
    // of the transition, and may be negative or >= 24h, and in which case
    // it would take us to another day, and perhaps week, or even month.
    struct posix_transition {
        enum date_format {
            J, N, M
        };

        struct date {
            struct non_leap_day {
                std::int_fast16_t day;  // day of non-leap year [1:365]
            };
            struct day {
                std::int_fast16_t day;  // day of year [0:365]
            };
            struct month_week_weekday {
                std::int_fast8_t month;    // month of year [1:12]
                std::int_fast8_t week;     // week of month [1:5] (5==last)
                std::int_fast8_t weekday;  // 0==Sun, ..., 6=Sat
            };

            date_format fmt;

            union {
                non_leap_day j;
                day n;
                month_week_weekday m;
            };
        };

        struct time_point {
            std::int_fast32_t offset;  // seconds before/after 00:00:00
        };

        date date;
        time_point time;
    };

    // The entirety of a POSIX-string specified time-zone rule. The standard
    // abbreviation and offset are always given. If the time zone includes
    // daylight saving, then the daylight abbrevation is non-empty and the
    // remaining fields are also valid. Note that the start/end transitions
    // are not ordered---in the southern hemisphere the transition to end
    // daylight time occurs first in any particular year.
    struct posix_time_zone {
        std::string std_abbr;
        std::int_fast32_t std_offset;

        std::string dst_abbr;
        std::int_fast32_t dst_offset;
        posix_transition dst_start;
        posix_transition dst_end;
    };

    // Breaks down a POSIX time-zone specification into its constituent pieces,
    // filling in any missing values (DST offset, or start/end transition times)
    // with the standard-defined defaults. Returns false if the specification
    // could not be parsed (although some fields of *res may have been altered).
    bool parse_posix_spec(const std::string &spec, posix_time_zone *res);

}  // namespace flare::times_internal

#endif  // FLARE_TIMES_INTERNAL_TIME_ZONE_POSIX_H_
