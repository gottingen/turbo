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
#include <vector>

int main(int argc, char **argv) {

    turbo::cli::App app{"App to demonstrate exclusionary option groups."};

    std::vector<int> range;
    app.add_option("--range,-R", range, "A range")->expected(-2);

    auto *ogroup = app.add_option_group("min_max_step", "set the min max and step");
    int min{0}, max{0}, step{1};
    ogroup->add_option("--min,-m", min, "The minimum")->required();
    ogroup->add_option("--max,-M", max, "The maximum")->required();
    ogroup->add_option("--step,-s", step, "The step")->capture_default_str();

    app.require_option(1);

    TURBO_APP_PARSE(app, argc, argv);

    if(!range.empty()) {
        if(range.size() == 2) {
            min = range[0];
            max = range[1];
        }
        if(range.size() >= 3) {
            step = range[0];
            min = range[1];
            max = range[2];
        }
    }
    std::cout << "range is [" << min << ':' << step << ':' << max << "]\n";
    return 0;
}
