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

#include <turbo/strings/str_split.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    namespace {

        // This GenericFind() template function encapsulates the finding algorithm
        // shared between the ByString and ByAnyChar delimiters. The FindPolicy
        // template parameter allows each delimiter to customize the actual find
        // function to use and the length of the found delimiter. For example, the
        // Literal delimiter will ultimately use std::string_view::find(), and the
        // AnyOf delimiter will use std::string_view::find_first_of().
        template<typename FindPolicy>
        std::string_view GenericFind(std::string_view text,
                                       std::string_view delimiter, size_t pos,
                                       FindPolicy find_policy) {
            if (delimiter.empty() && text.length() > 0) {
                // Special case for empty string delimiters: always return a zero-length
                // std::string_view referring to the item at position 1 past pos.
                return std::string_view(text.data() + pos + 1, 0);
            }
            size_t found_pos = std::string_view::npos;
            std::string_view found(text.data() + text.size(),
                                     0);  // By default, not found
            found_pos = find_policy.Find(text, delimiter, pos);
            if (found_pos != std::string_view::npos) {
                found = std::string_view(text.data() + found_pos,
                                           find_policy.Length(delimiter));
            }
            return found;
        }

        // Finds using std::string_view::find(), therefore the length of the found
        // delimiter is delimiter.length().
        struct LiteralPolicy {
            static size_t Find(std::string_view text, std::string_view delimiter,
                               size_t pos) {
                return text.find(delimiter, pos);
            }

            static size_t Length(std::string_view delimiter) {
                return delimiter.length();
            }
        };

        // Finds using std::string_view::find_first_of(), therefore the length of the
        // found delimiter is 1.
        struct AnyOfPolicy {
            static size_t Find(std::string_view text, std::string_view delimiter,
                               size_t pos) {
                return text.find_first_of(delimiter, pos);
            }

            static size_t Length(std::string_view /* delimiter */) { return 1; }
        };

    }  // namespace

    //
    // ByString
    //

    ByString::ByString(std::string_view sp) : delimiter_(sp) {}

    std::string_view ByString::Find(std::string_view text, size_t pos) const {
        if (delimiter_.length() == 1) {
            // Much faster to call find on a single character than on an
            // std::string_view.
            size_t found_pos = text.find(delimiter_[0], pos);
            if (found_pos == std::string_view::npos)
                return std::string_view(text.data() + text.size(), 0);
            return text.substr(found_pos, 1);
        }
        return GenericFind(text, delimiter_, pos, LiteralPolicy());
    }

    std::string_view ByAsciiWhitespace::Find(std::string_view text,
                                               size_t pos) const {
        return GenericFind(text, " \t\v\f\r\n", pos, AnyOfPolicy());
    }

    //
    // ByChar
    //

    std::string_view ByChar::Find(std::string_view text, size_t pos) const {
        size_t found_pos = text.find(c_, pos);
        if (found_pos == std::string_view::npos)
            return std::string_view(text.data() + text.size(), 0);
        return text.substr(found_pos, 1);
    }

    //
    // ByAnyChar
    //

    ByAnyChar::ByAnyChar(std::string_view sp) : delimiters_(sp) {}

    std::string_view ByAnyChar::Find(std::string_view text, size_t pos) const {
        return GenericFind(text, delimiters_, pos, AnyOfPolicy());
    }

    //
    // ByLength
    //
    ByLength::ByLength(ptrdiff_t length) : length_(length) {
        TURBO_RAW_CHECK(length > 0, "");
    }

    std::string_view ByLength::Find(std::string_view text, size_t pos) const {
        pos = std::min(pos, text.size());  // truncate `pos`
        std::string_view substr = text.substr(pos);
        // If the string is shorter than the chunk size we say we
        // "can't find the delimiter" so this will be the last chunk.
        if (substr.length() <= static_cast<size_t>(length_))
            return std::string_view(text.data() + text.size(), 0);

        return std::string_view(substr.data() + length_, 0);
    }

}  // namespace turbo
