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

int main(int argc, char *argv[]) {
    turbo::cli::App cli_global{"Demo app"};
    auto &cli_sub = *cli_global.add_subcommand("sub", "Some subcommand");
    std::string sub_arg;
    cli_sub.add_option("sub_arg", sub_arg, "Argument for subcommand")->required();
    TURBO_APP_PARSE(cli_global, argc, argv);
    if(cli_sub) {
        std::cout << "Got: " << sub_arg << '\n';
    }
    return 0;
}
