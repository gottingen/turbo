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
#include <iostream>
#include <utility>
#include <vector>

// This example shows the usage of the retired and deprecated option helper methods
int main(int argc, char **argv) {

    turbo::cli::App app("example for retired/deprecated options");
    std::vector<int> x;
    auto *opt1 = app.add_option("--retired_option2", x);

    std::pair<int, int> y;
    auto *opt2 = app.add_option("--deprecate", y);

    app.add_option("--not_deprecated", x);

    // specify that a non-existing option is retired
    turbo::cli::retire_option(app, "--retired_option");

    // specify that an existing option is retired and non-functional: this will replace the option with another that
    // behaves the same but does nothing
    turbo::cli::retire_option(app, opt1);

    // deprecate an existing option and specify the recommended replacement
    turbo::cli::deprecate_option(opt2, "--not_deprecated");

    TURBO_APP_PARSE(app, argc, argv);

    if(!x.empty()) {
        std::cout << "Retired option example: got --not_deprecated values:";
        for(auto &xval : x) {
            std::cout << xval << " ";
        }
        std::cout << '\n';
    } else if(app.count_all() == 1) {
        std::cout << "Retired option example: no arguments received\n";
    }
    return 0;
}
