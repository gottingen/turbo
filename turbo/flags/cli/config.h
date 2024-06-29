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

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <turbo/flags/cli/app.h>
#include <turbo/flags/cli/config_fwd.h>
#include <turbo/flags/cli/string_tools.h>
namespace turbo::cli {

namespace detail {

std::string convert_arg_for_ini(const std::string &arg,
                                char stringQuote = '"',
                                char literalQuote = '\'',
                                bool disable_multi_line = false);

/// Comma separated join, adds quotes if needed
std::string ini_join(const std::vector<std::string> &args,
                     char sepChar = ',',
                     char arrayStart = '[',
                     char arrayEnd = ']',
                     char stringQuote = '"',
                     char literalQuote = '\'');

void clean_name_string(std::string &name, const std::string &keyChars);

std::vector<std::string> generate_parents(const std::string &section, std::string &name, char parentSeparator);

/// assuming non default segments do a check on the close and open of the segments in a configItem structure
void checkParentSegments(std::vector<ConfigItem> &output, const std::string &currentSection, char parentSeparator);
}  // namespace detail

}  // namespace turbo::cli