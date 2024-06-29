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

#include <turbo/flags/app.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <iostream>
#include <sstream>
#include <turbo/flags/declare.h>
#include <turbo/flags/reflection.h>

TURBO_DECLARE_FLAG(std::vector<std::string>, flags_file);

TURBO_FLAG(int, test_flag, 0, "test flag");
TURBO_FLAG(std::string, test_string, "", "test flag");

int main(int argc, char **argv) {
    turbo::cli::App app;
    app.add_option("--test", FLAGS_test_flag, "test flag")->default_val(12);
    app.add_option("--str", FLAGS_test_string, "test flag")->default_val("asd");
    app.add_option("--config", FLAGS_flags_file, "test flag")->default_val(std::vector<std::string>{"a", "b", "c"});
    TURBO_APP_PARSE(app, argc, argv);
    std::cout << "test flag: " << turbo::get_flag(FLAGS_test_flag) << std::endl;
    std::cout << "test string: " << turbo::get_flag(FLAGS_test_string) << std::endl;
    for (auto &item : turbo::get_flag(FLAGS_flags_file)) {
        std::cout << "flags_file: " << item << std::endl;
    }
    return 0;
}
