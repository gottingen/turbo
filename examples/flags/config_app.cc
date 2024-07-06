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

    turbo::cli::App app("configuration print example");

    app.add_flag("-p,--print", "Print configuration and exit")->configurable(false);  // NEW: print flag

    std::string file;
    turbo::cli::Option *opt = app.add_option("-f,--file,file", file, "File name")
                           ->capture_default_str()
                           ->run_callback_for_default();  // NEW: capture_default_str()

    int count{0};
    turbo::cli::Option *copt =
        app.add_option("-c,--count", count, "Counter")->capture_default_str();  // NEW: capture_default_str()

    int v{0};
    turbo::cli::Option *flag = app.add_flag("--flag", v, "Some flag that can be passed multiple times")
                            ->capture_default_str();  // NEW: capture_default_str()

    double value{0.0};                                                          // = 3.14;
    app.add_option("-d,--double", value, "Some Value")->capture_default_str();  // NEW: capture_default_str()

    app.get_config_formatter_base()->quoteCharacter('"', '"');

    TURBO_APP_PARSE(app, argc, argv);

    if(app.get_option("--print")->as<bool>()) {  // NEW: print configuration and exit
        std::cout << app.config_to_str(true, false);
        return 0;
    }

    std::cout << "Working on file: " << file << ", direct count: " << app.count("--file")
              << ", opt count: " << opt->count() << '\n';
    std::cout << "Working on count: " << count << ", direct count: " << app.count("--count")
              << ", opt count: " << copt->count() << '\n';
    std::cout << "Received flag: " << v << " (" << flag->count() << ") times\n";
    std::cout << "Some value: " << value << '\n';

    return 0;
}
