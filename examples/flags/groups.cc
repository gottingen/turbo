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
#include <CLI/Timer.hpp>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    turbo::cli::AutoTimer give_me_a_name("This is a timer");

    turbo::cli::App app("K3Pi goofit fitter");

    std::string file;
    turbo::cli::Option *opt = app.add_option("-f,--file,file", file, "File name")->required()->group("Important");

    int count{0};
    turbo::cli::Option *copt = app.add_flag("-c,--count", count, "Counter")->required()->group("Important");

    double value{0.0};  // = 3.14;
    app.add_option("-d,--double", value, "Some Value")->group("Other");

    try {
        app.parse(argc, argv);
    } catch(const turbo::cli::ParseError &e) {
        return app.exit(e);
    }

    std::cout << "Working on file: " << file << ", direct count: " << app.count("--file")
              << ", opt count: " << opt->count() << '\n';
    std::cout << "Working on count: " << count << ", direct count: " << app.count("--count")
              << ", opt count: " << copt->count() << '\n';
    std::cout << "Some value: " << value << '\n';

    return 0;
}
