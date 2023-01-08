
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
// The implementation of the flare::duration class, which is declared in
// //flare/times/time.h.  This class behaves like a numeric type; it has no public
// methods and is used only through the operators defined here.
//
// Implementation notes:
//
// An flare::duration is represented as
//
//   _rep_hi : (int64_t)  Whole seconds
//   _rep_lo : (uint32_t) Fractions of a second
//
// The seconds value (_rep_hi) may be positive or negative as appropriate.
// The fractional seconds (_rep_lo) is always a positive offset from _rep_hi.
// The API for duration guarantees at least nanosecond resolution, which
// means _rep_lo could have a max value of 1B - 1 if it stored nanoseconds.
// However, to utilize more of the available 32 bits of space in _rep_lo,
// we instead store quarters of a nanosecond in _rep_lo resulting in a max
// value of 4B - 1.  This allows us to correctly handle calculations like
// 0.5 nanos + 0.5 nanos = 1 nano.  The following example shows the actual
// duration rep using quarters of a nanosecond.
//
//    2.5 sec = {_rep_hi=2,  _rep_lo=2000000000}  // lo = 4 * 500000000
//   -2.5 sec = {_rep_hi=-3, _rep_lo=2000000000}
//
// Infinite durations are represented as Durations with the _rep_lo field set
// to all 1s.
//
//   +infinite_duration:
//     _rep_hi : kint64max
//     _rep_lo : ~0U
//
//   -infinite_duration:
//     _rep_hi : kint64min
//     _rep_lo : ~0U
//
// Arithmetic overflows/underflows to +/- infinity and saturates.

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <limits>
#include <string>
#include "flare/times/duration.h"
#include "flare/times/time.h"

namespace flare {


    namespace {

        using times_internal::kTicksPerNanosecond;
        using times_internal::kTicksPerSecond;

// A functor that's similar to std::multiplies<T>, except this returns the max
// T value instead of overflowing. This is only defined for uint128.
        template<typename Ignored>
        struct safe_multiply {
            uint128 operator()(uint128 a, uint128 b) const {
                // b hi is always zero because it originated as an int64_t.
                assert(uint128_high64(b) == 0);
                // Fastpath to avoid the expensive overflow check with division.
                if (uint128_high64(a) == 0) {
                    return (((uint128_low64(a) | uint128_low64(b)) >> 32) == 0)
                           ? static_cast<uint128>(uint128_low64(a) * uint128_low64(b))
                           : a * b;
                }
                return b == 0 ? b : (a > kuint128max / b) ? kuint128max : a * b;
            }
        };


    }  // namespace


//
// Additive operators.
//

    int64_t duration::integer_div_duration(bool satq, const duration num, const duration den,
                                           duration *rem) {
        int64_t q = 0;
        if (num.i_div_fast_path(den, &q, rem)) {
            return q;
        }

        const bool num_neg = num < zero_duration();
        const bool den_neg = den < zero_duration();
        const bool quotient_neg = num_neg != den_neg;

        if (num.is_infinite_duration() || den == zero_duration()) {
            *rem = num_neg ? -infinite_duration() : infinite_duration();
            return quotient_neg ? times_internal::kint64min : times_internal::kint64max;
        }
        if (den.is_infinite_duration()) {
            *rem = num;
            return 0;
        }

        const uint128 a = num.make_uint128_ticks();
        const uint128 b = den.make_uint128_ticks();
        uint128 quotient128 = a / b;

        if (satq) {
            // Limits the quotient to the range of int64_t.
            if (quotient128 > uint128(static_cast<uint64_t>(times_internal::kint64max))) {
                quotient128 = quotient_neg ? uint128(static_cast<uint64_t>(times_internal::kint64min))
                                           : uint128(static_cast<uint64_t>(times_internal::kint64max));
            }
        }


        const uint128 remainder128 = a - quotient128 * b;
        *rem = make_duration_from_uint128(remainder128, num_neg);

        if (!quotient_neg || quotient128 == 0) {
            return uint128_low64(quotient128) & times_internal::kint64max;
        }
        // The quotient needs to be negated, but we need to carefully handle
        // quotient128s with the top bit on.
        return -static_cast<int64_t>(uint128_low64(quotient128 - 1) & times_internal::kint64max) - 1;
    }

    duration &duration::operator+=(duration rhs) {
        if (is_infinite_duration()) {
            return *this;
        }
        if (rhs.is_infinite_duration()) {
            return *this = rhs;
        }
        const int64_t orig_rep_hi = _rep_hi;
        _rep_hi =
                times_internal::decode_twos_comp(
                        times_internal::encode_twos_comp(_rep_hi) + times_internal::encode_twos_comp(rhs._rep_hi));
        if (_rep_lo >= kTicksPerSecond - rhs._rep_lo) {
            _rep_hi = times_internal::decode_twos_comp(times_internal::encode_twos_comp(_rep_hi) + 1);
            _rep_lo -= kTicksPerSecond;
        }
        _rep_lo += rhs._rep_lo;
        if (rhs._rep_hi < 0 ? _rep_hi > orig_rep_hi : _rep_hi < orig_rep_hi) {
            return *this = rhs._rep_hi < 0 ? -infinite_duration() : infinite_duration();
        }
        return *this;
    }

    duration &duration::operator-=(duration rhs) {
        if (is_infinite_duration()) {
            return *this;
        }
        if (rhs.is_infinite_duration()) {
            return *this = rhs._rep_hi >= 0 ? -infinite_duration() : infinite_duration();
        }
        const int64_t orig_rep_hi = _rep_hi;
        _rep_hi =
                times_internal::decode_twos_comp(
                        times_internal::encode_twos_comp(_rep_hi) - times_internal::encode_twos_comp(rhs._rep_hi));
        if (_rep_lo < rhs._rep_lo) {
            _rep_hi = times_internal::decode_twos_comp(times_internal::encode_twos_comp(_rep_hi) - 1);
            _rep_lo += kTicksPerSecond;
        }
        _rep_lo -= rhs._rep_lo;
        if (rhs._rep_hi < 0 ? _rep_hi < orig_rep_hi : _rep_hi > orig_rep_hi) {
            return *this = rhs._rep_hi >= 0 ? -infinite_duration() : infinite_duration();
        }
        return *this;
    }

    //
    // Multiplicative operators.
    //
    duration &duration::operator*=(int64_t r) {
        if (is_infinite_duration()) {
            const bool is_neg = (r < 0) != (_rep_hi < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = scale_fixed<safe_multiply>(r);
    }

    duration &duration::operator*=(double r) {
        if (is_infinite_duration() || !flare::base::is_finite(r)) {
            const bool is_neg = (std::signbit(r) != 0) != (_rep_hi < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = scale_double<std::multiplies>(r);
    }

    duration &duration::operator/=(int64_t r) {
        if (is_infinite_duration() || r == 0) {
            const bool is_neg = (r < 0) != (_rep_hi < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = scale_fixed<std::divides>(r);
    }

    duration &duration::operator/=(double r) {
        if (is_infinite_duration() || !times_internal::is_valid_divisor(r)) {
            const bool is_neg = (std::signbit(r) != 0) != (_rep_hi < 0);
            return *this = is_neg ? -infinite_duration() : infinite_duration();
        }
        return *this = scale_double<std::divides>(r);
    }

    duration &duration::operator%=(duration rhs) {
        duration::integer_div_duration(false, *this, rhs, this);
        return *this;
    }

    double duration::float_div_duration(duration den) const {
        // Arithmetic with infinity is sticky.
        if (is_infinite_duration() || den == zero_duration()) {
            return (*this < zero_duration()) == (den < zero_duration())
                   ? std::numeric_limits<double>::infinity()
                   : -std::numeric_limits<double>::infinity();
        }
        if (den.is_infinite_duration()) {
            return 0.0;
        }

        double a = static_cast<double>(_rep_hi) * kTicksPerSecond + _rep_lo;
        double b = static_cast<double>(den._rep_hi) * kTicksPerSecond + den._rep_lo;
        return a / b;
    }

    //
    // trunc/floor/ceil.
    //

    duration duration::trunc(duration unit) const {
        return *this - (*this % unit);
    }

    duration duration::floor(const duration unit) const {
        const duration td = this->trunc(unit);
        return td <= *this ? td : td - abs_duration(unit);
    }

    duration duration::ceil(const duration unit) const {
        const duration td = this->trunc(unit);
        return td >= *this ? td : td + abs_duration(unit);
    }

    //
    // Factory functions.
    //

    duration duration::from_timespec(timespec ts) {
        if (static_cast<uint64_t>(ts.tv_nsec) < 1000 * 1000 * 1000) {
            int64_t ticks = ts.tv_nsec * kTicksPerNanosecond;
            return make_duration(ts.tv_sec, ticks);
        }
        return seconds(ts.tv_sec) + nanoseconds(ts.tv_nsec);
    }

    duration duration::from_timeval(timeval tv) {
        if (static_cast<uint64_t>(tv.tv_usec) < 1000 * 1000) {
            int64_t ticks = tv.tv_usec * 1000 * times_internal::kTicksPerNanosecond;
            return make_duration(tv.tv_sec, ticks);
        }
        return seconds(tv.tv_sec) + microseconds(tv.tv_usec);
    }

    //
    // Conversion to other duration types.
    //

    int64_t duration::to_int64_nanoseconds() const {
        if (_rep_hi >= 0 &&
            _rep_hi >> 33 == 0) {
            return (_rep_hi * 1000 * 1000 * 1000) +
                   (_rep_lo / times_internal::kTicksPerNanosecond);
        }
        return *this / nanoseconds(1);
    }

    int64_t duration::to_int64_microseconds() const {
        if (_rep_hi >= 0 &&
            _rep_hi >> 43 == 0) {
            return (_rep_hi * 1000 * 1000) +
                   (_rep_lo / (times_internal::kTicksPerNanosecond * 1000));
        }
        return *this / microseconds(1);
    }

    int64_t duration::to_int64_milliseconds() const {
        if (_rep_hi >= 0 &&
            _rep_hi >> 53 == 0) {
            return (_rep_hi * 1000) +
                   (_rep_lo / (times_internal::kTicksPerNanosecond * 1000 * 1000));
        }
        return *this / milliseconds(1);
    }

    int64_t duration::to_int64_seconds() const {
        int64_t hi = _rep_hi;
        if (is_infinite_duration()) return hi;
        if (hi < 0 && _rep_lo != 0) ++hi;
        return hi;
    }

    int64_t duration::to_int64_minutes() const {
        int64_t hi = _rep_hi;
        if (is_infinite_duration()) {
            return hi;
        }
        if (hi < 0 && _rep_lo != 0) ++hi;
        return hi / 60;
    }

    int64_t duration::to_int64_hours() const {
        int64_t hi = _rep_hi;
        if (is_infinite_duration()) {
            return hi;
        }
        if (hi < 0 && _rep_lo != 0) {
            ++hi;
        }
        return hi / (60 * 60);
    }

    double duration::to_double_nanoseconds() const {
        return float_div_duration(nanoseconds(1));
    }

    double duration::to_double_microseconds() const {
        return float_div_duration(microseconds(1));
    }

    double duration::to_double_milliseconds() const {
        return float_div_duration(milliseconds(1));
    }

    double duration::to_double_seconds() const {
        return float_div_duration(seconds(1));
    }

    double duration::to_double_minutes() const {
        return float_div_duration(minutes(1));
    }

    double duration::to_double_hours() const {
        return float_div_duration(hours(1));
    }

    timespec duration::to_timespec() const {
        timespec ts;
        if (!is_infinite_duration()) {
            int64_t rep_hi = _rep_hi;
            uint32_t rep_lo = _rep_lo;
            if (rep_hi < 0) {
                // Tweak the fields so that unsigned division of rep_lo
                // maps to truncation (towards zero) for the timespec.
                rep_lo += kTicksPerNanosecond - 1;
                if (rep_lo >= kTicksPerSecond) {
                    rep_hi += 1;
                    rep_lo -= kTicksPerSecond;
                }
            }
            ts.tv_sec = rep_hi;
            if (ts.tv_sec == rep_hi) {  // no time_t narrowing
                ts.tv_nsec = rep_lo / kTicksPerNanosecond;
                return ts;
            }
        }
        if (*this >= zero_duration()) {
            ts.tv_sec = std::numeric_limits<time_t>::max();
            ts.tv_nsec = 1000 * 1000 * 1000 - 1;
        } else {
            ts.tv_sec = std::numeric_limits<time_t>::min();
            ts.tv_nsec = 0;
        }
        return ts;
    }

    timeval duration::to_timeval() const {
        timeval tv;
        timespec ts = to_timespec();
        if (ts.tv_sec < 0) {
            // Tweak the fields so that positive division of tv_nsec
            // maps to truncation (towards zero) for the timeval.
            ts.tv_nsec += 1000 - 1;
            if (ts.tv_nsec >= 1000 * 1000 * 1000) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000 * 1000 * 1000;
            }
        }
        tv.tv_sec = ts.tv_sec;
        if (tv.tv_sec != ts.tv_sec) {  // narrowing
            if (ts.tv_sec < 0) {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::min();
                tv.tv_usec = 0;
            } else {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::max();
                tv.tv_usec = 1000 * 1000 - 1;
            }
            return tv;
        }
        tv.tv_usec = static_cast<int>(ts.tv_nsec / 1000);  // suseconds_t
        return tv;
    }

    std::chrono::nanoseconds duration::to_chrono_nanoseconds() const {
        return to_chrono_duration<std::chrono::nanoseconds>();
    }

    std::chrono::microseconds duration::to_chrono_microseconds() const {
        return to_chrono_duration<std::chrono::microseconds>();
    }

    std::chrono::milliseconds duration::to_chrono_milliseconds() const {
        return to_chrono_duration<std::chrono::milliseconds>();
    }

    std::chrono::seconds duration::to_chrono_seconds() const {
        return to_chrono_duration<std::chrono::seconds>();
    }

    std::chrono::minutes duration::to_chrono_minutes() const {
        return to_chrono_duration<std::chrono::minutes>();
    }

    std::chrono::hours duration::to_chrono_hours() const {
        return to_chrono_duration<std::chrono::hours>();
    }

//
// To/From string formatting.
//

    namespace {

// Formats a positive 64-bit integer in the given field width.  Note that
// it is up to the caller of Format64() to ensure that there is sufficient
// space before ep to hold the conversion.
        char *Format64(char *ep, int width, int64_t v) {
            do {
                --width;
                *--ep = '0' + (v % 10);  // contiguous digits
            } while (v /= 10);
            while (--width >= 0) *--ep = '0';  // zero pad
            return ep;
        }

        // Helpers for format_duration() that format 'n' and append it to 'out'
        // followed by the given 'unit'.  If 'n' formats to "0", nothing is
        // appended (not even the unit).

        // A type that encapsulates how to display a value of a particular unit. For
        // values that are displayed with fractional parts, the precision indicates
        // where to round the value. The precision varies with the display unit because
        // a duration can hold only quarters of a nanosecond, so displaying information
        // beyond that is just noise.
        //
        // For example, a microsecond value of 42.00025xxxxx should not display beyond 5
        // fractional digits, because it is in the noise of what a duration can
        // represent.
        struct DisplayUnit {
            const char *abbr;
            int prec;
            double pow10;
        };
        const DisplayUnit kDisplayNano = {"ns", 2, 1e2};
        const DisplayUnit kDisplayMicro = {"us", 5, 1e5};
        const DisplayUnit kDisplayMilli = {"ms", 8, 1e8};
        const DisplayUnit kDisplaySec = {"s", 11, 1e11};
        const DisplayUnit kDisplayMin = {"m", -1, 0.0};   // prec ignored
        const DisplayUnit kDisplayHour = {"h", -1, 0.0};  // prec ignored

        void AppendNumberUnit(std::string *out, int64_t n, DisplayUnit unit) {
            char buf[sizeof("2562047788015216")];  // hours in max duration
            char *const ep = buf + sizeof(buf);
            char *bp = Format64(ep, 0, n);
            if (*bp != '0' || bp + 1 != ep) {
                out->append(bp, ep - bp);
                out->append(unit.abbr);
            }
        }

        // Note: unit.prec is limited to double's digits10 value (typically 15) so it
        // always fits in buf[].
        void AppendNumberUnit(std::string *out, double n, DisplayUnit unit) {
            const int buf_size = std::numeric_limits<double>::digits10;
            const int prec = std::min(buf_size, unit.prec);
            char buf[buf_size];  // also large enough to hold integer part
            char *ep = buf + sizeof(buf);
            double d = 0;
            int64_t frac_part = round(std::modf(n, &d) * unit.pow10);
            int64_t int_part = d;
            if (int_part != 0 || frac_part != 0) {
                char *bp = Format64(ep, 0, int_part);  // always < 1000
                out->append(bp, ep - bp);
                if (frac_part != 0) {
                    out->push_back('.');
                    bp = Format64(ep, prec, frac_part);
                    while (ep[-1] == '0') --ep;
                    out->append(bp, ep - bp);
                }
                out->append(unit.abbr);
            }
        }

    }  // namespace

    // From Go's doc at https://golang.org/pkg/time/#duration.String
    //   [format_duration] returns a string representing the duration in the
    //   form "72h3m0.5s". Leading zero units are omitted.  As a special
    //   case, durations less than one second format use a smaller unit
    //   (milli-, micro-, or nanoseconds) to ensure that the leading digit
    //   is non-zero.  The zero duration formats as 0, with no unit.
    std::string duration::format_duration() const {
        const duration min_duration = seconds(times_internal::kint64min);
        auto d = *this;
        if (d == min_duration) {
            // Avoid needing to negate kint64min by directly returning what the
            // following code should produce in that case.
            return "-2562047788015215h30m8s";
        }
        std::string s;
        if (d < zero_duration()) {
            s.append("-");
            d = -d;
        }
        if (d == infinite_duration()) {
            s.append("inf");
        } else if (d < seconds(1)) {
            // Special case for durations with a magnitude < 1 second.  The duration
            // is printed as a fraction of a single unit, e.g., "1.2ms".
            if (d < microseconds(1)) {
                AppendNumberUnit(&s, d.float_div_duration(nanoseconds(1)), kDisplayNano);
            } else if (d < milliseconds(1)) {
                AppendNumberUnit(&s, d.float_div_duration(microseconds(1)), kDisplayMicro);
            } else {
                AppendNumberUnit(&s, d.float_div_duration(milliseconds(1)), kDisplayMilli);
            }
        } else {
            AppendNumberUnit(&s, integer_div_duration(d, hours(1), &d), kDisplayHour);
            AppendNumberUnit(&s, integer_div_duration(d, minutes(1), &d), kDisplayMin);
            AppendNumberUnit(&s, d.float_div_duration(seconds(1)), kDisplaySec);
        }
        if (s.empty() || s == "-") {
            s = "0";
        }
        return s;
    }

    namespace {

        // A helper for parse_duration() that parses a leading number from the given
        // string and stores the result in *int_part/*frac_part/*frac_scale.  The
        // given string pointer is modified to point to the first unconsumed char.
        bool ConsumeDurationNumber(const char **dpp, int64_t *int_part,
                                   int64_t *frac_part, int64_t *frac_scale) {
            *int_part = 0;
            *frac_part = 0;
            *frac_scale = 1;  // invariant: *frac_part < *frac_scale
            const char *start = *dpp;
            for (; std::isdigit(**dpp); *dpp += 1) {
                const int d = **dpp - '0';  // contiguous digits
                if (*int_part > times_internal::kint64max / 10) return false;
                *int_part *= 10;
                if (*int_part > times_internal::kint64max - d) return false;
                *int_part += d;
            }
            const bool int_part_empty = (*dpp == start);
            if (**dpp != '.') return !int_part_empty;
            for (*dpp += 1; std::isdigit(**dpp); *dpp += 1) {
                const int d = **dpp - '0';  // contiguous digits
                if (*frac_scale <= times_internal::kint64max / 10) {
                    *frac_part *= 10;
                    *frac_part += d;
                    *frac_scale *= 10;
                }
            }
            return !int_part_empty || *frac_scale != 1;
        }

        // A helper for parse_duration() that parses a leading unit designator (e.g.,
        // ns, us, ms, s, m, h) from the given string and stores the resulting unit
        // in "*unit".  The given string pointer is modified to point to the first
        // unconsumed char.
        bool ConsumeDurationUnit(const char **start, duration *unit) {
            const char *s = *start;
            bool ok = true;
            if (strncmp(s, "ns", 2) == 0) {
                s += 2;
                *unit = duration::nanoseconds(1);
            } else if (strncmp(s, "us", 2) == 0) {
                s += 2;
                *unit = duration::microseconds(1);
            } else if (strncmp(s, "ms", 2) == 0) {
                s += 2;
                *unit = duration::milliseconds(1);
            } else if (strncmp(s, "s", 1) == 0) {
                s += 1;
                *unit = duration::seconds(1);
            } else if (strncmp(s, "m", 1) == 0) {
                s += 1;
                *unit = duration::minutes(1);
            } else if (strncmp(s, "h", 1) == 0) {
                s += 1;
                *unit = duration::hours(1);
            } else {
                ok = false;
            }
            *start = s;
            return ok;
        }

    }  // namespace

    // From Go's doc at https://golang.org/pkg/time/#parse_duration
    //   [parse_duration] parses a duration string. A duration string is
    //   a possibly signed sequence of decimal numbers, each with optional
    //   fraction and a unit suffix, such as "300ms", "-1.5h" or "2h45m".
    //   Valid time units are "ns", "us" "ms", "s", "m", "h".
    bool parse_duration(const std::string &dur_string, duration *d) {
        const char *start = dur_string.c_str();
        int sign = 1;

        if (*start == '-' || *start == '+') {
            sign = *start == '-' ? -1 : 1;
            ++start;
        }

        // Can't parse a duration from an empty std::string.
        if (*start == '\0') {
            return false;
        }

        // Special case for a std::string of "0".
        if (*start == '0' && *(start + 1) == '\0') {
            *d = zero_duration();
            return true;
        }

        if (strcmp(start, "inf") == 0) {
            *d = sign * infinite_duration();
            return true;
        }

        duration dur;
        while (*start != '\0') {
            int64_t int_part;
            int64_t frac_part;
            int64_t frac_scale;
            duration unit;
            if (!ConsumeDurationNumber(&start, &int_part, &frac_part, &frac_scale) ||
                !ConsumeDurationUnit(&start, &unit)) {
                return false;
            }
            if (int_part != 0) dur += sign * int_part * unit;
            if (frac_part != 0) dur += sign * frac_part * unit / frac_scale;
        }
        *d = dur;
        return true;
    }


}  // namespace flare
