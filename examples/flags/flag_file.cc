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

#include <turbo/flags/flag.h>
#include <turbo/flags/app.h>
#include <iostream>
#include <turbo/flags/declare.h>
#include <turbo/flags/reflection.h>

TURBO_DECLARE_FLAG(std::vector<std::string>, flags_file);
turbo::flat_hash_set<int> set{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

TURBO_FLAG(std::string, test_flag, "test", "test flag").on_validate(turbo::AllPassValidator<std::string>::validate);

TURBO_FLAG(int, gt_flag, 10, "test flag").on_validate(turbo::GeValidator<int, 5>::validate);

TURBO_FLAG(int, range_flag, 10, "test flag").on_validate(turbo::ClosedOpenInRangeValidator<int, 5,15>::validate);

TURBO_FLAG(int, inset_flag, 3, "test flag").on_validate(turbo::InSetValidator<int, set>::validate);

std::string_view prefix = "/opt/EA";
TURBO_FLAG(std::string, prefix_flag, "/opt/EA/inf", "test flag").on_validate(turbo::StartsWithValidator<prefix>::validate);

int main(int argc, char **argv) {
    turbo::setup_argv(argc, argv);
    turbo::load_flags({"conf.flags"});

    std::cout << "gt_flag: " << turbo::get_flag(FLAGS_gt_flag) << std::endl;

    turbo::set_flag(&FLAGS_gt_flag, 3);
    std::cout << "gt_flag: " << turbo::get_flag(FLAGS_gt_flag) << std::endl;

    auto *flag = turbo::find_command_line_flag("flags_file");

    flag->parse_from("con.flags,con1.flags", nullptr);
    for (auto &item : turbo::get_flag(FLAGS_flags_file)) {
        std::cout <<"flags_file: " << item << std::endl;

    }
    std::cout << "gt_flag: " <<FLAGS_gt_flag.name() << std::endl;

}