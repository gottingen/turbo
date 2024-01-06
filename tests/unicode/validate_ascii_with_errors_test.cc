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

TEST_CASE("no_error_ASCII") {

    turbo::Utf8Generator generator{1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        const auto ascii{generator.generate(512)};

        turbo::UnicodeResult res = turbo::validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

        REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
        REQUIRE_EQ(res.count, ascii.size());
    }
}

TEST_CASE("error_ASCII") {

    turbo::Utf8Generator generator{1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        auto ascii{generator.generate(512)};

        for (int i = 0; i < ascii.size(); i++) {
            ascii[i] += 0b10000000;

            turbo::UnicodeResult res = turbo::validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

            REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_LARGE);
            REQUIRE_EQ(res.count, i);

            ascii[i] -= 0b10000000;
        }
    }
}

