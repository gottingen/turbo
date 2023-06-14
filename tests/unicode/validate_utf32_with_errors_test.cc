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

#include "turbo/unicode/utf.h"

#include <array>
#include <algorithm>

#include "helpers/random_utf32.h"
#include <tests/unicode/helpers/test.h>
#include <fstream>
#include <iostream>
#include <memory>

TEST(validate_utf32_with_errors__returns_success_for_valid_input) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf32 generator{seed};
  for(size_t trial = 0; trial < 1000; trial++) {
    const auto utf32{generator.generate(256, seed)};

    turbo::result res = implementation.ValidateUtf32WithErrors(reinterpret_cast<const char32_t*>(utf32.data()), utf32.size());

    ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
    ASSERT_EQUAL(res.count, utf32.size());
  }
}

TEST(validate_utf32_with_errors__returns_success_for_empty_string) {
  const char32_t* buf = (char32_t*)"";

  turbo::result res = implementation.ValidateUtf32WithErrors(buf, 0);

  ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, 0);
}

TEST(validate_utf32_with_errors__returns_error_when_input_in_forbidden_range) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf32 generator{seed};
  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (char32_t wrong_value = 0xd800; wrong_value <= 0xdfff; wrong_value++) {
      for (size_t i=0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        turbo::result res = implementation.ValidateUtf32WithErrors(buf, len);

        ASSERT_EQUAL(res.error, turbo::error_code::SURROGATE);
        ASSERT_EQUAL(res.count, i);

        utf32[i] = old;
      }
    }
  }
}

TEST(validate_utf32_with_errors__returns_error_when_input_too_large) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf32 generator{seed};

  std::uniform_int_distribution<uint32_t> bad_range{0x110000, 0xffffffff};
  std::mt19937 gen{seed};

  for(size_t trial = 0; trial < 10; trial++) {
    auto utf32{generator.generate(128)};
    const char32_t*  buf = reinterpret_cast<const char32_t*>(utf32.data());
    const size_t len = utf32.size();

    for (size_t r = 0; r < 1000; r++) {
      uint32_t wrong_value = bad_range(gen);
      for (size_t i = 0; i < utf32.size(); i++) {
        const char32_t old = utf32[i];
        utf32[i] = wrong_value;

        turbo::result res = implementation.ValidateUtf32WithErrors(buf, len);

        ASSERT_EQUAL(res.error, turbo::error_code::TOO_LARGE);
        ASSERT_EQUAL(res.count, i);

        utf32[i] = old;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}
