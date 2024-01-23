// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdlib.h>
#include "turbo/var/flag_variable.h"
#include "turbo/flags/reflection.h"

namespace turbo {

    FlagVariable::FlagVariable(const std::string &gflag_name) {
        expose(gflag_name);
    }

    FlagVariable::FlagVariable(const std::string &prefix,
                               const std::string &flag_name)
            : _flag_name(flag_name.data(), flag_name.size()) {
        expose_as(prefix, flag_name);
    }

    void FlagVariable::describe(std::ostream &os, bool quote_string) const {
        auto cmd_flag = find_command_line_flag(flag_name());
        if(cmd_flag) {
            if (quote_string && cmd_flag->is_of_type<std::string>()) {
                os << '"' << cmd_flag->current_value() << '"';
            } else {
                os << cmd_flag->current_value();
            }
        } else {
            if (quote_string) {
                os << '"';
            }
            os << "Unknown flag=" << flag_name();
            if (quote_string) {
                os << '"';
            }
        }
    }

    std::string FlagVariable::get_value() const {
        auto cmd_flag = find_command_line_flag(flag_name());
        std::string str;
        if (!cmd_flag) {
            return "Unknown gflag=" + flag_name();
        }
        return cmd_flag->current_value();
    }

    bool FlagVariable::set_value(const char *value) {
        auto cmd_flag = find_command_line_flag(flag_name());
        if (!cmd_flag) {
            return false;
        }
        std::string err;
        return cmd_flag->parse_from(value, &err);
    }

}  // namespace turbo
