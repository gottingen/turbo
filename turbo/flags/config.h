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
#ifndef TURBO_FLAGS_CONFIG_H_
#define TURBO_FLAGS_CONFIG_H_

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>


#include "turbo/flags/app.h"
#include "turbo/flags/config_fwd.h"
#include "turbo/flags/string_tools.h"

namespace turbo::detail {

    std::string convert_arg_for_ini(const std::string &arg, char stringQuote = '"', char characterQuote = '\'');

    /// Comma separated join, adds quotes if needed
    std::string ini_join(const std::vector<std::string> &args,
                         char sepChar = ',',
                         char arrayStart = '[',
                         char arrayEnd = ']',
                         char stringQuote = '"',
                         char characterQuote = '\'');

    std::vector<std::string> generate_parents(const std::string &section, std::string &name, char parentSeparator);

    /// assuming non default segments do a check on the close and open of the segments in a configItem structure
    void
    checkParentSegments(std::vector<ConfigItem> &output, const std::string &currentSection, char parentSeparator);

}  // namespace turbo::detail

#endif  // TURBO_FLAGS_CONFIG_H_