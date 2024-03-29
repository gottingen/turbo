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

TEST_CASE("validate_utf32_with_errors__returns_success_for_valid_input") {

  turbo::Utf32Generator generator{};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf32{generator.generate(256)};

    turbo::UnicodeResult res = turbo::validate_utf32_with_errors(reinterpret_cast<const char32_t*>(utf32.data()), utf32.size());

    REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
    REQUIRE_EQ(res.count, utf32.size());
  }
}

TEST_CASE("validate_utf32_with_errors__returns_success_for_empty_string") {
  const char32_t* buf = (char32_t*)"";

  turbo::UnicodeResult res = turbo::validate_utf32_with_errors(buf, 0);

  REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
  REQUIRE_EQ(res.count, 0);
}

TEST_CASE("validate_utf32_with_errors__returns_error_when_input_in_forbidden_range") {

  turbo::Utf32Generator generator{};
  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (char32_t wrong_value = 0xd800; wrong_value <= 0xdfff; wrong_value++) {
      for (size_t i=0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        turbo::UnicodeResult res = turbo::validate_utf32_with_errors(buf, len);

        REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
        REQUIRE_EQ(res.count, i);

        utf32[i] = old;
      }
    }
  }
}

TEST_CASE("validate_utf32_with_errors__returns_error_when_input_too_large") {

  turbo::Utf32Generator generator{};

  turbo::uniform_int_distribution<uint32_t> bad_range{0x110000, 0xffffffff};
  turbo::BitGen gen;

  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (size_t r = 0; r < 1000; r++) {
      uint32_t wrong_value = bad_range(gen);
      for (size_t i = 0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        turbo::UnicodeResult res = turbo::validate_utf32_with_errors(buf, len);

        REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_LARGE);
        REQUIRE_EQ(res.count, i);

        utf32[i] = old;
      }
    }
  }
}

