//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Created by jeff on 24-6-5.
//

#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <gtest/gtest.h>

TURBO_FLAG(int, test_validate_flag, 0, "test validate flag").on_validate([](std::string_view intvalue, std::string* error) noexcept ->bool {
    int val;
    auto r = turbo::parse_flag(intvalue, &val, error);
    if (!r) {
        return false;
    }
    if (val < 0 || val > 100) {
        *error = "value must be in [0, 100]";
        return false;
    }
    return true;
}).on_update([]() noexcept {
    std::cout << "test_validate_flag updated" << std::endl;
});

TEST(FlagsValidateTest, ValidateFlag) {
    std::string error;
    auto cl = turbo::find_command_line_flag("test_validate_flag");
    EXPECT_TRUE(cl != nullptr);
    EXPECT_TRUE(cl->has_user_validator());
    EXPECT_TRUE(cl->user_validate("0", &error));
    EXPECT_TRUE(cl->user_validate("100", &error));
    EXPECT_FALSE(cl->user_validate("-1", &error));
    EXPECT_FALSE(cl->user_validate("101", &error));
    cl->parse_from("50", &error);
}