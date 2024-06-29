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
#include <vector>

int main(int argc, char **argv) {

    turbo::cli::App app("load shapes");

    app.set_help_all_flag("--help-all");
    auto *circle = app.add_subcommand("circle", "draw a circle")->immediate_callback();
    double radius{0.0};
    int circle_counter{0};
    circle->callback([&radius, &circle_counter] {
        ++circle_counter;
        std::cout << "circle" << circle_counter << " with radius " << radius << '\n';
    });

    circle->add_option("radius", radius, "the radius of the circle")->required();

    auto *rect = app.add_subcommand("rectangle", "draw a rectangle")->immediate_callback();
    double edge1{0.0};
    double edge2{0.0};
    int rect_counter{0};
    rect->callback([&edge1, &edge2, &rect_counter] {
        ++rect_counter;
        if(edge2 == 0) {
            edge2 = edge1;
        }
        std::cout << "rectangle" << rect_counter << " with edges [" << edge1 << ',' << edge2 << "]\n";
        edge2 = 0;
    });

    rect->add_option("edge1", edge1, "the first edge length of the rectangle")->required();
    rect->add_option("edge2", edge2, "the second edge length of the rectangle");

    auto *tri = app.add_subcommand("triangle", "draw a rectangle")->immediate_callback();
    std::vector<double> sides;
    int tri_counter = 0;
    tri->callback([&sides, &tri_counter] {
        ++tri_counter;

        std::cout << "triangle" << tri_counter << " with sides [" << turbo::cli::detail::join(sides) << "]\n";
    });

    tri->add_option("sides", sides, "the side lengths of the triangle");

    TURBO_APP_PARSE(app, argc, argv);

    return 0;
}
