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

// The implementation of the turbo::Time class, which is declared in
// //turbo/time.h.
//
// The representation for an turbo::Time is an turbo::Duration offset from the
// epoch.  We use the traditional Unix epoch (1970-01-01 00:00:00 +0000)
// for convenience, but this is not exposed in the API and could be changed.
//
// NOTE: To keep type verbosity to a minimum, the following variable naming
// conventions are used throughout this file.
//
// tz: An turbo::TimeZone
// ci: An turbo::TimeZone::CivilInfo
// ti: An turbo::TimeZone::TimeInfo
// cd: An turbo::CivilDay or a cctz::civil_day
// cs: An turbo::CivilSecond or a cctz::civil_second
// bd: An turbo::Time::Breakdown
// cl: A cctz::time_zone::civil_lookup
// al: A cctz::time_zone::absolute_lookup

#include <turbo/times/time.h>

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <cstring>
#include <ctime>
#include <limits>

#include <turbo/times/cctz/civil_time.h>
#include <turbo/times/cctz/time_zone.h>

namespace cctz = turbo::time_internal::cctz;

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    namespace {

        inline cctz::time_point <cctz::seconds> unix_epoch_cctz() {
            return std::chrono::time_point_cast<cctz::seconds>(
                    std::chrono::system_clock::from_time_t(0));
        }

        // Floors d to the next unit boundary closer to negative infinity.
        inline int64_t FloorToUnit(turbo::Duration d, turbo::Duration unit) {
            turbo::Duration rem;
            int64_t q = turbo::Duration::idiv(d, unit, &rem);
            return (q > 0 || rem >= Duration::zero() ||
                    q == std::numeric_limits<int64_t>::min())
                   ? q
                   : q - 1;
        }

        inline turbo::TimeZone::CivilInfo InfiniteFutureCivilInfo() {
            TimeZone::CivilInfo ci;
            ci.cs = CivilSecond::max();
            ci.subsecond = Duration::max_infinite();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        inline turbo::TimeZone::CivilInfo InfinitePastCivilInfo() {
            TimeZone::CivilInfo ci;
            ci.cs = CivilSecond::min();
            ci.subsecond = Duration::min_infinite();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        // Makes a Time from sec, overflowing to InfiniteFuture/Time::past_infinite as
        // necessary. If sec is min/max, then consult cs+tz to check for overflow.
        Time MakeTimeWithOverflow(const cctz::time_point <cctz::seconds> &sec,
                                  const cctz::civil_second &cs,
                                  const cctz::time_zone &tz,
                                  bool *normalized = nullptr) {
            const auto max = cctz::time_point<cctz::seconds>::max();
            const auto min = cctz::time_point<cctz::seconds>::min();
            if (sec == max) {
                const auto al = tz.lookup(max);
                if (cs > al.cs) {
                    if (normalized) *normalized = true;
                    return turbo::Time::future_infinite();
                }
            }
            if (sec == min) {
                const auto al = tz.lookup(min);
                if (cs < al.cs) {
                    if (normalized) *normalized = true;
                    return turbo::Time::past_infinite();
                }
            }
            const auto hi = (sec - unix_epoch_cctz()).count();
            return time_internal::FromUnixDuration(time_internal::MakeDuration(hi));
        }

// Returns Mon=1..Sun=7.
        inline int MapWeekday(const cctz::weekday &wd) {
            switch (wd) {
                case cctz::weekday::monday:
                    return 1;
                case cctz::weekday::tuesday:
                    return 2;
                case cctz::weekday::wednesday:
                    return 3;
                case cctz::weekday::thursday:
                    return 4;
                case cctz::weekday::friday:
                    return 5;
                case cctz::weekday::saturday:
                    return 6;
                case cctz::weekday::sunday:
                    return 7;
            }
            return 1;
        }

        bool FindTransition(const cctz::time_zone &tz,
                            bool (cctz::time_zone::*find_transition)(
                                    const cctz::time_point <cctz::seconds> &tp,
                                    cctz::time_zone::civil_transition *trans) const,
                            Time t, TimeZone::CivilTransition *trans) {
            // Transitions are second-aligned, so we can discard any fractional part.
            const auto tp = unix_epoch_cctz() + cctz::seconds(Time::to_seconds(t));
            cctz::time_zone::civil_transition tr;
            if (!(tz.*find_transition)(tp, &tr)) return false;
            trans->from = CivilSecond(tr.from);
            trans->to = CivilSecond(tr.to);
            return true;
        }

    }  // namespace

    //
    // Time
    //

    //
    // Conversions from/to other time types.
    //

    turbo::Time Time::from_udate(double udate) {
        return time_internal::FromUnixDuration(turbo::Duration::milliseconds(udate));
    }

    turbo::Time Time::from_universal(int64_t universal) {
        return turbo::Time::from_universal_epoch() + 100 * turbo::Duration::nanoseconds(universal);
    }

    int64_t Time::to_nanoseconds(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 33 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) *
                    1000 * 1000 * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4);
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::Duration::nanoseconds(1));
    }

    int64_t Time::to_microseconds(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 43 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) *
                    1000 * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4000);
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::Duration::microseconds(1));
    }

    int64_t Time::to_milliseconds(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 53 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) /
                    (4000 * 1000));
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::Duration::milliseconds(1));
    }

    int64_t Time::to_seconds(Time t) {
        return time_internal::GetRepHi(time_internal::ToUnixDuration(t));
    }

    time_t Time::to_time_t(Time t) { return turbo::Time::to_timespec(t).tv_sec; }

    double Time::to_udate(Time t) {
        return turbo::Duration::fdiv(time_internal::ToUnixDuration(t),
                                   turbo::Duration::milliseconds(1));
    }

    int64_t Time::to_universal(turbo::Time t) {
        return turbo::FloorToUnit(t - turbo::Time::from_universal_epoch(), turbo::Duration::nanoseconds(100));
    }

    turbo::Time Time::from_timespec(timespec ts) {
        return time_internal::FromUnixDuration(turbo::Duration::from_timespec(ts));
    }

    turbo::Time Time::from_timeval(timeval tv) {
        return time_internal::FromUnixDuration(turbo::Duration::from_timeval(tv));
    }

    timespec Time::to_timespec(Time t) {
        timespec ts;
        turbo::Duration d = time_internal::ToUnixDuration(t);
        if (!time_internal::IsInfiniteDuration(d)) {
            ts.tv_sec = static_cast<decltype(ts.tv_sec)>(time_internal::GetRepHi(d));
            if (ts.tv_sec == time_internal::GetRepHi(d)) {  // no time_t narrowing
                ts.tv_nsec = time_internal::GetRepLo(d) / 4;  // floor
                return ts;
            }
        }
        if (d >= turbo::Duration::zero()) {
            ts.tv_sec = std::numeric_limits<time_t>::max();
            ts.tv_nsec = 1000 * 1000 * 1000 - 1;
        } else {
            ts.tv_sec = std::numeric_limits<time_t>::min();
            ts.tv_nsec = 0;
        }
        return ts;
    }

    timeval Time::to_timeval(Time t) {
        timeval tv;
        timespec ts = turbo::Time::to_timespec(t);
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(ts.tv_sec);
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

    Time Time::from_chrono(const std::chrono::system_clock::time_point &tp) {
        return time_internal::FromUnixDuration(time_internal::FromChrono(
                tp - std::chrono::system_clock::from_time_t(0)));
    }

    std::chrono::system_clock::time_point Time::to_chrono(turbo::Time t) {
        using D = std::chrono::system_clock::duration;
        auto d = time_internal::ToUnixDuration(t);
        if (d < Duration::zero()) d = Duration::floor(d, Duration::from_chrono(D{1}));
        return std::chrono::system_clock::from_time_t(0) +
               time_internal::ToChronoDuration<D>(d);
    }

    //
    // TimeZone
    //

    turbo::TimeZone::CivilInfo TimeZone::at(Time t) const {
        if (t == turbo::Time::future_infinite()) return InfiniteFutureCivilInfo();
        if (t == turbo::Time::past_infinite()) return InfinitePastCivilInfo();

        const auto ud = time_internal::ToUnixDuration(t);
        const auto tp = unix_epoch_cctz() + cctz::seconds(time_internal::GetRepHi(ud));
        const auto al = cz_.lookup(tp);

        TimeZone::CivilInfo ci;
        ci.cs = CivilSecond(al.cs);
        ci.subsecond = time_internal::MakeDuration(0, time_internal::GetRepLo(ud));
        ci.offset = al.offset;
        ci.is_dst = al.is_dst;
        ci.zone_abbr = al.abbr;
        return ci;
    }

    turbo::TimeZone::TimeInfo TimeZone::at(CivilSecond ct) const {
        const cctz::civil_second cs(ct);
        const auto cl = cz_.lookup(cs);

        TimeZone::TimeInfo ti;
        switch (cl.kind) {
            case cctz::time_zone::civil_lookup::UNIQUE:
                ti.kind = TimeZone::TimeInfo::UNIQUE;
                break;
            case cctz::time_zone::civil_lookup::SKIPPED:
                ti.kind = TimeZone::TimeInfo::SKIPPED;
                break;
            case cctz::time_zone::civil_lookup::REPEATED:
                ti.kind = TimeZone::TimeInfo::REPEATED;
                break;
        }
        ti.pre = MakeTimeWithOverflow(cl.pre, cs, cz_);
        ti.trans = MakeTimeWithOverflow(cl.trans, cs, cz_);
        ti.post = MakeTimeWithOverflow(cl.post, cs, cz_);
        return ti;
    }

    bool TimeZone::NextTransition(Time t, CivilTransition *trans) const {
        return FindTransition(cz_, &cctz::time_zone::next_transition, t, trans);
    }

    bool TimeZone::PrevTransition(Time t, CivilTransition *trans) const {
        return FindTransition(cz_, &cctz::time_zone::prev_transition, t, trans);
    }


    turbo::Time Time::from_tm(const struct tm &tm, turbo::TimeZone tz) {
        civil_year_t tm_year = tm.tm_year;
        // Avoids years that are too extreme for CivilSecond to normalize.
        if (tm_year > 300000000000ll) return Time::future_infinite();
        if (tm_year < -300000000000ll) return Time::past_infinite();
        int tm_mon = tm.tm_mon;
        if (tm_mon == std::numeric_limits<int>::max()) {
            tm_mon -= 12;
            tm_year += 1;
        }
        const auto ti = tz.at(CivilSecond(tm_year + 1900, tm_mon + 1, tm.tm_mday,
                                          tm.tm_hour, tm.tm_min, tm.tm_sec));
        return tm.tm_isdst == 0 ? ti.post : ti.pre;
    }

    struct tm Time::to_tm(turbo::Time t, turbo::TimeZone tz) {
        struct tm tm = {};

        const auto ci = tz.at(t);
        const auto &cs = ci.cs;
        tm.tm_sec = cs.second();
        tm.tm_min = cs.minute();
        tm.tm_hour = cs.hour();
        tm.tm_mday = cs.day();
        tm.tm_mon = cs.month() - 1;

        // Saturates tm.tm_year in cases of over/underflow, accounting for the fact
        // that tm.tm_year is years since 1900.
        if (cs.year() < std::numeric_limits<int>::min() + 1900) {
            tm.tm_year = std::numeric_limits<int>::min();
        } else if (cs.year() > std::numeric_limits<int>::max()) {
            tm.tm_year = std::numeric_limits<int>::max() - 1900;
        } else {
            tm.tm_year = static_cast<int>(cs.year() - 1900);
        }

        switch (GetWeekday(cs)) {
            case Weekday::sunday:
                tm.tm_wday = 0;
                break;
            case Weekday::monday:
                tm.tm_wday = 1;
                break;
            case Weekday::tuesday:
                tm.tm_wday = 2;
                break;
            case Weekday::wednesday:
                tm.tm_wday = 3;
                break;
            case Weekday::thursday:
                tm.tm_wday = 4;
                break;
            case Weekday::friday:
                tm.tm_wday = 5;
                break;
            case Weekday::saturday:
                tm.tm_wday = 6;
                break;
        }
        tm.tm_yday = GetYearDay(cs) - 1;
        tm.tm_isdst = ci.is_dst ? 1 : 0;

        return tm;
    }

    TURBO_NAMESPACE_END
}  // namespace turbo
