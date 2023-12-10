// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/unicode/utf.h"

#include <cstdint>
#include <utility>

#include "gtest/gtest.h"
#include "turbo/platform/port.h"

namespace {

#if !defined(__cpp_char8_t)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++2a-compat"
#endif
TEST(EncodeUTF8Char, BasicFunction) {
  std::pair<char32_t, std::string> tests[] = {{0x0030, u8"\u0030"},
                                              {0x00A3, u8"\u00A3"},
                                              {0x00010000, u8"\U00010000"},
                                              {0x0000FFFF, u8"\U0000FFFF"},
                                              {0x0010FFFD, u8"\U0010FFFD"}};
  for (auto &test : tests) {
    char buf0[7] = {'\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};
    char buf1[7] = {'\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF'};
    char *buf0_written =
        &buf0[turbo::convert_utf32_to_utf8(&test.first, 1, buf0)];
    char *buf1_written =
        &buf1[turbo::convert_utf32_to_utf8(&test.first, 1, buf1)];
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
  char32_t a= 0x00110000;
  EXPECT_LE(turbo::convert_utf32_to_utf8(&a, 1, buf), 4);
  char buf2[32] = "Negative is invalid but sane";
  char32_t b = -1;
  EXPECT_LE(turbo::convert_utf32_to_utf8(&b,1, buf2), 4);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif  // !defined(__cpp_char8_t)

}  // namespace
