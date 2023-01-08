
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_TIMES_INTERNAL_TIME_ZONE_IMPL_H_
#define FLARE_TIMES_INTERNAL_TIME_ZONE_IMPL_H_

#include <memory>
#include <string>
#include "flare/base/profile.h"
#include "flare/times/internal/chrono_time_internal.h"
#include "flare/times/internal/time_zone.h"
#include "flare/times/internal/time_zone_if.h"
#include "flare/times/internal/time_zone_info.h"

namespace flare::times_internal {


// time_zone::Impl is the internal object referenced by a flare::times_internal::time_zone.
    class time_zone::Impl {
    public:
        // The UTC time zone. Also used for other time zones that fail to load.
        static time_zone UTC();

        // Load a named time zone. Returns false if the name is invalid, or if
        // some other kind of error occurs. Note that loading "UTC" never fails.
        static bool load_time_zone(const std::string &name, time_zone *tz);

        // Clears the map of cached time zones.  Primarily for use in benchmarks
        // that gauge the performance of loading/parsing the time-zone data.
        static void ClearTimeZoneMapTestOnly();

        // The primary key is the time-zone ID (e.g., "America/New_York").
        const std::string &Name() const {
            // TODO: It would nice if the zoneinfo data included the zone name.
            return name_;
        }

        // Breaks a time_point down to civil-time components in this time zone.
        time_zone::absolute_lookup BreakTime(const time_point<seconds> &tp) const {
            return zone_->break_time(tp);
        }

        // Converts the civil-time components in this time zone into a time_point.
        // That is, the opposite of BreakTime(). The requested civil time may be
        // ambiguous or illegal due to a change of UTC offset.
        time_zone::civil_lookup MakeTime(const civil_second &cs) const {
            return zone_->make_time(cs);
        }

        // Finds the time of the next/previous offset change in this time zone.
        bool next_transition(const time_point<seconds> &tp,
                             time_zone::civil_transition *trans) const {
            return zone_->next_transition(tp, trans);
        }

        bool prev_transition(const time_point<seconds> &tp,
                             time_zone::civil_transition *trans) const {
            return zone_->prev_transition(tp, trans);
        }

        // Returns an implementation-defined version std::string for this time zone.
        std::string Version() const { return zone_->version(); }

        // Returns an implementation-defined description of this time zone.
        std::string Description() const { return zone_->description(); }

    private:
        explicit Impl(const std::string &name);

        static const Impl *UTCImpl();

        const std::string name_;
        std::unique_ptr<time_zone_if> zone_;
    };


}  // namespace flare::times_internal
#endif  // FLARE_TIMES_INTERNAL_TIME_ZONE_IMPL_H_
