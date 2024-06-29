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

    turbo::cli::App app("data output specification");
    app.set_help_all_flag("--help-all", "Expand all help");

    auto *format = app.add_option_group("output_format", "formatting type for output");
    auto *target = app.add_option_group("output target", "target location for the output");
    bool csv{false};
    bool human{false};
    bool binary{false};
    format->add_flag("--csv", csv, "specify the output in csv format");
    format->add_flag("--human", human, "specify the output in human readable text format");
    format->add_flag("--binary", binary, "specify the output in binary format");
    // require one of the options to be selected
    format->require_option(1);
    std::string fileLoc;
    std::string networkAddress;
    target->add_option("-o,--file", fileLoc, "specify the file location of the output");
    target->add_option("--address", networkAddress, "specify a network address to send the file");

    // require at most one of the target options
    target->require_option(0, 1);
    TURBO_APP_PARSE(app, argc, argv);

    std::string format_type = (csv) ? std::string("CSV") : ((human) ? "human readable" : "binary");
    std::cout << "Selected " << format_type << " format\n";
    if(!fileLoc.empty()) {
        std::cout << " sent to file " << fileLoc << '\n';
    } else if(!networkAddress.empty()) {
        std::cout << " sent over network to " << networkAddress << '\n';
    } else {
        std::cout << " sent to std::cout\n";
    }

    return 0;
}
