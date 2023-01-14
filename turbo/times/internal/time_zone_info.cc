
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/times/internal/time_zone_info.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "turbo/base/profile.h"
#include "turbo/times/internal/chrono_time_internal.h"
#include "turbo/times/internal/time_zone_fixed.h"
#include "turbo/times/internal/time_zone_posix.h"

namespace turbo::times_internal {

    namespace {

        TURBO_FORCE_INLINE bool is_leap(year_t year) {
            return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
        }

// The number of days in non-leap and leap years respectively.
        const std::int_least32_t kDaysPerYear[2] = {365, 366};

// The day offsets of the beginning of each (1-based) month in non-leap and
// leap years respectively (e.g., 335 days before December in a leap year).
        const std::int_least16_t kMonthOffsets[2][1 + 12 + 1] = {
                {-1, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
                {-1, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},
        };

// We reject leap-second encoded zoneinfo and so assume 60-second minutes.
        const std::int_least32_t kSecsPerDay = 24 * 60 * 60;

// 400-year chunks always have 146097 days (20871 weeks).
        const std::int_least64_t kSecsPer400Years = 146097LL * kSecsPerDay;

// Like kDaysPerYear[] but scaled up by a factor of kSecsPerDay.
        const std::int_least32_t kSecsPerYear[2] = {
                365 * kSecsPerDay,
                366 * kSecsPerDay,
        };

// Single-byte, unsigned numeric values are encoded directly.
        TURBO_FORCE_INLINE std::uint_fast8_t decode8(const char *cp) {
            return static_cast<std::uint_fast8_t>(*cp) & 0xff;
        }

// Multi-byte, numeric values are encoded using a MSB first,
// twos-complement representation. These helpers decode, from
// the given address, 4-byte and 8-byte values respectively.
// Note: If int_fastXX_t == intXX_t and this machine is not
// twos complement, then there will be at least one input value
// we cannot represent.
        std::int_fast32_t decode32(const char *cp) {
            std::uint_fast32_t v = 0;
            for (int i = 0; i != (32 / 8); ++i)
                v = (v << 8) | decode8(cp++);
            const std::int_fast32_t s32max = 0x7fffffff;
            const auto s32maxU = static_cast<std::uint_fast32_t>(s32max);
            if (v <= s32maxU)
                return static_cast<std::int_fast32_t>(v);
            return static_cast<std::int_fast32_t>(v - s32maxU - 1) - s32max - 1;
        }

        std::int_fast64_t decode64(const char *cp) {
            std::uint_fast64_t v = 0;
            for (int i = 0; i != (64 / 8); ++i)
                v = (v << 8) | decode8(cp++);
            const std::int_fast64_t s64max = 0x7fffffffffffffff;
            const auto s64maxU = static_cast<std::uint_fast64_t>(s64max);
            if (v <= s64maxU)
                return static_cast<std::int_fast64_t>(v);
            return static_cast<std::int_fast64_t>(v - s64maxU - 1) - s64max - 1;
        }

// Generate a year-relative offset for a posix_transition.
        std::int_fast64_t trans_offset(bool leap_year, int jan1_weekday,
                                       const posix_transition &pt) {
            std::int_fast64_t days = 0;
            switch (pt.date.fmt) {
                case posix_transition::J: {
                    days = pt.date.j.day;
                    if (!leap_year || days < kMonthOffsets[1][3])
                        days -= 1;
                    break;
                }
                case posix_transition::N: {
                    days = pt.date.n.day;
                    break;
                }
                case posix_transition::M: {
                    const bool last_week = (pt.date.m.week == 5);
                    days = kMonthOffsets[leap_year][pt.date.m.month + last_week];
                    const std::int_fast64_t weekday = (jan1_weekday + days) % 7;
                    if (last_week) {
                        days -= (weekday + 7 - 1 - pt.date.m.weekday) % 7 + 1;
                    } else {
                        days += (pt.date.m.weekday + 7 - weekday) % 7;
                        days += (pt.date.m.week - 1) * 7;
                    }
                    break;
                }
            }
            return (days * kSecsPerDay) + pt.time.offset;
        }

        TURBO_FORCE_INLINE time_zone::civil_lookup make_unique(const time_point<seconds> &tp) {
            time_zone::civil_lookup cl;
            cl.kind = time_zone::civil_lookup::UNIQUE;
            cl.pre = cl.trans = cl.post = tp;
            return cl;
        }

        TURBO_FORCE_INLINE time_zone::civil_lookup make_unique(std::int_fast64_t unix_time) {
            return make_unique(from_unix_seconds(unix_time));
        }

        TURBO_FORCE_INLINE time_zone::civil_lookup make_skipped(const transition &tr,
                                                                const civil_second &cs) {
            time_zone::civil_lookup cl;
            cl.kind = time_zone::civil_lookup::SKIPPED;
            cl.pre = from_unix_seconds(tr.unix_time - 1 + (cs - tr.prev_civil_sec));
            cl.trans = from_unix_seconds(tr.unix_time);
            cl.post = from_unix_seconds(tr.unix_time - (tr.civil_sec - cs));
            return cl;
        }

        TURBO_FORCE_INLINE time_zone::civil_lookup make_repeated(const transition &tr,
                                                                 const civil_second &cs) {
            time_zone::civil_lookup cl;
            cl.kind = time_zone::civil_lookup::REPEATED;
            cl.pre = from_unix_seconds(tr.unix_time - 1 - (tr.prev_civil_sec - cs));
            cl.trans = from_unix_seconds(tr.unix_time);
            cl.post = from_unix_seconds(tr.unix_time + (cs - tr.civil_sec));
            return cl;
        }

        TURBO_FORCE_INLINE civil_second year_shift(const civil_second &cs, year_t shift) {
            return civil_second(cs.year() + shift, cs.month(), cs.day(), cs.hour(),
                                cs.minute(), cs.second());
        }

    }  // namespace

// What (no leap-seconds) UTC+seconds zoneinfo would look like.
    bool time_zone_info::reset_to_builtin_utc(const seconds &offset) {
        transition_types_.resize(1);
        transition_type &tt(transition_types_.back());
        tt.utc_offset = static_cast<std::int_least32_t>(offset.count());
        tt.is_dst = false;
        tt.abbr_index = 0;

        // We temporarily add some redundant, contemporary (2013 through 2023)
        // transitions for performance reasons.  See time_zone_info::local_time().
        // TODO: Fix the performance issue and remove the extra transitions.
        transitions_.clear();
        transitions_.reserve(12);
        for (const std::int_fast64_t unix_time : {
                -(1LL << 59),  // BIG_BANG
                1356998400LL,  // 2013-01-01T00:00:00+00:00
                1388534400LL,  // 2014-01-01T00:00:00+00:00
                1420070400LL,  // 2015-01-01T00:00:00+00:00
                1451606400LL,  // 2016-01-01T00:00:00+00:00
                1483228800LL,  // 2017-01-01T00:00:00+00:00
                1514764800LL,  // 2018-01-01T00:00:00+00:00
                1546300800LL,  // 2019-01-01T00:00:00+00:00
                1577836800LL,  // 2020-01-01T00:00:00+00:00
                1609459200LL,  // 2021-01-01T00:00:00+00:00
                1640995200LL,  // 2022-01-01T00:00:00+00:00
                1672531200LL,  // 2023-01-01T00:00:00+00:00
                2147483647LL,  // 2^31 - 1
        }) {
            transition &tr(*transitions_.emplace(transitions_.end()));
            tr.unix_time = unix_time;
            tr.type_index = 0;
            tr.civil_sec = local_time(tr.unix_time, tt).cs;
            tr.prev_civil_sec = tr.civil_sec - 1;
        }

        default_transition_type_ = 0;
        abbreviations_ = fixed_offset_to_abbr(offset);
        abbreviations_.append(1, '\0');  // add NUL
        future_spec_.clear();            // never needed for a fixed-offset zone
        extended_ = false;

        tt.civil_max = local_time(seconds::max().count(), tt).cs;
        tt.civil_min = local_time(seconds::min().count(), tt).cs;

        transitions_.shrink_to_fit();
        return true;
    }

// Builds the in-memory header using the raw bytes from the file.
    bool time_zone_info::header::build(const tzhead &tzh) {
        std::int_fast32_t v;
        if ((v = decode32(tzh.tzh_timecnt)) < 0)
            return false;
        timecnt = static_cast<std::size_t>(v);
        if ((v = decode32(tzh.tzh_typecnt)) < 0)
            return false;
        typecnt = static_cast<std::size_t>(v);
        if ((v = decode32(tzh.tzh_charcnt)) < 0)
            return false;
        charcnt = static_cast<std::size_t>(v);
        if ((v = decode32(tzh.tzh_leapcnt)) < 0)
            return false;
        leapcnt = static_cast<std::size_t>(v);
        if ((v = decode32(tzh.tzh_ttisstdcnt)) < 0)
            return false;
        ttisstdcnt = static_cast<std::size_t>(v);
        if ((v = decode32(tzh.tzh_ttisutcnt)) < 0)
            return false;
        if ((v = decode32(tzh.tzh_ttisutcnt)) < 0)
            return false;
        ttisutcnt = static_cast<std::size_t>(v);
        return true;
    }

// How many bytes of data are associated with this header. The result
// depends upon whether this is a section with 4-byte or 8-byte times.
    std::size_t time_zone_info::header::data_length(std::size_t time_len) const {
        std::size_t len = 0;
        len += (time_len + 1) * timecnt;  // unix_time + type_index
        len += (4 + 1 + 1) * typecnt;     // utc_offset + is_dst + abbr_index
        len += 1 * charcnt;               // abbreviations
        len += (time_len + 4) * leapcnt;  // leap-time + TAI-UTC
        len += 1 * ttisstdcnt;            // UTC/local indicators
        len += 1 * ttisutcnt;             // standard/wall indicators
        return len;
    }

// Check that the transition_type has the expected offset/is_dst/abbreviation.
    void time_zone_info::check_transition(const std::string &name,
                                          const transition_type &tt,
                                          std::int_fast32_t offset, bool is_dst,
                                          const std::string &abbr) const {
        if (tt.utc_offset != offset || tt.is_dst != is_dst ||
            &abbreviations_[tt.abbr_index] != abbr) {
            std::clog << name << ": Transition"
                      << " offset=" << tt.utc_offset << "/"
                      << (tt.is_dst ? "DST" : "STD")
                      << "/abbr=" << &abbreviations_[tt.abbr_index]
                      << " does not match POSIX spec '" << future_spec_ << "'\n";
        }
    }

// zic(8) can generate no-op transitions when a zone changes rules at an
// instant when there is actually no discontinuity.  So we check whether
// two transitions have equivalent types (same offset/is_dst/abbr).
    bool time_zone_info::equiv_transitions(std::uint_fast8_t tt1_index,
                                           std::uint_fast8_t tt2_index) const {
        if (tt1_index == tt2_index)
            return true;
        const transition_type &tt1(transition_types_[tt1_index]);
        const transition_type &tt2(transition_types_[tt2_index]);
        if (tt1.is_dst != tt2.is_dst)
            return false;
        if (tt1.utc_offset != tt2.utc_offset)
            return false;
        if (tt1.abbr_index != tt2.abbr_index)
            return false;
        return true;
    }

// Use the POSIX-TZ-environment-variable-style string to handle times
// in years after the last transition stored in the zoneinfo data.
    void time_zone_info::extend_transitions(const std::string &name,
                                            const header &hdr) {
        extended_ = false;
        bool extending = !future_spec_.empty();

        posix_time_zone posix;
        if (extending && !parse_posix_spec(future_spec_, &posix)) {
            std::clog << name << ": Failed to parse '" << future_spec_ << "'\n";
            extending = false;
        }

        if (extending && posix.dst_abbr.empty()) {  // std only
            // The future specification should match the last/default transition,
            // and that means that handling the future will fall out naturally.
            std::uint_fast8_t index = default_transition_type_;
            if (hdr.timecnt != 0)
                index = transitions_[hdr.timecnt - 1].type_index;
            const transition_type &tt(transition_types_[index]);
            check_transition(name, tt, posix.std_offset, false, posix.std_abbr);
            extending = false;
        }

        if (extending && hdr.timecnt < 2) {
            std::clog << name << ": Too few transitions for POSIX spec\n";
            extending = false;
        }

        if (!extending) {
            // Ensure that there is always a transition in the second half of the
            // time line (the BIG_BANG transition is in the first half) so that the
            // signed difference between a civil_second and the civil_second of its
            // previous transition is always representable, without overflow.
            const transition &last(transitions_.back());
            if (last.unix_time < 0) {
                const std::uint_fast8_t type_index = last.type_index;
                transition &tr(*transitions_.emplace(transitions_.end()));
                tr.unix_time = 2147483647;  // 2038-01-19T03:14:07+00:00
                tr.type_index = type_index;
            }
            return;  // last transition wins
        }

        // Extend the transitions for an additional 400 years using the
        // future specification. Years beyond those can be handled by
        // mapping back to a cycle-equivalent year within that range.
        // zic(8) should probably do this so that we don't have to.
        // TODO: Reduce the extension by the number of compatible
        // transitions already in place.
        transitions_.reserve(hdr.timecnt + 400 * 2 + 1);
        transitions_.resize(hdr.timecnt + 400 * 2);
        extended_ = true;

        // The future specification should match the last two transitions,
        // and those transitions should have different is_dst flags.  Note
        // that nothing says the UTC offset used by the is_dst transition
        // must be greater than that used by the !is_dst transition.  (See
        // Europe/Dublin, for example.)
        const transition *tr0 = &transitions_[hdr.timecnt - 1];
        const transition *tr1 = &transitions_[hdr.timecnt - 2];
        const transition_type *tt0 = &transition_types_[tr0->type_index];
        const transition_type *tt1 = &transition_types_[tr1->type_index];
        const transition_type &dst(tt0->is_dst ? *tt0 : *tt1);
        const transition_type &std(tt0->is_dst ? *tt1 : *tt0);
        check_transition(name, dst, posix.dst_offset, true, posix.dst_abbr);
        check_transition(name, std, posix.std_offset, false, posix.std_abbr);

        // Add the transitions to tr1 and back to tr0 for each extra year.
        last_year_ = local_time(tr0->unix_time, *tt0).cs.year();
        bool leap_year = is_leap(last_year_);
        const civil_day jan1(last_year_, 1, 1);
        std::int_fast64_t jan1_time = civil_second(jan1) - civil_second();
        int jan1_weekday = (static_cast<int>(get_weekday(jan1)) + 1) % 7;
        transition *tr = &transitions_[hdr.timecnt];  // next trans to fill
        if (local_time(tr1->unix_time, *tt1).cs.year() != last_year_) {
            // Add a single extra transition to align to a calendar year.
            transitions_.resize(transitions_.size() + 1);
            assert(tr == &transitions_[hdr.timecnt]);  // no reallocation
            const posix_transition &pt1(tt0->is_dst ? posix.dst_end : posix.dst_start);
            std::int_fast64_t tr1_offset = trans_offset(leap_year, jan1_weekday, pt1);
            tr->unix_time = jan1_time + tr1_offset - tt0->utc_offset;
            tr++->type_index = tr1->type_index;
            tr0 = &transitions_[hdr.timecnt];
            tr1 = &transitions_[hdr.timecnt - 1];
            tt0 = &transition_types_[tr0->type_index];
            tt1 = &transition_types_[tr1->type_index];
        }
        const posix_transition &pt1(tt0->is_dst ? posix.dst_end : posix.dst_start);
        const posix_transition &pt0(tt0->is_dst ? posix.dst_start : posix.dst_end);
        for (const year_t limit = last_year_ + 400; last_year_ < limit;) {
            last_year_ += 1;  // an additional year of generated transitions
            jan1_time += kSecsPerYear[leap_year];
            jan1_weekday = (jan1_weekday + kDaysPerYear[leap_year]) % 7;
            leap_year = !leap_year && is_leap(last_year_);
            std::int_fast64_t tr1_offset = trans_offset(leap_year, jan1_weekday, pt1);
            tr->unix_time = jan1_time + tr1_offset - tt0->utc_offset;
            tr++->type_index = tr1->type_index;
            std::int_fast64_t tr0_offset = trans_offset(leap_year, jan1_weekday, pt0);
            tr->unix_time = jan1_time + tr0_offset - tt1->utc_offset;
            tr++->type_index = tr0->type_index;
        }
        assert(tr == &transitions_[0] + transitions_.size());
    }

    bool time_zone_info::load(const std::string &name, zone_info_source *zip) {
        // Read and validate the header.
        tzhead tzh;
        if (zip->read(&tzh, sizeof(tzh)) != sizeof(tzh))
            return false;
        if (strncmp(tzh.tzh_magic, TZ_MAGIC, sizeof(tzh.tzh_magic)) != 0)
            return false;
        header hdr;
        if (!hdr.build(tzh))
            return false;
        std::size_t time_len = 4;
        if (tzh.tzh_version[0] != '\0') {
            // Skip the 4-byte data.
            if (zip->skip(hdr.data_length(time_len)) != 0)
                return false;
            // Read and validate the header for the 8-byte data.
            if (zip->read(&tzh, sizeof(tzh)) != sizeof(tzh))
                return false;
            if (strncmp(tzh.tzh_magic, TZ_MAGIC, sizeof(tzh.tzh_magic)) != 0)
                return false;
            if (tzh.tzh_version[0] == '\0')
                return false;
            if (!hdr.build(tzh))
                return false;
            time_len = 8;
        }
        if (hdr.typecnt == 0)
            return false;
        if (hdr.leapcnt != 0) {
            // This code assumes 60-second minutes so we do not want
            // the leap-second encoded zoneinfo. We could reverse the
            // compensation, but the "right" encoding is rarely used
            // so currently we simply reject such data.
            return false;
        }
        if (hdr.ttisstdcnt != 0 && hdr.ttisstdcnt != hdr.typecnt)
            return false;
        if (hdr.ttisutcnt != 0 && hdr.ttisutcnt != hdr.typecnt)
            return false;

        // Read the data into a local buffer.
        std::size_t len = hdr.data_length(time_len);
        std::vector<char> tbuf(len);
        if (zip->read(tbuf.data(), len) != len)
            return false;
        const char *bp = tbuf.data();

        // Decode and validate the transitions.
        transitions_.reserve(hdr.timecnt + 2);  // We might add a couple.
        transitions_.resize(hdr.timecnt);
        for (std::size_t i = 0; i != hdr.timecnt; ++i) {
            transitions_[i].unix_time = (time_len == 4) ? decode32(bp) : decode64(bp);
            bp += time_len;
            if (i != 0) {
                // Check that the transitions are ordered by time (as zic guarantees).
                if (!transition::by_unix_time()(transitions_[i - 1], transitions_[i]))
                    return false;  // out of order
            }
        }
        bool seen_type_0 = false;
        for (std::size_t i = 0; i != hdr.timecnt; ++i) {
            transitions_[i].type_index = decode8(bp++);
            if (transitions_[i].type_index >= hdr.typecnt)
                return false;
            if (transitions_[i].type_index == 0)
                seen_type_0 = true;
        }

        // Decode and validate the transition types.
        transition_types_.resize(hdr.typecnt);
        for (std::size_t i = 0; i != hdr.typecnt; ++i) {
            transition_types_[i].utc_offset =
                    static_cast<std::int_least32_t>(decode32(bp));
            if (transition_types_[i].utc_offset >= kSecsPerDay ||
                transition_types_[i].utc_offset <= -kSecsPerDay)
                return false;
            bp += 4;
            transition_types_[i].is_dst = (decode8(bp++) != 0);
            transition_types_[i].abbr_index = decode8(bp++);
            if (transition_types_[i].abbr_index >= hdr.charcnt)
                return false;
        }

        // Determine the before-first-transition type.
        default_transition_type_ = 0;
        if (seen_type_0 && hdr.timecnt != 0) {
            std::uint_fast8_t index = 0;
            if (transition_types_[0].is_dst) {
                index = transitions_[0].type_index;
                while (index != 0 && transition_types_[index].is_dst)
                    --index;
            }
            while (index != hdr.typecnt && transition_types_[index].is_dst)
                ++index;
            if (index != hdr.typecnt)
                default_transition_type_ = index;
        }

        // Copy all the abbreviations.
        abbreviations_.assign(bp, hdr.charcnt);
        bp += hdr.charcnt;

        // Skip the unused portions. We've already dispensed with leap-second
        // encoded zoneinfo. The ttisstd/ttisgmt indicators only apply when
        // interpreting a POSIX spec that does not include start/end rules, and
        // that isn't the case here (see "zic -p").
        bp += (8 + 4) * hdr.leapcnt;  // leap-time + TAI-UTC
        bp += 1 * hdr.ttisstdcnt;     // UTC/local indicators
        bp += 1 * hdr.ttisutcnt;      // standard/wall indicators
        assert(bp == tbuf.data() + tbuf.size());

        future_spec_.clear();
        if (tzh.tzh_version[0] != '\0') {
            // Snarf up the NL-enclosed future POSIX spec. Note
            // that version '3' files utilize an extended format.
            auto get_char = [](zone_info_source *azip) -> int {
                unsigned char ch;  // all non-EOF results are positive
                return (azip->read(&ch, 1) == 1) ? ch : EOF;
            };
            if (get_char(zip) != '\n')
                return false;
            for (int c = get_char(zip); c != '\n'; c = get_char(zip)) {
                if (c == EOF)
                    return false;
                future_spec_.push_back(static_cast<char>(c));
            }
        }

        // We don't check for EOF so that we're forwards compatible.

        // If we did not find version information during the standard loading
        // process (as of tzh_version '3' that is unsupported), then ask the
        // zone_info_source for any out-of-bound version std::string it may be privy to.
        if (version_.empty()) {
            version_ = zip->version();
        }

        // Trim redundant transitions. zic may have added these to work around
        // differences between the glibc and reference implementations (see
        // zic.c:dontmerge) and the Qt library (see zic.c:WORK_AROUND_QTBUG_53071).
        // For us, they just get in the way when we do future_spec_ extension.
        while (hdr.timecnt > 1) {
            if (!equiv_transitions(transitions_[hdr.timecnt - 1].type_index,
                                   transitions_[hdr.timecnt - 2].type_index)) {
                break;
            }
            hdr.timecnt -= 1;
        }
        transitions_.resize(hdr.timecnt);

        // Ensure that there is always a transition in the first half of the
        // time line (the second half is handled in ExtendTransitions()) so that
        // the signed difference between a civil_second and the civil_second of
        // its previous transition is always representable, without overflow.
        // A contemporary zic will usually have already done this for us.
        if (transitions_.empty() || transitions_.front().unix_time >= 0) {
            transition &tr(*transitions_.emplace(transitions_.begin()));
            tr.unix_time = -(1LL << 59);  // see tz/zic.c "BIG_BANG"
            tr.type_index = default_transition_type_;
            hdr.timecnt += 1;
        }

        // Extend the transitions using the future specification.
        extend_transitions(name, hdr);

        // Compute the local civil time for each transition and the preceding
        // second. These will be used for reverse conversions in MakeTime().
        const transition_type *ttp = &transition_types_[default_transition_type_];
        for (std::size_t i = 0; i != transitions_.size(); ++i) {
            transition &tr(transitions_[i]);
            tr.prev_civil_sec = local_time(tr.unix_time, *ttp).cs - 1;
            ttp = &transition_types_[tr.type_index];
            tr.civil_sec = local_time(tr.unix_time, *ttp).cs;
            if (i != 0) {
                // Check that the transitions are ordered by civil time. Essentially
                // this means that an offset change cannot cross another such change.
                // No one does this in practice, and we depend on it in MakeTime().
                if (!transition::by_civil_time()(transitions_[i - 1], tr))
                    return false;  // out of order
            }
        }

        // Compute the maximum/minimum civil times that can be converted to a
        // time_point<seconds> for each of the zone's transition types.
        for (auto &tt : transition_types_) {
            tt.civil_max = local_time(seconds::max().count(), tt).cs;
            tt.civil_min = local_time(seconds::min().count(), tt).cs;
        }

        transitions_.shrink_to_fit();
        return true;
    }

    namespace {

// fopen(3) adaptor.
        TURBO_FORCE_INLINE FILE *FOpen(const char *path, const char *mode) {
#if defined(_MSC_VER)
            FILE* fp;
          if (fopen_s(&fp, path, mode) != 0) fp = nullptr;
          return fp;
#else
            return fopen(path, mode);  // TODO: Enable the close-on-exec flag.
#endif
        }

// A stdio(3)-backed implementation of zone_info_source.
        class FileZoneInfoSource : public zone_info_source {
        public:
            static std::unique_ptr<zone_info_source> Open(const std::string &name);

            std::size_t read(void *ptr, std::size_t size) override {
                size = std::min(size, len_);
                std::size_t nread = fread(ptr, 1, size, fp_.get());
                len_ -= nread;
                return nread;
            }

            int skip(std::size_t offset) override {
                offset = std::min(offset, len_);
                int rc = fseek(fp_.get(), static_cast<long>(offset), SEEK_CUR);
                if (rc == 0)
                    len_ -= offset;
                return rc;
            }

            std::string version() const override {
                // TODO: It would nice if the zoneinfo data included the tzdb version.
                return std::string();
            }

        protected:
            explicit FileZoneInfoSource(
                    FILE *fp, std::size_t len = std::numeric_limits<std::size_t>::max())
                    : fp_(fp, fclose), len_(len) {}

        private:
            std::unique_ptr<FILE, int (*)(FILE *)> fp_;
            std::size_t len_;
        };

        std::unique_ptr<zone_info_source> FileZoneInfoSource::Open(
                const std::string &name) {
            // Use of the "file:" prefix is intended for testing purposes only.
            const std::size_t pos = (name.compare(0, 5, "file:") == 0) ? 5 : 0;

            // Map the time-zone name to a path name.
            std::string path;
            if (pos == name.size() || name[pos] != '/') {
                const char *tzdir = "/usr/share/zoneinfo";
                char *tzdir_env = nullptr;
#if defined(_MSC_VER)
                _dupenv_s(&tzdir_env, nullptr, "TZDIR");
#else
                tzdir_env = std::getenv("TZDIR");
#endif
                if (tzdir_env && *tzdir_env)
                    tzdir = tzdir_env;
                path += tzdir;
                path += '/';
#if defined(_MSC_VER)
                free(tzdir_env);
#endif
            }
            path.append(name, pos, std::string::npos);

            // Open the zoneinfo file.
            FILE *fp = FOpen(path.c_str(), "rb");
            if (fp == nullptr)
                return nullptr;
            std::size_t length = 0;
            if (fseek(fp, 0, SEEK_END) == 0) {
                long fpos = ftell(fp);
                if (fpos >= 0) {
                    length = static_cast<std::size_t>(fpos);
                }
                rewind(fp);
            }
            return std::unique_ptr<zone_info_source>(new FileZoneInfoSource(fp, length));
        }

        class AndroidZoneInfoSource : public FileZoneInfoSource {
        public:
            static std::unique_ptr<zone_info_source> Open(const std::string &name);

            std::string version() const override { return version_; }

        private:
            explicit AndroidZoneInfoSource(FILE *fp, std::size_t len, const char *vers)
                    : FileZoneInfoSource(fp, len), version_(vers) {}

            std::string version_;
        };

        std::unique_ptr<zone_info_source> AndroidZoneInfoSource::Open(
                const std::string &name) {
            // Use of the "file:" prefix is intended for testing purposes only.
            const std::size_t pos = (name.compare(0, 5, "file:") == 0) ? 5 : 0;

            // See Android's libc/tzcode/bionic.cc for additional information.
            for (const char *tzdata : {"/data/misc/zoneinfo/current/tzdata",
                                       "/system/usr/share/zoneinfo/tzdata"}) {
                std::unique_ptr<FILE, int (*)(FILE *)> fp(FOpen(tzdata, "rb"), fclose);
                if (fp.get() == nullptr)
                    continue;

                char hbuf[24];  // covers header.zonetab_offset too
                if (fread(hbuf, 1, sizeof(hbuf), fp.get()) != sizeof(hbuf))
                    continue;
                if (strncmp(hbuf, "tzdata", 6) != 0)
                    continue;
                const char *vers = (hbuf[11] == '\0') ? hbuf + 6 : "";
                const std::int_fast32_t index_offset = decode32(hbuf + 12);
                const std::int_fast32_t data_offset = decode32(hbuf + 16);
                if (index_offset < 0 || data_offset < index_offset)
                    continue;
                if (fseek(fp.get(), static_cast<long>(index_offset), SEEK_SET) != 0)
                    continue;

                char ebuf[52];  // covers entry.unused too
                const std::size_t index_size =
                        static_cast<std::size_t>(data_offset - index_offset);
                const std::size_t zonecnt = index_size / sizeof(ebuf);
                if (zonecnt * sizeof(ebuf) != index_size)
                    continue;
                for (std::size_t i = 0; i != zonecnt; ++i) {
                    if (fread(ebuf, 1, sizeof(ebuf), fp.get()) != sizeof(ebuf))
                        break;
                    const std::int_fast32_t start = data_offset + decode32(ebuf + 40);
                    const std::int_fast32_t length = decode32(ebuf + 44);
                    if (start < 0 || length < 0)
                        break;
                    ebuf[40] = '\0';  // ensure zone name is NUL terminated
                    if (strcmp(name.c_str() + pos, ebuf) == 0) {
                        if (fseek(fp.get(), static_cast<long>(start), SEEK_SET) != 0)
                            break;
                        return std::unique_ptr<zone_info_source>(new AndroidZoneInfoSource(
                                fp.release(), static_cast<std::size_t>(length), vers));
                    }
                }
            }

            return nullptr;
        }

    }  // namespace

    bool time_zone_info::load(const std::string &name) {
        // We can ensure that the loading of UTC or any other fixed-offset
        // zone never fails because the simple, fixed-offset state can be
        // internally generated. Note that this depends on our choice to not
        // accept leap-second encoded ("right") zoneinfo.
        auto offset = seconds::zero();
        if (fixed_offset_from_name(name, &offset)) {
            return reset_to_builtin_utc(offset);
        }

        // Find and use a zone_info_source to load the named zone.
        auto zip = zone_info_source_factory(
                name, [](const std::string &sname) -> std::unique_ptr<zone_info_source> {
                    if (auto tzip = FileZoneInfoSource::Open(sname))
                        return tzip;
                    if (auto tzip = AndroidZoneInfoSource::Open(sname))
                        return tzip;
                    return nullptr;
                });
        return zip != nullptr && load(name, zip.get());
    }

// BreakTime() translation for a particular transition type.
    time_zone::absolute_lookup time_zone_info::local_time(
            std::int_fast64_t unix_time, const transition_type &tt) const {
        // A civil time in "+offset" looks like (time+offset) in UTC.
        // Note: We perform two additions in the civil_second domain to
        // sidestep the chance of overflow in (unix_time + tt.utc_offset).
        return {(civil_second() + unix_time) + tt.utc_offset, tt.utc_offset,
                tt.is_dst, &abbreviations_[tt.abbr_index]};
    }

// BreakTime() translation for a particular transition.
    time_zone::absolute_lookup time_zone_info::local_time(std::int_fast64_t unix_time,
                                                          const transition &tr) const {
        const transition_type &tt = transition_types_[tr.type_index];
        // Note: (unix_time - tr.unix_time) will never overflow as we
        // have ensured that there is always a "nearby" transition.
        return {tr.civil_sec + (unix_time - tr.unix_time),  // TODO: Optimize.
                tt.utc_offset, tt.is_dst, &abbreviations_[tt.abbr_index]};
    }

// MakeTime() translation with a conversion-preserving +N * 400-year shift.
    time_zone::civil_lookup time_zone_info::time_local(const civil_second &cs,
                                                       year_t c4_shift) const {
        assert(last_year_ - 400 < cs.year() && cs.year() <= last_year_);
        time_zone::civil_lookup cl = make_time(cs);
        if (c4_shift > seconds::max().count() / kSecsPer400Years) {
            cl.pre = cl.trans = cl.post = time_point<seconds>::max();
        } else {
            const auto offset = seconds(c4_shift * kSecsPer400Years);
            const auto limit = time_point<seconds>::max() - offset;
            for (auto *tp : {&cl.pre, &cl.trans, &cl.post}) {
                if (*tp > limit) {
                    *tp = time_point<seconds>::max();
                } else {
                    *tp += offset;
                }
            }
        }
        return cl;
    }

    time_zone::absolute_lookup time_zone_info::break_time(
            const time_point<seconds> &tp) const {
        std::int_fast64_t unix_time = to_unix_seconds(tp);
        const std::size_t timecnt = transitions_.size();
        assert(timecnt != 0);  // We always add a transition.

        if (unix_time < transitions_[0].unix_time) {
            return local_time(unix_time, transition_types_[default_transition_type_]);
        }
        if (unix_time >= transitions_[timecnt - 1].unix_time) {
            // After the last transition. If we extended the transitions using
            // future_spec_, shift back to a supported year using the 400-year
            // cycle of calendaric equivalence and then compensate accordingly.
            if (extended_) {
                const std::int_fast64_t diff =
                        unix_time - transitions_[timecnt - 1].unix_time;
                const year_t shift = diff / kSecsPer400Years + 1;
                const auto d = seconds(shift * kSecsPer400Years);
                time_zone::absolute_lookup al = break_time(tp - d);
                al.cs = year_shift(al.cs, shift * 400);
                return al;
            }
            return local_time(unix_time, transitions_[timecnt - 1]);
        }

        const std::size_t hint = local_time_hint_.load(std::memory_order_relaxed);
        if (0 < hint && hint < timecnt) {
            if (transitions_[hint - 1].unix_time <= unix_time) {
                if (unix_time < transitions_[hint].unix_time) {
                    return local_time(unix_time, transitions_[hint - 1]);
                }
            }
        }

        const transition target = {unix_time, 0, civil_second(), civil_second()};
        const transition *begin = &transitions_[0];
        const transition *tr = std::upper_bound(begin, begin + timecnt, target,
                                                transition::by_unix_time());
        local_time_hint_.store(static_cast<std::size_t>(tr - begin),
                               std::memory_order_relaxed);
        return local_time(unix_time, *--tr);
    }

    time_zone::civil_lookup time_zone_info::make_time(const civil_second &cs) const {
        const std::size_t timecnt = transitions_.size();
        assert(timecnt != 0);  // We always add a transition.

        // Find the first transition after our target civil time.
        const transition *tr = nullptr;
        const transition *begin = &transitions_[0];
        const transition *end = begin + timecnt;
        if (cs < begin->civil_sec) {
            tr = begin;
        } else if (cs >= transitions_[timecnt - 1].civil_sec) {
            tr = end;
        } else {
            const std::size_t hint = time_local_hint_.load(std::memory_order_relaxed);
            if (0 < hint && hint < timecnt) {
                if (transitions_[hint - 1].civil_sec <= cs) {
                    if (cs < transitions_[hint].civil_sec) {
                        tr = begin + hint;
                    }
                }
            }
            if (tr == nullptr) {
                const transition target = {0, 0, cs, civil_second()};
                tr = std::upper_bound(begin, end, target, transition::by_civil_time());
                time_local_hint_.store(static_cast<std::size_t>(tr - begin),
                                       std::memory_order_relaxed);
            }
        }

        if (tr == begin) {
            if (tr->prev_civil_sec >= cs) {
                // Before first transition, so use the default offset.
                const transition_type &tt(transition_types_[default_transition_type_]);
                if (cs < tt.civil_min)
                    return make_unique(time_point<seconds>::min());
                return make_unique(cs - (civil_second() + tt.utc_offset));
            }
            // tr->prev_civil_sec < cs < tr->civil_sec
            return make_skipped(*tr, cs);
        }

        if (tr == end) {
            if (cs > (--tr)->prev_civil_sec) {
                // After the last transition. If we extended the transitions using
                // future_spec_, shift back to a supported year using the 400-year
                // cycle of calendaric equivalence and then compensate accordingly.
                if (extended_ && cs.year() > last_year_) {
                    const year_t shift = (cs.year() - last_year_ - 1) / 400 + 1;
                    return time_local(year_shift(cs, shift * -400), shift);
                }
                const transition_type &tt(transition_types_[tr->type_index]);
                if (cs > tt.civil_max)
                    return make_unique(time_point<seconds>::max());
                return make_unique(tr->unix_time + (cs - tr->civil_sec));
            }
            // tr->civil_sec <= cs <= tr->prev_civil_sec
            return make_repeated(*tr, cs);
        }

        if (tr->prev_civil_sec < cs) {
            // tr->prev_civil_sec < cs < tr->civil_sec
            return make_skipped(*tr, cs);
        }

        if (cs <= (--tr)->prev_civil_sec) {
            // tr->civil_sec <= cs <= tr->prev_civil_sec
            return make_repeated(*tr, cs);
        }

        // In between transitions.
        return make_unique(tr->unix_time + (cs - tr->civil_sec));
    }

    std::string time_zone_info::version() const { return version_; }

    std::string time_zone_info::description() const {
        std::ostringstream oss;
        oss << "#trans=" << transitions_.size();
        oss << " #types=" << transition_types_.size();
        oss << " spec='" << future_spec_ << "'";
        return oss.str();
    }

    bool time_zone_info::next_transition(const time_point<seconds> &tp,
                                         time_zone::civil_transition *trans) const {
        if (transitions_.empty())
            return false;
        const transition *begin = &transitions_[0];
        const transition *end = begin + transitions_.size();
        if (begin->unix_time <= -(1LL << 59)) {
            // Do not report the BIG_BANG found in recent zoneinfo data as it is
            // really a sentinel, not a transition.  See tz/zic.c.
            ++begin;
        }
        std::int_fast64_t unix_time = to_unix_seconds(tp);
        const transition target = {unix_time, 0, civil_second(), civil_second()};
        const transition *tr =
                std::upper_bound(begin, end, target, transition::by_unix_time());
        for (; tr != end; ++tr) {  // skip no-op transitions
            std::uint_fast8_t prev_type_index =
                    (tr == begin) ? default_transition_type_ : tr[-1].type_index;
            if (!equiv_transitions(prev_type_index, tr[0].type_index))
                break;
        }
        // When tr == end we return false, ignoring future_spec_.
        if (tr == end)
            return false;
        trans->from = tr->prev_civil_sec + 1;
        trans->to = tr->civil_sec;
        return true;
    }

    bool time_zone_info::prev_transition(const time_point<seconds> &tp,
                                         time_zone::civil_transition *trans) const {
        if (transitions_.empty())
            return false;
        const transition *begin = &transitions_[0];
        const transition *end = begin + transitions_.size();
        if (begin->unix_time <= -(1LL << 59)) {
            // Do not report the BIG_BANG found in recent zoneinfo data as it is
            // really a sentinel, not a transition.  See tz/zic.c.
            ++begin;
        }
        std::int_fast64_t unix_time = to_unix_seconds(tp);
        if (from_unix_seconds(unix_time) != tp) {
            if (unix_time == std::numeric_limits<std::int_fast64_t>::max()) {
                if (end == begin)
                    return false;  // Ignore future_spec_.
                trans->from = (--end)->prev_civil_sec + 1;
                trans->to = end->civil_sec;
                return true;
            }
            unix_time += 1;  // ceils
        }
        const transition target = {unix_time, 0, civil_second(), civil_second()};
        const transition *tr =
                std::lower_bound(begin, end, target, transition::by_unix_time());
        for (; tr != begin; --tr) {  // skip no-op transitions
            std::uint_fast8_t prev_type_index =
                    (tr - 1 == begin) ? default_transition_type_ : tr[-2].type_index;
            if (!equiv_transitions(prev_type_index, tr[-1].type_index))
                break;
        }
        // When tr == end we return the "last" transition, ignoring future_spec_.
        if (tr == begin)
            return false;
        trans->from = (--tr)->prev_civil_sec + 1;
        trans->to = tr->civil_sec;
        return true;
    }

}  // namespace turbo::times_internal
