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
// -----------------------------------------------------------------------------
// File: string_view.h
// -----------------------------------------------------------------------------
//
// This file contains the definition of the `std::string_view` class. A
// `string_view` points to a contiguous span of characters, often part or all of
// another `std::string`, double-quoted string literal, character array, or even
// another `string_view`.
//
// This `std::string_view` abstraction is designed to be a drop-in
// replacement for the C++17 `std::string_view` abstraction.
#ifndef TURBO_STRINGS_STRING_VIEW_H_
#define TURBO_STRINGS_STRING_VIEW_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/nullability.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/throw_delegate.h>
#include <turbo/base/macros.h>
#include <turbo/base/optimization.h>
#include <turbo/base/port.h>


#include <string_view>  // IWYU pragma: export

namespace turbo {
    TURBO_NAMESPACE_BEGIN
//using string_view = std::string_view;
    TURBO_NAMESPACE_END
}  // namespace turbo

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    // ClippedSubstr()
    //
    // Like `s.substr(pos, n)`, but clips `pos` to an upper bound of `s.size()`.
    // Provided because std::string_view::substr throws if `pos > size()`
    inline std::string_view ClippedSubstr(std::string_view s, size_t pos,
                                          size_t n = std::string_view::npos) {
        pos = (std::min)(pos, static_cast<size_t>(s.size()));
        return s.substr(pos, n);
    }

    // NullSafeStringView()
    //
    // Creates an `std::string_view` from a pointer `p` even if it's null-valued.
    // This function should be used where an `std::string_view` can be created from
    // a possibly-null pointer.
    constexpr std::string_view NullSafeStringView(turbo::Nullable<const char *> p) {
        return p ? std::string_view(p) : std::string_view();
    }

    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_STRING_VIEW_H_
