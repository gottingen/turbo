
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_TIMES_INTERNAL_TIME_ZONE_INFO_H_
#define FLARE_TIMES_INTERNAL_TIME_ZONE_INFO_H_


#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "flare/base/profile.h"
#include "flare/times/internal/chrono_time_internal.h"
#include "flare/times/internal/time_zone.h"
#include "flare/times/internal/zone_info_source.h"
#include "flare/times/internal/time_zone_if.h"
#include "flare/times/internal/tzfile.h"

namespace flare::times_internal {


// A transition to a new UTC offset.
    struct transition {
        std::int_least64_t unix_time;   // the instant of this transition
        std::uint_least8_t type_index;  // index of the transition type
        civil_second civil_sec;         // local civil time of transition
        civil_second prev_civil_sec;    // local civil time one second earlier

        struct by_unix_time {
            FLARE_FORCE_INLINE bool operator()(const transition &lhs, const transition &rhs) const {
                return lhs.unix_time < rhs.unix_time;
            }
        };

        struct by_civil_time {
            FLARE_FORCE_INLINE bool operator()(const transition &lhs, const transition &rhs) const {
                return lhs.civil_sec < rhs.civil_sec;
            }
        };
    };

// The characteristics of a particular transition.
    struct transition_type {
        std::int_least32_t utc_offset;  // the new prevailing UTC offset
        civil_second civil_max;         // max convertible civil time for offset
        civil_second civil_min;         // min convertible civil time for offset
        bool is_dst;                    // did we move into daylight-saving time
        std::uint_least8_t abbr_index;  // index of the new abbreviation
    };

// A time zone backed by the IANA time_point Zone Database (zoneinfo).
    class time_zone_info : public time_zone_if {
    public:
        time_zone_info() = default;

        time_zone_info(const time_zone_info &) = delete;

        time_zone_info &operator=(const time_zone_info &) = delete;

        // Loads the zoneinfo for the given name, returning true if successful.
        bool load(const std::string &name);

        // TimeZoneIf implementations.
        time_zone::absolute_lookup break_time(
                const time_point<seconds> &tp) const override;

        time_zone::civil_lookup make_time(const civil_second &cs) const override;

        bool next_transition(const time_point<seconds> &tp,
                             time_zone::civil_transition *trans) const override;

        bool prev_transition(const time_point<seconds> &tp,
                             time_zone::civil_transition *trans) const override;

        std::string version() const override;

        std::string description() const override;

    private:
        struct header {            // counts of:
            std::size_t timecnt;     // transition times
            std::size_t typecnt;     // transition types
            std::size_t charcnt;     // zone abbreviation characters
            std::size_t leapcnt;     // leap seconds (we expect none)
            std::size_t ttisstdcnt;  // UTC/local indicators (unused)
            std::size_t ttisutcnt;   // standard/wall indicators (unused)

            bool build(const tzhead &tzh);

            std::size_t data_length(std::size_t time_len) const;
        };

        void check_transition(const std::string &name, const transition_type &tt,
                              std::int_fast32_t offset, bool is_dst,
                              const std::string &abbr) const;

        bool equiv_transitions(std::uint_fast8_t tt1_index,
                               std::uint_fast8_t tt2_index) const;

        void extend_transitions(const std::string &name, const header &hdr);

        bool reset_to_builtin_utc(const seconds &offset);

        bool load(const std::string &name, zone_info_source *zip);

        // Helpers for BreakTime() and MakeTime().
        time_zone::absolute_lookup local_time(std::int_fast64_t unix_time,
                                              const transition_type &tt) const;

        time_zone::absolute_lookup local_time(std::int_fast64_t unix_time,
                                              const transition &tr) const;

        time_zone::civil_lookup time_local(const civil_second &cs,
                                           year_t c4_shift) const;

        std::vector<transition> transitions_;  // ordered by unix_time and civil_sec
        std::vector<transition_type> transition_types_;  // distinct transition types
        std::uint_fast8_t default_transition_type_;     // for before first transition
        std::string abbreviations_;  // all the NUL-terminated abbreviations

        std::string version_;      // the tzdata version if available
        std::string future_spec_;  // for after the last zic transition
        bool extended_;            // future_spec_ was used to generate transitions
        year_t last_year_;         // the final year of the generated transitions

        // We remember the transitions found during the last BreakTime() and
        // MakeTime() calls. If the next request is for the same transition we
        // will avoid re-searching.
        mutable std::atomic<std::size_t> local_time_hint_ = {};  // BreakTime() hint
        mutable std::atomic<std::size_t> time_local_hint_ = {};  // MakeTime() hint
    };

}  // namespace flare::times_internal

#endif  // FLARE_TIMES_INTERNAL_TIME_ZONE_INFO_H_
