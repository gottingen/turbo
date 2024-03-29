// Copyright 2022 The Turbo Authors.
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

#include "turbo/memory/prefetch.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"

namespace {

    int number = 42;

    TEST_CASE("Prefetch") {
        SUBCASE("TemporalLocalityNone") {
            turbo::prefetch_nta(&number);
            CHECK_EQ(number, 42);
        }

        SUBCASE("TemporalLocalityLow") {
            turbo::prefetch_t2(&number);
            CHECK_EQ(number, 42);
        }

        SUBCASE("TemporalLocalityMedium") {
            turbo::prefetch_t1(&number);
            CHECK_EQ(number, 42);
        }

        SUBCASE("TemporalLocalityHigh") {
            turbo::prefetch_t0(&number);
            CHECK_EQ(number, 42);
        }
    }
}  // namespace
