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
#include <sstream>

// example file to demonstrate a custom lexical cast function
namespace hala {
    template<class T = int>
    struct Values {
        T a;
        T b;
        T c;
    };

// in C++20 this is constructible from a double due to the new aggregate initialization in C++20.
    using DoubleValues = Values<double>;

// the lexical cast operator should be in the same namespace as the type for ADL to work properly
    bool lexical_cast(const std::string &input, Values<double> & /*v*/) {
        std::cout << "called correct lexical_cast function ! val: " << input << '\n';
        return true;
    }

    DoubleValues doubles;

    void argparse(turbo::cli::Option_group *group) { group->add_option("--dv", doubles)->default_str("0"); }
}  // namespace hala
int main(int argc, char **argv) {
    turbo::cli::App app;

    hala::argparse(app.add_option_group("param"));
    TURBO_APP_PARSE(app, argc, argv);
    return 0;
}
