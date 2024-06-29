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

int main(int argc, char **argv) {

    turbo::cli::App app("K3Pi goofit fitter");
    app.set_help_all_flag("--help-all", "Expand all help");
    app.add_flag("--random", "Some random flag");
    turbo::cli::App *start = app.add_subcommand("start", "A great subcommand");
    turbo::cli::App *stop = app.add_subcommand("stop", "Do you really want to stop?");
    app.require_subcommand();  // 1 or more

    std::string file;
    start->add_option("-f,--file", file, "File name");

    turbo::cli::Option *s = stop->add_flag("-c,--count", "Counter");

    TURBO_APP_PARSE(app, argc, argv);

    std::cout << "Working on --file from start: " << file << '\n';
    std::cout << "Working on --count from stop: " << s->count() << ", direct count: " << stop->count("--count") << '\n';
    std::cout << "Count of --random flag: " << app.count("--random") << '\n';
    for(auto *subcom : app.get_subcommands())
        std::cout << "Subcommand: " << subcom->get_name() << '\n';

    return 0;
}
