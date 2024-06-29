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
#include <turbo/base/macros.h>
#include <string>
#include <string_view>

namespace turbo::cli {


/// Convert a wide string to a narrow string.
std::string narrow(const std::wstring &str);
std::string narrow(const wchar_t *str);
std::string narrow(const wchar_t *str, std::size_t size);

/// Convert a narrow string to a wide string.
std::wstring widen(const std::string &str);
std::wstring widen(const char *str);
std::wstring widen(const char *str, std::size_t size);

std::string narrow(std::wstring_view str);
std::wstring widen(std::string_view str);

}  // namespace turbo::cli
