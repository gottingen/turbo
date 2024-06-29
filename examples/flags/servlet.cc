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

#include <turbo/flags/servlet.h>
#include <turbo/version.h>
#include <iostream>
#include <string>
#include <turbo/flags/declare.h>
#include <turbo/flags/parse.h>

TURBO_DECLARE_FLAG(std::vector<std::string>, flags_file);

TURBO_FLAG(int, gt_flag, 10, "test flag").on_validate(turbo::GeValidator<int, 5>::validate);

int main(int argc, char **argv) {
    auto &svt = turbo::Servlet::instance();
    svt.set_name("K3Pi")
        .set_version(TURBO_VERSION_STRING)
        .set_description("K3Pi goofit fitter");

    std::string file;
    turbo::cli::Option *opt = svt.run_app()->add_option("-f,--file,file", file, "File name");

    int count{0};
    turbo::cli::Option *copt = svt.run_app()->add_option("-c,--count", count, "Counter");

    svt.run_app()->add_option("--gt", FLAGS_gt_flag, "test flag");
    svt.run_app()->add_option_function<int>("--gtf", [](const int&v) {
        std::cout << "gtf" << std::endl;
        }, "test flag");
    svt.root().add_option_function<int>("--gtff", [](const int&v) {
        std::cout << "root gtf" << std::endl;
    }, "test flag");



    int v{0};
    turbo::cli::Option *flag = svt.run_app()->add_flag("--flag", v, "Some flag that can be passed multiple times");

    double value{0.0};  // = 3.14;
    svt.run_app()->add_option("-d,--double", value, "Some Value");
    turbo::setup_argv(argc, argv);

    TURBO_SERVLET_PARSE(argc, argv);

    std::cout << "Working on file: " << file << ", direct count: " << svt.run_app()->count("--file")
              << ", opt count: " << opt->count() << '\n';
    std::cout << "Working on count: " << count << ", direct count: " << svt.run_app()->count("--count")
              << ", opt count: " << copt->count() << '\n';
    std::cout << "Received flag: " << v << " (" << flag->count() << ") times\n";
    std::cout << "Some value: " << value << '\n';
    std::cout << "gt_flag: " << turbo::get_flag(FLAGS_gt_flag) << std::endl;
    for(auto &item : turbo::get_flag(FLAGS_flags_file)) {
        std::cout << "flags_file: " << item << std::endl;
    }

    return 0;
}
