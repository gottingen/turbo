// Copyright 2023 The Turbo Authors.
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"

#include "turbo/unicode/converter.h"
#include "transcode_test_base.h"

#include <array>
#include <algorithm>

#include "turbo/random/random.h"

#include <fstream>
#include <iostream>
#include <memory>

constexpr size_t num_trials = 1000;

TEST_CASE("no_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    const auto utf8{generator.generate(512)};
    turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
    REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
    REQUIRE_EQ(res.count, utf8.size());
  }
}


TEST_CASE("header_bits_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};

    for (int i = 0; i < 512; i++) {
      if((utf8[i] & 0b11000000) != 0b10000000) {  // Only process leading bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b11111000);
        turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
          turbo::UnicodeResult res1 = turbo::unicode::Converter<turbo::unicode::scalar_engine>::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        REQUIRE_EQ(res.error, turbo::UnicodeError::HEADER_BITS);
          REQUIRE_EQ(res1.count, i);
        REQUIRE_EQ(res.count, i);

        utf8[i] = old;
      }
    }
  }
}

TEST_CASE("too_short_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};
    int leading_byte_pos = 0;
    for (int i = 0; i < 512; i++) {
      if((utf8[i] & 0b11000000) == 0b10000000) {  // Only process continuation bytes by making them leading bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b11100000);
        turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_SHORT);
        REQUIRE_EQ(res.count, leading_byte_pos);
        utf8[i] = old;
      } else {
        leading_byte_pos = i;
      }
    }
  }
}

TEST_CASE("too_long_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};
    for (int i = 1; i < 512; i++) {
      if(((utf8[i] & 0b11000000) != 0b10000000)) {  // Only process leading bytes by making them continuation bytes
        const unsigned char old = utf8[i];
        utf8[i] = uint8_t(0b10000000);
        turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_LONG);
        REQUIRE_EQ(res.count, i);
        utf8[i] = old;
      }
    }
  }
}

TEST_CASE("overlong_error") {

  turbo::Utf8Generator generator{ 1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};
    for (int i = 1; i < 512; i++) {
      if(utf8[i] >= 0b11000000) { // Only non-ASCII leading bytes can be overlong
        const unsigned char old = utf8[i];
        const unsigned char second_old = utf8[i+1];
        if ((old & 0b11100000) == 0b11000000) { // two-bytes case, change to a value less or equal than 0x7f
          utf8[i] = 0b11000000;
        } else if ((old & 0b11110000) == 0b11100000) {  // three-bytes case, change to a value less or equal than 0x7ff
          utf8[i] = 0b11100000;
          utf8[i+1] = utf8[i+1] & 0b11011111;
        } else {  // four-bytes case, change to a value less or equal than 0xffff
          utf8[i] = 0b11110000;
          utf8[i+1] = utf8[i+1] & 0b11001111;
        }
        turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        REQUIRE_EQ(res.error, turbo::UnicodeError::OVERLONG);
        REQUIRE_EQ(res.count, i);
        utf8[i] = old;
        utf8[i+1] = second_old;
      }
    }
  }
}

TEST_CASE("too_large_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};
    for (int i = 1; i < 512; i++) {
      if((utf8[i] & 0b11111000) == 0b11110000) { // Can only have too large error in 4-bytes case
        utf8[i] += ((utf8[i] & 0b100) == 0b100) ? 0b10 : 0b100;   // Make sure we get too large error and not header bits error
        turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_LARGE);
        REQUIRE_EQ(res.count, i);
        utf8[i] -= 0b100;
      }
    }
  }
}

TEST_CASE("surrogate_error") {

  turbo::Utf8Generator generator{1, 1, 1, 1};
  for(size_t trial = 0; trial < num_trials; trial++) {
    auto utf8{generator.generate(512)};
    for (int i = 1; i < 512; i++) {
      if((utf8[i] & 0b11110000) == 0b11100000) { // Can only have surrogate error in 3-bytes case
        const unsigned char old = utf8[i];
        const unsigned char second_old = utf8[i+1];
        utf8[i] = 0b11101101;                 // Leading byte is always the same
        for (int s = 0x8; s < 0xf; s++) {  // Modify second byte to create a surrogate codepoint
          utf8[i+1] = (utf8[i+1] & 0b11000011) | (s << 2);
          turbo::UnicodeResult res = turbo::validate_utf8_with_errors(reinterpret_cast<const char*>(utf8.data()), utf8.size());
          REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
          REQUIRE_EQ(res.count, i);
        }
        utf8[i] = old;
        utf8[i+1] = second_old;
      }
    }
  }
}


