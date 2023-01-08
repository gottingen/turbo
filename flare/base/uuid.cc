
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "flare/base/uuid.h"

#include <cstdio>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include "flare/base/fast_rand.h"
#include "flare/strings/str_format.h"

namespace flare {

    std::string uuid::to_string() const {
        return string_format(
                "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:"
                "02x}{:02x}{:02x}{:02x}{:02x}",
                _bytes[0], _bytes[1], _bytes[2], _bytes[3], _bytes[4], _bytes[5],
                _bytes[6], _bytes[7], _bytes[8], _bytes[9], _bytes[10], _bytes[11],
                _bytes[12], _bytes[13], _bytes[14], _bytes[15]);
    }

    uuid uuid::generate() {
        uint64_t hi = flare::base::fast_rand();
        uint64_t low = flare::base::fast_rand();
        return uuid(hi, low);
    }

    bool parse_uuid(const std::string_view &s, uuid *ret) {
        static constexpr auto kExpectedLength = 36;
        static constexpr std::array<bool, kExpectedLength> kHexExpected = [&]() {
            std::array<bool, kExpectedLength> ary{};
            for (auto &&e : ary) {
                e = true;
            }
            ary[8] = ary[13] = ary[18] = ary[23] = false;
            return ary;
        }();

        if (s.size() != kExpectedLength) {
            return false;
        }
        for (int i = 0; i != kExpectedLength; ++i) {
            if (!kHexExpected[i]) {
                if (s[i] != '-') {
                    return false;
                }
            } else {
                bool valid = false;
                valid |= s[i] >= '0' && s[i] <= '9';
                valid |= s[i] >= 'a' && s[i] <= 'f';
                valid |= s[i] >= 'A' && s[i] <= 'F';
                if (!valid) {
                    return false;
                }
            }
        }
        *ret = uuid(s);
        return true;
    }

}  // namespace flare
