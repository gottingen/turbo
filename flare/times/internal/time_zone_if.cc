
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/times/internal/time_zone_if.h"
#include "flare/base/profile.h"
#include "flare/times/internal/time_zone_info.h"
#include "flare/times/internal/time_zone_libc.h"

namespace flare::times_internal {


    std::unique_ptr<time_zone_if> time_zone_if::load(const std::string &name) {
        // Support "libc:localtime" and "libc:*" to access the legacy
        // localtime and UTC support respectively from the C library.
        if (name.compare(0, 5, "libc:") == 0) {
            return std::unique_ptr<time_zone_if>(new time_zone_libc(name.substr(5)));
        }

        // Otherwise use the "zoneinfo" implementation by default.
        std::unique_ptr<time_zone_info> tz(new time_zone_info);
        if (!tz->load(name)) tz.reset();
        return std::unique_ptr<time_zone_if>(tz.release());
    }

    // Defined out-of-line to avoid emitting a weak vtable in all TUs.
    time_zone_if::~time_zone_if() {}

}  // namespace flare::times_internal

