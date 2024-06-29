//
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
//
// Created by jeff on 24-6-30.
//

#include <turbo/flags/servlet.h>
#include <turbo/flags/declare.h>
#include <vector>
#include <string>

TURBO_DECLARE_FLAG(std::vector<std::string>, flags_file);

namespace turbo {
    Servlet Servlet::instance_;
    Servlet::Servlet() {
        setup();
    }

    void Servlet::setup() {
        run_app_ = app_.add_subcommand("run", "run Servlet");
        app_.add_option_function<std::vector<std::string>>("-c,--config", [](const std::vector<std::string> &files){
            load_flags(files);
            },"servlet config file, the config files can be a list of files"
                                                         " separated by space, the later file will override the former file"
                                                         "these file will load first, then the command line flags may override"
                                                         "the config file flags");
        app_.require_subcommand(true);
    }

    void Servlet::run(int argc, char** argv) {
        setup_argv(argc, argv);
        try {
            app_.parse(argc, argv);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    Servlet& Servlet::set_version(const std::string& version) {
        app_.set_version_flag("--version", version);
        return *this;
    }

    Servlet& Servlet::set_description(const std::string& version) {
        app_.description(version);
        return *this;
    }

    Servlet& Servlet::set_name(const std::string& name) {
        app_.name(name);
        return *this;
    }

}  // namespace turbo
