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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/strings/inlined_string.h"
#include "turbo/format/str_format.h"

TEST(Format, InlinedString) {

    turbo::inlined_string s = turbo::Format(42);
    ASSERT_EQ(s, "42");
    std::string stdstr  = turbo::Format(42);
    ASSERT_EQ(stdstr, "42");
}

TEST(Format, range) {
    std::vector<int> array = {1, 2, 4};
    auto s = turbo::FormatRange("{}",array, ", ");
    ASSERT_EQ(s, "1, 2, 4");
    turbo::FormatRangeAppend(&s,"{}",array, ", ");
    ASSERT_EQ(s, "1, 2, 41, 2, 4");
}