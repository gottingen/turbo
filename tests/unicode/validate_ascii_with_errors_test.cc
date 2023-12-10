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

#include <tests/unicode/helpers/test.h>
#include "turbo/random/random.h"
#include <fstream>
#include <iostream>
#include <memory>

TEST(no_error_ASCII) {
    uint32_t seed{1234};
    turbo::Utf8Generator generator{1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        const auto ascii{generator.generate(512)};

        turbo::result res = implementation.validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

        ASSERT_EQUAL(res.error, turbo::error_code::SUCCESS);
        ASSERT_EQUAL(res.count, ascii.size());
    }
}

TEST(error_ASCII) {
    uint32_t seed{1234};
    turbo::Utf8Generator generator{1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        auto ascii{generator.generate(512)};

        for (int i = 0; i < ascii.size(); i++) {
            ascii[i] += 0b10000000;

            turbo::result res = implementation.validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

            ASSERT_EQUAL(res.error, turbo::error_code::TOO_LARGE);
            ASSERT_EQUAL(res.count, i);

            ascii[i] -= 0b10000000;
        }
    }
}

int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}