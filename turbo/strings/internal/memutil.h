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

#pragma once

#include <cstddef>
#include <cstring>

#include <turbo/base/port.h>  // disable some warnings on Windows
#include <turbo/strings/ascii.h>  // for turbo::ascii_tolower

namespace turbo::strings_internal {

    // Performs a byte-by-byte comparison of `len` bytes of the strings `s1` and
    // `s2`, ignoring the case of the characters. It returns an integer less than,
    // equal to, or greater than zero if `s1` is found, respectively, to be less
    // than, to match, or be greater than `s2`.
    int memcasecmp(const char *s1, const char *s2, size_t len);

}  // namespace turbo::strings_internal