// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
// Created by jeff on 24-4-26.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <turbo/testing/doctest.h>
#include <turbo/bitmap/bitmap.h>

TEST_CASE("SaveLoadTest") {
    turbo::Roaring r1;
    r1.addRange(0, 1000000);
    std::vector<char> rb;
    bool portable = false;
    turbo::save_bitmap(r1, rb, portable);
    turbo::Roaring r2;
    turbo::load_bitmap(rb, r2, portable);
    REQUIRE_EQ(r1, r2);

    portable = true;
    turbo::save_bitmap(r1, rb, portable);
    turbo::load_bitmap(rb, r2, portable);
    REQUIRE_EQ(r1, r2);
}

TEST_CASE("LoadBadDataTest") {
    turbo::Roaring r1;
    std::vector<char> rb;
    rb.resize(100, '0');
    bool portable = false;
    REQUIRE_FALSE(turbo::load_bitmap(rb, r1, portable));
}