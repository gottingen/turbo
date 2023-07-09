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

TEST_CASE("Format -InlinedString") {

    turbo::inlined_string s = turbo::Format(42);
    CHECK_EQ(s, "42");
    std::string stdstr  = turbo::Format(42);
    CHECK_EQ(stdstr, "42");
}

TEST_CASE("Format -InlinedString") {
    std::vector<int> array = {1, 2, 4};
    auto s = turbo::FormatRange("{}",array, ", ");
    CHECK_EQ(s, "1, 2, 4");
    turbo::FormatRangeAppend(&s,"{}",array, ", ");
    CHECK_EQ(s, "1, 2, 41, 2, 4");
}