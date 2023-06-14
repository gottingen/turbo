// Copyright 2023 The titan-search Authors.
// Copyright (c) Jeff.li
// Copyright (c) Johan Mabille, Sylvain Corlay, Wolf Vollprecht and Martin Renou
// Copyright (c) QuantStack
// Copyright (c) Serge Guelton
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
//


/*
 * Make sure the inclusion works correctly without _GNU_SOURCE
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "turbo/simd/simd.h"

#include "tests/doctest/doctest.h"

TEST_CASE("[GNU_SOURCE support]")
{

    SUBCASE("exp10") {
        CHECK_EQ(turbo::simd::exp10(0.), 1.);
        CHECK_EQ(turbo::simd::exp10(0.f), 1.f);
    }
}
