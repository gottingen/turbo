
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_FLARE_TIMES_INTERNAL_TIME_ZONE_LIBC_H_
#define FLARE_FLARE_TIMES_INTERNAL_TIME_ZONE_LIBC_H_

#include <string>
#include "flare/base/profile.h"
#include "flare/times/internal/time_zone_if.h"

namespace flare::times_internal {


// A time zone backed by gmtime_r(3), localtime_r(3), and mktime(3),
// and which therefore only supports UTC and the local time zone.
// TODO: Add support for fixed offsets from UTC.
class time_zone_libc : public time_zone_if {
public:
    explicit time_zone_libc(const std::string &name);

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
    const bool local_;  // localtime or UTC
};

}  // namespace flare::times_internal

#endif  // FLARE_FLARE_TIMES_INTERNAL_TIME_ZONE_LIBC_H_
