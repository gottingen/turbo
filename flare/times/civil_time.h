
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
//
// -----------------------------------------------------------------------------
// File: civil_time.h
// -----------------------------------------------------------------------------
//
// This header file defines abstractions for computing with "civil time".
// The term "civil time" refers to the legally recognized human-scale time
// that is represented by the six fields `YYYY-MM-DD hh:mm:ss`. A "date"
// is perhaps the most common example of a civil time (represented here as
// an `flare::chrono_day`).
//
// Modern-day civil time follows the Gregorian Calendar and is a
// time-zone-independent concept: a civil time of "2015-06-01 12:00:00", for
// example, is not tied to a time zone. Put another way, a civil time does not
// map to a unique point in time; a civil time must be mapped to an absolute
// time *through* a time zone.
//
// Because a civil time is what most people think of as "time," it is common to
// map absolute times to civil times to present to users.
//
// time_point zones define the relationship between absolute and civil times. Given an
// absolute or civil time and a time zone, you can compute the other time:
//
//   Civil time_point = F(Absolute time_point, time_point Zone)
//   Absolute time_point = G(Civil time_point, time_point Zone)
//
// The flare time library allows you to construct such civil times from
// absolute times; consult time.h for such functionality.
//
// This library provides six classes for constructing civil-time objects, and
// provides several helper functions for rounding, iterating, and performing
// arithmetic on civil-time objects, while avoiding complications like
// daylight-saving time (DST):
//
//   * `flare::chrono_second`
//   * `flare::chrono_minute`
//   * `flare::chrono_hour`
//   * `flare::chrono_day`
//   * `flare::chrono_month`
//   * `flare::chrono_year`
//
// Example:
//
//   // Construct a civil-time object for a specific day
//   const flare::chrono_day cd(1969, 07, 20);
//
//   // Construct a civil-time object for a specific second
//   const flare::chrono_second cd(2018, 8, 1, 12, 0, 1);
//
// Note: In C++14 and later, this library is usable in a constexpr context.
//
// Example:
//
//   // Valid in C++14
//   constexpr flare::chrono_day cd(1969, 07, 20);

#ifndef FLARE_TIMES_CIVIL_TIME_H_
#define FLARE_TIMES_CIVIL_TIME_H_

#include <string>
#include <string_view>
#include "flare/times/internal/chrono_time_internal.h"
#include "flare/base/profile.h"

namespace flare {

    namespace times_internal {
        struct second_tag : flare::times_internal::times_detail::second_tag {
        };
        struct minute_tag : second_tag, flare::times_internal::times_detail::minute_tag {
        };
        struct hour_tag : minute_tag, flare::times_internal::times_detail::hour_tag {
        };
        struct day_tag : hour_tag, flare::times_internal::times_detail::day_tag {
        };
        struct month_tag : day_tag, flare::times_internal::times_detail::month_tag {
        };
        struct year_tag : month_tag, flare::times_internal::times_detail::year_tag {
        };
    }  // namespace times_internal

    // -----------------------------------------------------------------------------
    // chrono_second, chrono_minute, chrono_hour, chrono_day, chrono_month, chrono_year
    // -----------------------------------------------------------------------------
    //
    // Each of these civil-time types is a simple value type with the same
    // interface for construction and the same six accessors for each of the civil
    // time fields (year, month, day, hour, minute, and second, aka YMDHMS). These
    // classes differ only in their alignment, which is indicated by the type name
    // and specifies the field on which arithmetic operates.
    //
    // CONSTRUCTION
    //
    // Each of the civil-time types can be constructed in two ways: by directly
    // passing to the constructor up to six integers representing the YMDHMS fields,
    // or by copying the YMDHMS fields from a differently aligned civil-time type.
    // Omitted fields are assigned their minimum valid value. hours, minutes, and
    // seconds will be set to 0, month and day will be set to 1. Since there is no
    // minimum year, the default is 1970.
    //
    // Examples:
    //
    //   flare::chrono_day default_value;               // 1970-01-01 00:00:00
    //
    //   flare::chrono_day a(2015, 2, 3);               // 2015-02-03 00:00:00
    //   flare::chrono_day b(2015, 2, 3, 4, 5, 6);      // 2015-02-03 00:00:00
    //   flare::chrono_day c(2015);                     // 2015-01-01 00:00:00
    //
    //   flare::chrono_second ss(2015, 2, 3, 4, 5, 6);  // 2015-02-03 04:05:06
    //   flare::chrono_minute mm(ss);                   // 2015-02-03 04:05:00
    //   flare::chrono_hour hh(mm);                     // 2015-02-03 04:00:00
    //   flare::chrono_day d(hh);                       // 2015-02-03 00:00:00
    //   flare::chrono_month m(d);                      // 2015-02-01 00:00:00
    //   flare::chrono_year y(m);                       // 2015-01-01 00:00:00
    //
    //   m = flare::chrono_month(y);                    // 2015-01-01 00:00:00
    //   d = flare::chrono_day(m);                      // 2015-01-01 00:00:00
    //   hh = flare::chrono_hour(d);                    // 2015-01-01 00:00:00
    //   mm = flare::chrono_minute(hh);                 // 2015-01-01 00:00:00
    //   ss = flare::chrono_second(mm);                 // 2015-01-01 00:00:00
    //
    // Each civil-time class is aligned to the civil-time field indicated in the
    // class's name after normalization. Alignment is performed by setting all the
    // inferior fields to their minimum valid value (as described above). The
    // following are examples of how each of the six types would align the fields
    // representing November 22, 2015 at 12:34:56 in the afternoon. (Note: the
    // string format used here is not important; it's just a shorthand way of
    // showing the six YMDHMS fields.)
    //
    //   flare::chrono_second   : 2015-11-22 12:34:56
    //   flare::chrono_minute   : 2015-11-22 12:34:00
    //   flare::chrono_hour     : 2015-11-22 12:00:00
    //   flare::chrono_day      : 2015-11-22 00:00:00
    //   flare::chrono_month    : 2015-11-01 00:00:00
    //   flare::chrono_year     : 2015-01-01 00:00:00
    //
    // Each civil-time type performs arithmetic on the field to which it is
    // aligned. This means that adding 1 to an flare::chrono_day increments the day
    // field (normalizing as necessary), and subtracting 7 from an flare::chrono_month
    // operates on the month field (normalizing as necessary). All arithmetic
    // produces a valid civil time. Difference requires two similarly aligned
    // civil-time objects and returns the scalar answer in units of the objects'
    // alignment. For example, the difference between two flare::chrono_hour objects
    // will give an answer in units of civil hours.
    //
    // ALIGNMENT CONVERSION
    //
    // The alignment of a civil-time object cannot change, but the object may be
    // used to construct a new object with a different alignment. This is referred
    // to as "realigning". When realigning to a type with the same or more
    // precision (e.g., flare::chrono_day -> flare::chrono_second), the conversion may be
    // performed implicitly since no information is lost. However, if information
    // could be discarded (e.g., chrono_second -> chrono_day), the conversion must
    // be explicit at the call site.
    //
    // Examples:
    //
    //   void UseDay(flare::chrono_day day);
    //
    //   flare::chrono_second cs;
    //   UseDay(cs);                  // Won't compile because data may be discarded
    //   UseDay(flare::chrono_day(cs));  // OK: explicit conversion
    //
    //   flare::chrono_day cd;
    //   UseDay(cd);                  // OK: no conversion needed
    //
    //   flare::chrono_month cm;
    //   UseDay(cm);                  // OK: implicit conversion to flare::chrono_day
    //
    // NORMALIZATION
    //
    // Normalization takes invalid values and adjusts them to produce valid values.
    // Within the civil-time library, integer arguments passed to the Civil*
    // constructors may be out-of-range, in which case they are normalized by
    // carrying overflow into a field of courser granularity to produce valid
    // civil-time objects. This normalization enables natural arithmetic on
    // constructor arguments without worrying about the field's range.
    //
    // Examples:
    //
    //   // Out-of-range; normalized to 2016-11-01
    //   flare::chrono_day d(2016, 10, 32);
    //   // Out-of-range, negative: normalized to 2016-10-30T23
    //   flare::chrono_hour h1(2016, 10, 31, -1);
    //   // Normalization is cumulative: normalized to 2016-10-30T23
    //   flare::chrono_hour h2(2016, 10, 32, -25);
    //
    // Note: If normalization is undesired, you can signal an error by comparing
    // the constructor arguments to the normalized values returned by the YMDHMS
    // properties.
    //
    // COMPARISON
    //
    // Comparison between civil-time objects considers all six YMDHMS fields,
    // regardless of the type's alignment. Comparison between differently aligned
    // civil-time types is allowed.
    //
    // Examples:
    //
    //   flare::chrono_day feb_3(2015, 2, 3);  // 2015-02-03 00:00:00
    //   flare::chrono_day mar_4(2015, 3, 4);  // 2015-03-04 00:00:00
    //   // feb_3 < mar_4
    //   // flare::chrono_year(feb_3) == flare::chrono_year(mar_4)
    //
    //   flare::chrono_second feb_3_noon(2015, 2, 3, 12, 0, 0);  // 2015-02-03 12:00:00
    //   // feb_3 < feb_3_noon
    //   // feb_3 == flare::chrono_day(feb_3_noon)
    //
    //   // Iterates all the days of February 2015.
    //   for (flare::chrono_day d(2015, 2, 1); d < flare::chrono_month(2015, 3); ++d) {
    //     // ...
    //   }
    //
    // ARITHMETIC
    //
    // Civil-time types support natural arithmetic operators such as addition,
    // subtraction, and difference. Arithmetic operates on the civil-time field
    // indicated in the type's name. Difference operators require arguments with
    // the same alignment and return the answer in units of the alignment.
    //
    // Example:
    //
    //   flare::chrono_day a(2015, 2, 3);
    //   ++a;                              // 2015-02-04 00:00:00
    //   --a;                              // 2015-02-03 00:00:00
    //   flare::chrono_day b = a + 1;         // 2015-02-04 00:00:00
    //   flare::chrono_day c = 1 + b;         // 2015-02-05 00:00:00
    //   int n = c - a;                    // n = 2 (civil days)
    //   int m = c - flare::chrono_month(c);  // Won't compile: different types.
    //
    // ACCESSORS
    //
    // Each civil-time type has accessors for all six of the civil-time fields:
    // year, month, day, hour, minute, and second.
    //
    // chrono_year_t year()
    // int          month()
    // int          day()
    // int          hour()
    // int          minute()
    // int          second()
    //
    // Recall that fields inferior to the type's alignment will be set to their
    // minimum valid value.
    //
    // Example:
    //
    //   flare::chrono_day d(2015, 6, 28);
    //   // d.year() == 2015
    //   // d.month() == 6
    //   // d.day() == 28
    //   // d.hour() == 0
    //   // d.minute() == 0
    //   // d.second() == 0
    //
    // CASE STUDY: Adding a month to January 31.
    //
    // One of the classic questions that arises when considering a civil time
    // library (or a date library or a date/time library) is this:
    //   "What is the result of adding a month to January 31?"
    // This is an interesting question because it is unclear what is meant by a
    // "month", and several different answers are possible, depending on context:
    //
    //   1. March 3 (or 2 if a leap year), if "add a month" means to add a month to
    //      the current month, and adjust the date to overflow the extra days into
    //      March. In this case the result of "February 31" would be normalized as
    //      within the civil-time library.
    //   2. February 28 (or 29 if a leap year), if "add a month" means to add a
    //      month, and adjust the date while holding the resulting month constant.
    //      In this case, the result of "February 31" would be truncated to the last
    //      day in February.
    //   3. An error. The caller may get some error, an exception, an invalid date
    //      object, or perhaps return `false`. This may make sense because there is
    //      no single unambiguously correct answer to the question.
    //
    // Practically speaking, any answer that is not what the programmer intended
    // is the wrong answer.
    //
    // The flare time library avoids this problem by making it impossible to
    // ask ambiguous questions. All civil-time objects are aligned to a particular
    // civil-field boundary (such as aligned to a year, month, day, hour, minute,
    // or second), and arithmetic operates on the field to which the object is
    // aligned. This means that in order to "add a month" the object must first be
    // aligned to a month boundary, which is equivalent to the first day of that
    // month.
    //
    // Of course, there are ways to compute an answer the question at hand using
    // this flare time library, but they require the programmer to be explicit
    // about the answer they expect. To illustrate, let's see how to compute all
    // three of the above possible answers to the question of "Jan 31 plus 1
    // month":
    //
    // Example:
    //
    //   const flare::chrono_day d(2015, 1, 31);
    //
    //   // Answer 1:
    //   // Add 1 to the month field in the constructor, and rely on normalization.
    //   const auto normalized = flare::chrono_day(d.year(), d.month() + 1, d.day());
    //   // normalized == 2015-03-03 (aka Feb 31)
    //
    //   // Answer 2:
    //   // Add 1 to month field, capping to the end of next month.
    //   const auto next_month = flare::chrono_month(d) + 1;
    //   const auto last_day_of_next_month = flare::chrono_day(next_month + 1) - 1;
    //   const auto capped = std::min(normalized, last_day_of_next_month);
    //   // capped == 2015-02-28
    //
    //   // Answer 3:
    //   // signal an error if the normalized answer is not in next month.
    //   if (flare::chrono_month(normalized) != next_month) {
    //     // error, month overflow
    //   }
    //
    using chrono_second =
    flare::times_internal::times_detail::civil_time<times_internal::second_tag>;
    using chrono_minute =
    flare::times_internal::times_detail::civil_time<times_internal::minute_tag>;
    using chrono_hour =
    flare::times_internal::times_detail::civil_time<times_internal::hour_tag>;
    using chrono_day =
    flare::times_internal::times_detail::civil_time<times_internal::day_tag>;
    using chrono_month =
    flare::times_internal::times_detail::civil_time<times_internal::month_tag>;
    using chrono_year =
    flare::times_internal::times_detail::civil_time<times_internal::year_tag>;

    // chrono_year_t
    //
    // Type alias of a civil-time year value. This type is guaranteed to (at least)
    // support any year value supported by `time_t`.
    //
    // Example:
    //
    //   flare::chrono_second cs = ...;
    //   flare::chrono_year_t y = cs.year();
    //   cs = flare::chrono_second(y, 1, 1, 0, 0, 0);  // chrono_second(chrono_year(cs))
    //
    using chrono_year_t = flare::times_internal::year_t;

    // chrono_diff_t
    //
    // Type alias of the difference between two civil-time values.
    // This type is used to indicate arguments that are not
    // normalized (such as parameters to the civil-time constructors), the results
    // of civil-time subtraction, or the operand to civil-time addition.
    //
    // Example:
    //
    //   flare::chrono_diff_t n_sec = cs1 - cs2;             // cs1 == cs2 + n_sec;
    //
    using chrono_diff_t = flare::times_internal::diff_t;

    // chrono_weekday::monday, chrono_weekday::tuesday, chrono_weekday::wednesday, chrono_weekday::thursday,
    // chrono_weekday::friday, chrono_weekday::saturday, chrono_weekday::sunday
    //
    // The chrono_weekday enum class represents the civil-time concept of a "weekday" with
    // members for all days of the week.
    //
    //   flare::chrono_weekday wd = flare::chrono_weekday::thursday;
    //::
    using chrono_weekday = flare::times_internal::times_detail::weekday;

    // get_weekday()
    //
    // Returns the flare::chrono_weekday for the given (realigned) civil-time value.
    //
    // Example:
    //
    //   flare::chrono_day a(2015, 8, 13);
    //   flare::chrono_weekday wd = flare::get_weekday(a);  // wd == flare::chrono_weekday::thursday
    //
    FLARE_FORCE_INLINE chrono_weekday get_weekday(chrono_second cs) {
        return flare::times_internal::times_detail::get_weekday(cs);
    }

    // next_weekday()
    // prev_weekday()
    //
    // Returns the flare::chrono_day that strictly follows or precedes a given
    // flare::chrono_day, and that falls on the given flare::chrono_weekday.
    //
    // Example, given the following month:
    //
    //       August 2015
    //   Su Mo Tu We Th Fr Sa
    //                      1
    //    2  3  4  5  6  7  8
    //    9 10 11 12 13 14 15
    //   16 17 18 19 20 21 22
    //   23 24 25 26 27 28 29
    //   30 31
    //
    //   flare::chrono_day a(2015, 8, 13);
    //   // flare::get_weekday(a) == flare::chrono_weekday::thursday
    //   flare::chrono_day b = flare::next_weekday(a, flare::chrono_weekday::thursday);
    //   // b = 2015-08-20
    //   flare::chrono_day c = flare::prev_weekday(a, flare::chrono_weekday::thursday);
    //   // c = 2015-08-06
    //
    //   flare::chrono_day d = ...
    //   // Gets the following Thursday if d is not already Thursday
    //   flare::chrono_day thurs1 = flare::next_weekday(d - 1, flare::chrono_weekday::thursday);
    //   // Gets the previous Thursday if d is not already Thursday
    //   flare::chrono_day thurs2 = flare::prev_weekday(d + 1, flare::chrono_weekday::thursday);
    //
    FLARE_FORCE_INLINE chrono_day next_weekday(chrono_day cd, chrono_weekday wd) {
        return chrono_day(flare::times_internal::times_detail::next_weekday(cd, wd));
    }

    FLARE_FORCE_INLINE chrono_day prev_weekday(chrono_day cd, chrono_weekday wd) {
        return chrono_day(flare::times_internal::times_detail::prev_weekday(cd, wd));
    }

    // get_yearday()
    //
    // Returns the day-of-year for the given (realigned) civil-time value.
    //
    // Example:
    //
    //   flare::chrono_day a(2015, 1, 1);
    //   int yd_jan_1 = flare::get_yearday(a);   // yd_jan_1 = 1
    //   flare::chrono_day b(2015, 12, 31);
    //   int yd_dec_31 = flare::get_yearday(b);  // yd_dec_31 = 365
    //
    FLARE_FORCE_INLINE int get_yearday(chrono_second cs) {
        return flare::times_internal::times_detail::get_yearday(cs);
    }

    // format_chrono_time()
    //
    // Formats the given civil-time value into a string value of the following
    // format:
    //
    //  Type        | Format
    //  ---------------------------------
    //  chrono_second | YYYY-MM-DDTHH:MM:SS
    //  chrono_minute | YYYY-MM-DDTHH:MM
    //  chrono_hour   | YYYY-MM-DDTHH
    //  chrono_day    | YYYY-MM-DD
    //  chrono_month  | YYYY-MM
    //  chrono_year   | YYYY
    //
    // Example:
    //
    //   flare::chrono_day d = flare::chrono_day(1969, 7, 20);
    //   std::string day_string = flare::format_chrono_time(d);  // "1969-07-20"
    //
    std::string format_chrono_time(chrono_second c);

    std::string format_chrono_time(chrono_minute c);

    std::string format_chrono_time(chrono_hour c);

    std::string format_chrono_time(chrono_day c);

    std::string format_chrono_time(chrono_month c);

    std::string format_chrono_time(chrono_year c);

    // flare::parse_chrono_time()
    //
    // Parses a civil-time value from the specified `std::string_view` into the
    // passed output parameter. Returns `true` upon successful parsing.
    //
    // The expected form of the input string is as follows:
    //
    //  Type        | Format
    //  ---------------------------------
    //  chrono_second | YYYY-MM-DDTHH:MM:SS
    //  chrono_minute | YYYY-MM-DDTHH:MM
    //  chrono_hour   | YYYY-MM-DDTHH
    //  chrono_day    | YYYY-MM-DD
    //  chrono_month  | YYYY-MM
    //  chrono_year   | YYYY
    //
    // Example:
    //
    //   flare::chrono_day d;
    //   bool ok = flare::parse_chrono_time("2018-01-02", &d); // OK
    //
    // Note that parsing will fail if the string's format does not match the
    // expected type exactly. `ParseLenientCivilTime()` below is more lenient.
    //
    bool parse_chrono_time(std::string_view s, chrono_second *c);

    bool parse_chrono_time(std::string_view s, chrono_minute *c);

    bool parse_chrono_time(std::string_view s, chrono_hour *c);

    bool parse_chrono_time(std::string_view s, chrono_day *c);

    bool parse_chrono_time(std::string_view s, chrono_month *c);

    bool parse_chrono_time(std::string_view s, chrono_year *c);

    // ParseLenientCivilTime()
    //
    // Parses any of the formats accepted by `flare::parse_chrono_time()`, but is more
    // lenient if the format of the string does not exactly match the associated
    // type.
    //
    // Example:
    //
    //   flare::chrono_day d;
    //   bool ok = flare::ParseLenientCivilTime("1969-07-20", &d); // OK
    //   ok = flare::ParseLenientCivilTime("1969-07-20T10", &d);   // OK: T10 floored
    //   ok = flare::ParseLenientCivilTime("1969-07", &d);   // OK: day defaults to 1
    //
    bool ParseLenientCivilTime(std::string_view s, chrono_second *c);

    bool ParseLenientCivilTime(std::string_view s, chrono_minute *c);

    bool ParseLenientCivilTime(std::string_view s, chrono_hour *c);

    bool ParseLenientCivilTime(std::string_view s, chrono_day *c);

    bool ParseLenientCivilTime(std::string_view s, chrono_month *c);

    bool ParseLenientCivilTime(std::string_view s, chrono_year *c);

    namespace times_internal {  // For functions found via ADL on civil-time tags.

        // Streaming Operators
        //
        // Each civil-time type may be sent to an output stream using operator<<().
        // The result matches the string produced by `format_chrono_time()`.
        //
        // Example:
        //
        //   flare::chrono_day d = flare::chrono_day(1969, 7, 20);
        //   std::cout << "Date is: " << d << "\n";
        //
        std::ostream &operator<<(std::ostream &os, chrono_year y);

        std::ostream &operator<<(std::ostream &os, chrono_month m);

        std::ostream &operator<<(std::ostream &os, chrono_day d);

        std::ostream &operator<<(std::ostream &os, chrono_hour h);

        std::ostream &operator<<(std::ostream &os, chrono_minute m);

        std::ostream &operator<<(std::ostream &os, chrono_second s);

    }  // namespace times_internal
}  // namespace flare

#endif  // FLARE_TIMES_CIVIL_TIME_H_
