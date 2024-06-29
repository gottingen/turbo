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
#include <memory>

class MyFormatter : public turbo::cli::Formatter {
  public:
    MyFormatter() : Formatter() {}
    std::string make_option_opts(const turbo::cli::Option *) const override { return " OPTION"; }
};

int main(int argc, char **argv) {
    turbo::cli::App app;
    app.set_help_all_flag("--help-all", "Show all help");

    auto fmt = std::make_shared<MyFormatter>();
    fmt->column_width(15);
    app.formatter(fmt);

    app.add_flag("--flag", "This is a flag");

    auto *sub1 = app.add_subcommand("one", "Description One");
    sub1->add_flag("--oneflag", "Some flag");
    auto *sub2 = app.add_subcommand("two", "Description Two");
    sub2->add_flag("--twoflag", "Some other flag");

    TURBO_APP_PARSE(app, argc, argv);

    std::cout << "This app was meant to show off the formatter, run with -h" << '\n';

    return 0;
}
