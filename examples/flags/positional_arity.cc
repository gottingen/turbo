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

    turbo::cli::App app("test for positional arity");

    auto *numbers = app.add_option_group("numbers", "specify key numbers");
    auto *files = app.add_option_group("files", "specify files");
    int num1{-1}, num2{-1};
    numbers->add_option("num1", num1, "first number");
    numbers->add_option("num2", num2, "second number");
    std::string file1, file2;
    files->add_option("file1", file1, "first file")->required();
    files->add_option("file2", file2, "second file");
    // set a pre parse callback that turns the numbers group on or off depending on the number of arguments
    app.preparse_callback([numbers](std::size_t arity) {
        if(arity <= 2) {
            numbers->disabled();
        } else {
            numbers->disabled(false);
        }
    });

    TURBO_APP_PARSE(app, argc, argv);

    if(num1 != -1)
        std::cout << "Num1 = " << num1 << '\n';

    if(num2 != -1)
        std::cout << "Num2 = " << num2 << '\n';

    std::cout << "File 1 = " << file1 << '\n';
    if(!file2.empty()) {
        std::cout << "File 2 = " << file2 << '\n';
    }

    return 0;
}
