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
//

#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "turbo/strings/inlined_string.h"
#include "turbo/format/format.h"
#include "turbo/format/print.h"
#include <memory>

TEST_CASE("format -InlinedString") {

    turbo::inlined_string s = turbo::format(42);
    CHECK_EQ(s, "42");
    std::string stdstr  = turbo::format(42);
    CHECK_EQ(stdstr, "42");
}

TEST_CASE("format -InlinedString") {
    std::vector<int> array = {1, 2, 4};
    auto s = turbo::format_range("{}",array, ", ");
    CHECK_EQ(s, "1, 2, 4");
    turbo::format_range_append(&s,"{}",array, ", ");
    CHECK_EQ(s, "1, 2, 41, 2, 4");
}

enum TestE {
    A, B, C
};
TEST_CASE("format -smart pointer") {
    auto e = TestE::A;
    auto s = turbo::format("{}", turbo::underlying(e));
    turbo::println("{}", s);
}