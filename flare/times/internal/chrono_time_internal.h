
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_INTERNAL_CHRONO_TIME_INTERNAL_H_
#define FLARE_TIMES_INTERNAL_CHRONO_TIME_INTERNAL_H_

#include "flare/base/profile.h"
#include "flare/times/internal/chrono_time_detail.h"

namespace flare::times_internal {

    using civil_year = times_detail::civil_year;
    using civil_month = times_detail::civil_month;
    using civil_day = times_detail::civil_day;
    using civil_hour = times_detail::civil_hour;
    using civil_minute = times_detail::civil_minute;
    using civil_second = times_detail::civil_second;

    // An enum class with members monday, tuesday, wednesday, thursday, friday,
    // saturday, and sunday. These enum values may be sent to an output stream
    // using operator<<(). The result is the full weekday name in English with a
    // leading capital letter.
    //
    //   weekday wd = weekday::thursday;
    //   std::cout << wd << "\n";  // Outputs: Thursday
    //
    using times_detail::weekday;

    // Returns the weekday for the given civil-time value.
    //
    //   civil_day a(2015, 8, 13);
    //   weekday wd = get_weekday(a);  // wd == weekday::thursday
    //
    using times_detail::get_weekday;

    // Returns the civil_day that strictly follows or precedes the given
    // civil_day, and that falls on the given weekday.
    //
    // For example, given:
    //
    //     August 2015
    // Su Mo Tu We Th Fr Sa
    //                    1
    //  2  3  4  5  6  7  8
    //  9 10 11 12 13 14 15
    // 16 17 18 19 20 21 22
    // 23 24 25 26 27 28 29
    // 30 31
    //
    //   civil_day a(2015, 8, 13);  // get_weekday(a) == weekday::thursday
    //   civil_day b = next_weekday(a, weekday::thursday);  // b = 2015-08-20
    //   civil_day c = prev_weekday(a, weekday::thursday);  // c = 2015-08-06
    //
    //   civil_day d = ...
    //   // Gets the following Thursday if d is not already Thursday
    //   civil_day thurs1 = next_weekday(d - 1, weekday::thursday);
    //   // Gets the previous Thursday if d is not already Thursday
    //   civil_day thurs2 = prev_weekday(d + 1, weekday::thursday);
    //
    using times_detail::next_weekday;
    using times_detail::prev_weekday;

    // Returns the day-of-year for the given civil-time value.
    //
    //   civil_day a(2015, 1, 1);
    //   int yd_jan_1 = get_yearday(a);   // yd_jan_1 = 1
    //   civil_day b(2015, 12, 31);
    //   int yd_dec_31 = get_yearday(b);  // yd_dec_31 = 365
    //
    using times_detail::get_yearday;

}  // flare::times_internal

#endif  // FLARE_TIMES_INTERNAL_CHRONO_TIME_INTERNAL_H_
