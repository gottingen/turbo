
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_INTERNAL_TIME_ZONE_IF_H_
#define FLARE_TIMES_INTERNAL_TIME_ZONE_IF_H_

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include "flare/base/profile.h"
#include "flare/times/internal/chrono_time_internal.h"
#include "flare/times/internal/time_zone.h"

namespace flare::times_internal {


// A simple interface used to hide time-zone complexities from time_zone::Impl.
// Subclasses implement the functions for civil-time conversions in the zone.
    class time_zone_if {
    public:
        // A factory function for time_zone_if implementations.
        static std::unique_ptr<time_zone_if> load(const std::string &name);

        virtual ~time_zone_if();

        virtual time_zone::absolute_lookup break_time(
                const time_point<seconds> &tp) const = 0;

        virtual time_zone::civil_lookup make_time(const civil_second &cs) const = 0;

        virtual bool next_transition(const time_point<seconds> &tp,
                                     time_zone::civil_transition *trans) const = 0;

        virtual bool prev_transition(const time_point<seconds> &tp,
                                     time_zone::civil_transition *trans) const = 0;

        virtual std::string version() const = 0;

        virtual std::string description() const = 0;

    protected:
        time_zone_if() {}
    };

// Convert between time_point<seconds> and a count of seconds since the
// Unix epoch.  We assume that the std::chrono::system_clock and the
// Unix clock are second aligned, but not that they share an epoch.
    FLARE_FORCE_INLINE std::int_fast64_t to_unix_seconds(const time_point<seconds> &tp) {
        return (tp - std::chrono::time_point_cast<seconds>(
                std::chrono::system_clock::from_time_t(0)))
                .count();
    }

    FLARE_FORCE_INLINE time_point<seconds> from_unix_seconds(std::int_fast64_t t) {
        return std::chrono::time_point_cast<seconds>(
                std::chrono::system_clock::from_time_t(0)) +
               seconds(t);
    }


}  // namespace flare::times_internal
#endif  // FLARE_TIMES_INTERNAL_TIME_ZONE_IF_H_
