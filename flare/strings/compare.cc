
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/compare.h"
#include "flare/strings/ascii.h"

namespace flare {

int compare_case(std::string_view a, std::string_view b) {
    std::string_view::const_iterator ai = a.begin();
    std::string_view::const_iterator bi = b.begin();

    while (ai != a.end() && bi != b.end()) {
        int ca = ascii::to_lower(*ai++);
        int cb = ascii::to_lower(*bi++);

        if (ca == cb) {
            continue;
        }
        if (ca < cb) {
            return -1;
        } else {
            return +1;
        }
    }

    if (ai == a.end() && bi != b.end()) {
        return +1;
    } else if (ai != a.end() && bi == b.end()) {
        return -1;
    } else {
        return 0;
    }
}

}  // namespace flare
