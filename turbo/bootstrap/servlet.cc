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

#include <turbo/bootstrap/servlet.h>
#include <turbo/flags/declare.h>
#include <turbo/log/flags.h>
#include <turbo/log/logging.h>
#include <turbo/strings/str_join.h>
#include <vector>
#include <string>

TURBO_DECLARE_FLAG(std::vector<std::string>, flags_file);
TURBO_DECLARE_FLAG(int, stderr_threshold);
TURBO_DECLARE_FLAG(int, min_log_level);
TURBO_DECLARE_FLAG(std::string, backtrace_log_at);
TURBO_DECLARE_FLAG(bool, log_with_prefix);
TURBO_DECLARE_FLAG(int, verbosity);
TURBO_DECLARE_FLAG(std::string, vlog_module);

namespace turbo {
    Servlet Servlet::instance_;

    static bool no_log{false};
    Servlet::Servlet() {
        setup();
    }

    void Servlet::setup() {
        run_app_ = app_.add_subcommand("run", "run Servlet");
        app_.add_option_function<std::vector<std::string>>("-c,--config", [this](const std::vector<std::string> &files) {
            bool has_default = !default_flags_files_.empty();
            if(!files.empty() && !files[0].empty()) {
                load_flags(files);
                if(has_default) {
                    std::cerr<<"default config file: ["<<str_join(default_flags_files_, ",")<<"] will be ignored"<<std::endl;
                }
                std::cerr<<"load flags from config file: ["<<str_join(files, ",")<<"]"<<std::endl;
                return ;
            }

            if(!default_flags_files_.empty()) {
                load_flags(default_flags_files_);
                std::cerr<<"load flags from default config file: ["<<str_join(default_flags_files_, ",")<<"]"<<std::endl;
                return;
            }
        }, "servlet config file, the config files can be a list of files"
           " separated by space, the later file will override the former file"
           "these file will load first, then the command line flags may override"
           "the config file flags")->force_callback();

        run_app_->add_option("--log_stderr", FLAGS_stderr_threshold, FLAGS_stderr_threshold.help());
        run_app_->add_option("--min_log_level", FLAGS_min_log_level, FLAGS_min_log_level.help());
        run_app_->add_option("--backtrace_log_at", FLAGS_backtrace_log_at, FLAGS_backtrace_log_at.help());
        run_app_->add_option_function<bool>("--log_with_prefix", [](const bool &flag) {
            turbo::set_flag(&FLAGS_log_with_prefix, flag);
        }, FLAGS_log_with_prefix.help());
        run_app_->add_option("--verbosity", FLAGS_verbosity, FLAGS_verbosity.help());
        run_app_->add_option("--vlog_module", FLAGS_vlog_module, FLAGS_vlog_module.help());
        run_app_->add_option("--log_type", FLAGS_log_type, FLAGS_log_type.help());
        run_app_->add_flag("--no_log", no_log, "disable log setup");
    }

    std::pair<bool,int>  Servlet::run(int argc, char **argv) {
        app_.require_subcommand(true);
        setup_argv(argc, argv);
        try {
            app_.parse(argc, argv);
        } catch (const turbo::cli::ParseError &e) {
            return {true,app_.exit(e)};
        }
        if(!no_log) {
            turbo::setup_log_by_flags();
        }
        launch_params_ = &get_argv();
        return {false,0};
    }

    Servlet &Servlet::set_version(const std::string &version) {
        app_.set_version_flag("--version", version);
        return *this;
    }

    Servlet &Servlet::set_description(const std::string &version) {
        app_.description(version);
        return *this;
    }

    Servlet &Servlet::set_name(const std::string &name) {
        app_.name(name);
        return *this;
    }

    const std::vector<std::string> *Servlet::launch_params() const {
        return launch_params_;
    }

    Servlet &Servlet::add_default_flags_file(const std::string &file) {
        if(file.empty()) {
            return *this;
        }
        default_flags_files_.push_back(file);
        return *this;
    }

    Servlet &Servlet::add_default_flags_files(const std::vector<std::string> &files) {
        for(const auto& file : files) {
            add_default_flags_file(file);
        }
        return *this;
    }

    Servlet &Servlet::clear_default_flags_files() {
        default_flags_files_.clear();
        return *this;
    }

    std::string Servlet::default_flags_file() const {
        if(default_flags_files_.empty()) {
            return "";
        }
        std::string  result;
        for(const auto& file : default_flags_files_) {
            result += file + " ";
        }
        return result;
    }

}  // namespace turbo
