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

#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <turbo/base/macros.h>

namespace turbo::cli::detail {
    // Returns false if not a short option. Otherwise, sets opt name and rest and returns true
    bool split_short(const std::string &current, std::string &name, std::string &rest);

    // Returns false if not a long option. Otherwise, sets opt name and other side of = and returns true
    bool split_long(const std::string &current, std::string &name, std::string &value);

    // Returns false if not a windows style option. Otherwise, sets opt name and value and returns true
    bool split_windows_style(const std::string &current, std::string &name, std::string &value);

    // Splits a string into multiple long and short names
    std::vector<std::string> split_names(std::string current);

    /// extract default flag values either {def} or starting with a !
    std::vector<std::pair<std::string, std::string>> get_default_flag_values(const std::string &str);

    /// Get a vector of short names, one of long names, and a single name
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
    get_names(const std::vector<std::string> &input);

}  // namespace turbo::cli::detail
