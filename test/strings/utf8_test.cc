
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/internal/utf8.h"

#include <cstdint>
#include <utility>

#include "testing/gtest_wrap.h"
#include "flare/base/profile.h"

namespace {

#if !defined(__cpp_char8_t)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++2a-compat"
#endif
    TEST(EncodeUTF8Char, BasicFunction) {
        std::pair<char32_t, std::string> tests[] = {{0x0030,     u8"\u0030"},
                                                    {0x00A3,     u8"\u00A3"},
                                                    {0x00010000, u8"\U00010000"},
                                                    {0x0000FFFF, u8"\U0000FFFF"},
                                                    {0x0010FFFD, u8"\U0010FFFD"}};
        for (auto &test : tests) {
            char buf0[7] = {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};
            char buf1[7] = {'\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF'};
            char *buf0_written =
                    &buf0[flare::strings_internal::EncodeUTF8Char(buf0, test.first)];
            char *buf1_written =
                    &buf1[flare::strings_internal::EncodeUTF8Char(buf1, test.first)];
            int apparent_length = 7;
            while (buf0[apparent_length - 1] == '\x00' &&
                   buf1[apparent_length - 1] == '\xFF') {
                if (--apparent_length == 0) break;
            }
            EXPECT_EQ(apparent_length, buf0_written - buf0);
            EXPECT_EQ(apparent_length, buf1_written - buf1);
            EXPECT_EQ(apparent_length, test.second.length());
            EXPECT_EQ(std::string(buf0, apparent_length), test.second);
            EXPECT_EQ(std::string(buf1, apparent_length), test.second);
        }
        char buf[32] = "Don't Tread On Me";
        EXPECT_LE(flare::strings_internal::EncodeUTF8Char(buf, 0x00110000),
                  flare::strings_internal::kMaxEncodedUTF8Size);
        char buf2[32] = "Negative is invalid but sane";
        EXPECT_LE(flare::strings_internal::EncodeUTF8Char(buf2, -1),
                  flare::strings_internal::kMaxEncodedUTF8Size);
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif  // !defined(__cpp_char8_t)

}  // namespace
