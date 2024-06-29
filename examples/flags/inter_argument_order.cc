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
#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

int main(int argc, char **argv) {
    turbo::cli::App app{"An app to practice mixing unlimited arguments, but still recover the original order."};

    std::vector<int> foos;
    auto *foo = app.add_option("--foo,-f", foos, "Some unlimited argument");

    std::vector<int> bars;
    auto *bar = app.add_option("--bar", bars, "Some unlimited argument");

    app.add_flag("--z,--x", "Random other flags");

    // Standard parsing lines (copy and paste in, or use TURBO_APP_PARSE)
    try {
        app.parse(argc, argv);
    } catch(const turbo::cli::ParseError &e) {
        return app.exit(e);
    }

    // I prefer using the back and popping
    std::reverse(std::begin(foos), std::end(foos));
    std::reverse(std::begin(bars), std::end(bars));

    std::vector<std::pair<std::string, int>> keyval;
    for(auto *option : app.parse_order()) {
        if(option == foo) {
            keyval.emplace_back("foo", foos.back());
            foos.pop_back();
        }
        if(option == bar) {
            keyval.emplace_back("bar", bars.back());
            bars.pop_back();
        }
    }

    // Prove the vector is correct
    for(auto &pair : keyval) {
        std::cout << pair.first << " : " << pair.second << '\n';
    }
}
