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
#include <string>

/** This example demonstrates the use of `prefix_command` on a subcommand
to capture all subsequent arguments along with an alias to make it appear as a regular options.

All the values after the "sub" or "--sub" are available in the remaining() method.
*/
int main(int argc, const char *argv[]) {

    int value{0};
    turbo::cli::App app{"Test App"};
    app.add_option("-v", value, "value");

    auto *subcom = app.add_subcommand("sub", "")->prefix_command();
    subcom->alias("--sub");
    TURBO_APP_PARSE(app, argc, argv);

    std::cout << "value=" << value << '\n';
    std::cout << "after Args:";
    for(const auto &aarg : subcom->remaining()) {
        std::cout << aarg << " ";
    }
    std::cout << '\n';
}
