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
// Created by jeff on 24-6-28.
//

#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <iostream>

turbo::flat_hash_set<int> set{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

TURBO_FLAG(std::string, test_flag, "test", "test flag").on_validate(turbo::AllPassValidator<std::string>::validate);

TURBO_FLAG(int, gt_flag, 10, "test flag").on_validate(turbo::GeValidator<int, 5>::validate);

TURBO_FLAG(int, inset_flag, 3, "test flag").on_validate(turbo::InSetValidator<int, set>::validate);

std::string_view prefix = "/opt/EA";
TURBO_FLAG(std::string, prefix_flag, "/opt/EA/inf", "test flag").on_validate(turbo::StartsWithValidator<prefix>::validate);

int main(int argc, char **argv) {
    std::cout << "test_flag: " << turbo::get_flag(FLAGS_test_flag) << std::endl;
    turbo::set_flag(&FLAGS_test_flag, "test2");
    std::cout << "test_flag: " << turbo::get_flag(FLAGS_test_flag) << std::endl;
    auto flag = turbo::find_command_line_flag("test_flag");
    if (flag) {
        std::cout << "flag: " << flag->name() << std::endl;
        if(flag->has_user_validator()) {
            std::cout << "flag has user validator" << std::endl;
            std::cout<<flag->user_validate("test3", nullptr)<<std::endl;
        }

    }

    auto gt_flag = turbo::find_command_line_flag("gt_flag");
    std::cout<<"this should be 0, "<<gt_flag->user_validate("4", nullptr)<<std::endl;
    std::cout<<"this should be 1, "<<gt_flag->user_validate("6", nullptr)<<std::endl;

    auto inset_flag = turbo::find_command_line_flag("inset_flag");
    std::cout<<"this should be 0, "<<inset_flag->user_validate("11", nullptr)<<std::endl;
    std::cout<<"this should be 1, "<<inset_flag->user_validate("7", nullptr)<<std::endl;

    auto prefix_flag = turbo::find_command_line_flag("prefix_flag");
    std::cout<<"this should be 0, "<<prefix_flag->user_validate("/opt/ea", nullptr)<<std::endl;
    std::cout<<"this should be 1, "<<prefix_flag->user_validate("/opt/EA/inf", nullptr)<<std::endl;

}