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

    turbo::cli::App app("Validator checker");

    std::string file;
    app.add_option("-f,--file,file", file, "File name")->check(turbo::cli::ExistingFile);

    int count{0};
    app.add_option("-v,--value", count, "Value in range")->check(turbo::cli::Range(3, 6));
    TURBO_APP_PARSE(app, argc, argv);

    std::cout << "Try printing help or failing the validator" << '\n';

    return 0;
}
