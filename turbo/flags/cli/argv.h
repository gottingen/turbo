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
#include <vector>
#include <turbo/base/macros.h>

namespace turbo::cli {
namespace detail {
#ifdef _WIN32
/// Decode and return UTF-8 argv from GetCommandLineW.
std::vector<std::string> compute_win32_argv();
#endif
}  // namespace detail
}  // namespace turbo::cli

namespace turbo {

    void setup_argv(int argc, char** argv);

    const std::vector<std::string>& get_argv();

    void load_flags(const std::vector<std::string>& flags_files);
    void load_flags();

}  // namespace turbo