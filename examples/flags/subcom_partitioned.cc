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
#include <memory>
#include <string>

int main(int argc, char **argv) {
    turbo::cli::AutoTimer give_me_a_name("This is a timer");

    turbo::cli::App app("K3Pi goofit fitter");

    turbo::cli::App_p impOpt = std::make_shared<turbo::cli::App>("Important");
    std::string file;
    turbo::cli::Option *opt = impOpt->add_option("-f,--file,file", file, "File name")->required();

    int count{0};
    turbo::cli::Option *copt = impOpt->add_flag("-c,--count", count, "Counter")->required();

    turbo::cli::App_p otherOpt = std::make_shared<turbo::cli::App>("Other");
    double value{0.0};  // = 3.14;
    otherOpt->add_option("-d,--double", value, "Some Value");

    // add the subapps to the main one
    app.add_subcommand(impOpt);
    app.add_subcommand(otherOpt);

    try {
        app.parse(argc, argv);
    } catch(const turbo::cli::ParseError &e) {
        return app.exit(e);
    }

    std::cout << "Working on file: " << file << ", direct count: " << impOpt->count("--file")
              << ", opt count: " << opt->count() << '\n';
    std::cout << "Working on count: " << count << ", direct count: " << impOpt->count("--count")
              << ", opt count: " << copt->count() << '\n';
    std::cout << "Some value: " << value << '\n';

    return 0;
}
