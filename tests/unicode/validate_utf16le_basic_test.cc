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

#ifndef TURBO_IS_BIG_ENDIAN
#error "TURBO_IS_BIG_ENDIAN should be defined."
#endif

#include <array>
#include <algorithm>


#include "turbo/random/random.h"

#include <fstream>
#include <iostream>
#include <memory>

TEST_CASE("issue92") {
  char16_t input[] = u"\u5d00\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041\u0041";
  size_t strlen = sizeof(input)/sizeof(char16_t)-1;
#if TURBO_IS_BIG_ENDIAN
  std::cout << "Flipping bytes because you have big endian system." << std::endl;
  turbo::change_endianness_utf16(input, strlen, input);
#endif
  REQUIRE(turbo::validate_utf16le(input, strlen));
  REQUIRE(turbo::utf8_length_from_utf16le(input, strlen)
     == 2 + strlen);
  size_t size = turbo::utf8_length_from_utf16le(input, strlen); // should be 26.
  std::unique_ptr<char[]> output_buffer{new char[size]};
  size_t  measured_size = turbo::convert_valid_utf16le_to_utf8(input, strlen, output_buffer.get());
  std::cout << "Expect " << size << " got " << measured_size << std::endl;
  REQUIRE(measured_size == size);
}

TEST_CASE("validate_utf16le__returns_true_for_valid_input__single_words") {

  turbo::Utf16Generator generator{ 1, 0};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(512)};

    REQUIRE(turbo::validate_utf16le(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
  }
}

TEST_CASE("validate_utf16le__returns_true_for_valid_input__surrogate_pairs_short") {

  turbo::Utf16Generator generator{ 0, 1};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(8)};

    REQUIRE(turbo::validate_utf16le(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
  }
}


TEST_CASE("validate_utf16le__returns_true_for_valid_input__surrogate_pairs") {

  turbo::Utf16Generator generator{ 0, 1};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf16{generator.generate(512)};

    REQUIRE(turbo::validate_utf16le(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
  }
}

// mixed = either 16-bit or 32-bit codewords
TEST_CASE("validate_utf16le__returns_true_for_valid_input__mixed") {

  turbo::Utf16Generator generator{ 1, 1};
  const auto utf16{generator.generate(512)};

  REQUIRE(turbo::validate_utf16le(
              reinterpret_cast<const char16_t*>(utf16.data()), utf16.size()));
}

TEST_CASE("validate_utf16le__returns_true_for_empty_string") {
  const char16_t* buf = (const char16_t*)"";

  REQUIRE(turbo::validate_utf16le(buf, 0));
}

// The first word must not be in range [0xDC00 .. 0xDFFF]
/*
2.2 Decoding UTF-16

   [...]

   1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
      of W1. Terminate.

   2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
      is in error [...]
*/
#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else
TEST_CASE("validate_utf16le__returns_false_when_input_has_wrong_first_word_value") {

  turbo::Utf16Generator generator{ 1, 0};
  for(size_t trial = 0; trial < 10; trial++) {
    auto utf16{generator.generate(128)};
    const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
    const size_t len = utf16.size();

    for (char16_t wrong_value = 0xdc00; wrong_value <= 0xdfff; wrong_value++) {
      for (size_t i=0; i < utf16.size(); i++) {
        const char16_t old = utf16[i];
        utf16[i] = wrong_value;

        REQUIRE_FALSE(turbo::validate_utf16le(buf, len));

        utf16[i] = old;
      }
    }
  }
}
#endif

/*
 RFC-2781:

 3) [..] if W2 is not between 0xDC00 and 0xDFFF, the sequence is in error.
    Terminate.
*/
#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else
TEST_CASE("validate_utf16le__returns_false_when_input_has_wrong_second_word_value") {

  turbo::Utf16Generator generator{ 1, 0};
  auto utf16{generator.generate(128)};
  const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
  const size_t len = utf16.size();
  const std::array<char16_t, 5> sample_wrong_second_word{
    0x0000, 0x1000, 0xdbff, 0xe000, 0xffff
  };
  const char16_t valid_surrogate_W1 = 0xd800;
  for (char16_t W2: sample_wrong_second_word) {
    for (size_t i=0; i < utf16.size() - 1; i++) {
      const char16_t old_W1 = utf16[i + 0];
      const char16_t old_W2 = utf16[i + 1];

      utf16[i + 0] = valid_surrogate_W1;
      utf16[i + 1] = W2;
      REQUIRE_FALSE(turbo::validate_utf16le(buf, len));

      utf16[i + 0] = old_W1;
      utf16[i + 1] = old_W2;
    }
  }
}
#endif

/*
 RFC-2781:

 3) If there is no W2 (that is, the sequence ends with W1) [...]
    the sequence is in error. Terminate.
*/
#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else
TEST_CASE("validate_utf16le__returns_false_when_input_is_truncated") {
  const char16_t valid_surrogate_W1 = 0xd800;

  turbo::Utf16Generator generator{ 1, 0};
  for (size_t size = 1; size < 128; size++) {
    auto utf16{generator.generate(128)};
    const char16_t*  buf = reinterpret_cast<const char16_t*>(utf16.data());
    const size_t len = utf16.size();

    utf16[size - 1] = valid_surrogate_W1;

    REQUIRE_FALSE(turbo::validate_utf16le(buf, len));
  }
}
#endif

#if TURBO_IS_BIG_ENDIAN
//t odo: port this test for big-endian platforms.
#else
TEST_CASE("validate_utf16le__extensive_tests") {
  const std::string path{"validate_utf16_testcases.txt"};
  std::ifstream file{path};
  if (not file) {
    printf("File '%s' cannot be open, skipping test\n", path.c_str());
    return;
  }

  constexpr uint16_t V = 0xfaea;
  constexpr uint16_t L = 0xd852;
  constexpr uint16_t H = 0xde12;

  constexpr size_t len = 32;
  char16_t buf[len];

  long lineno = 0;
  while (file) {
    std::string line;
    std::getline(file, line);
    lineno += 1;
    if (line.empty() or line[0] == '#')
      continue;

    // format: [TF][VLH]{16}
    bool valid = false;
    switch (line[0]) {
      case 'T':
        valid = true;
        break;
      case 'F':
        valid = false;
        break;
      default:
        throw std::invalid_argument("Error at line #" + std::to_string(lineno) +
                                    ": the first character must be either 'T' or 'F'");
    }

    // prepare input
    for (size_t i = 0; i < len; i++) {
      buf[i] = V;
    }

    for (size_t i = 1; i < line.size(); i++) {
      switch (line[i]) {
        case 'L':
          buf[i - 1] = L;
          break;
        case 'H':
          buf[i - 1] = H;
          break;
        case 'V':
          buf[i - 1] = V;
          break;
        default:
          throw std::invalid_argument("Error at line #" + std::to_string(lineno) +
                                      ": allowed characters are 'L', 'H' and 'V'");
      }
    }

    // check
    REQUIRE(turbo::validate_utf16le(buf, len) == valid);
  }
}
#endif

