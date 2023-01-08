
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "flare/strings/str_split.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>

#include "flare/log/logging.h"
#include "flare/strings/ascii.h"

namespace flare {


    namespace {

        // This generic_find() template function encapsulates the finding algorithm
        // shared between the by_string and by_any_char delimiters. The FindPolicy
        // template parameter allows each delimiter to customize the actual find
        // function to use and the length of the found delimiter. For example, the
        // Literal delimiter will ultimately use std::string_view::find(), and the
        // AnyOf delimiter will use std::string_view::find_first_of().
        template<typename FindPolicy>
        std::string_view generic_find(std::string_view text,
                                      std::string_view delimiter, size_t pos,
                                      FindPolicy find_policy) {
            if (delimiter.empty() && text.length() > 0) {
                // Special case for empty std::string delimiters: always return a zero-length
                // std::string_view referring to the item at position 1 past pos.
                return std::string_view(text.data() + pos + 1, 0);
            }
            size_t found_pos = std::string_view::npos;
            std::string_view found(text.data() + text.size(),
                                   0);  // By default, not found
            found_pos = find_policy.find(text, delimiter, pos);
            if (found_pos != std::string_view::npos) {
                found = std::string_view(text.data() + found_pos,
                                         find_policy.Length(delimiter));
            }
            return found;
        }

        // Finds using std::string_view::find(), therefore the length of the found
        // delimiter is delimiter.length().
        struct literal_policy {
            size_t find(std::string_view text, std::string_view delimiter, size_t pos) {
                return text.find(delimiter, pos);
            }

            size_t Length(std::string_view delimiter) { return delimiter.length(); }
        };

        // Finds using std::string_view::find_first_of(), therefore the length of the
        // found delimiter is 1.
        struct any_of_policy {
            size_t find(std::string_view text, std::string_view delimiter, size_t pos) {
                return text.find_first_of(delimiter, pos);
            }

            size_t Length(std::string_view /* delimiter */) { return 1; }
        };

    }  // namespace

    //
    // by_string
    //

    by_string::by_string(std::string_view sp) : _delimiter(sp) {}

    std::string_view by_string::find(std::string_view text, size_t pos) const {
        if (_delimiter.length() == 1) {
            // Much faster to call find on a single character than on an
            // std::string_view.
            size_t found_pos = text.find(_delimiter[0], pos);
            if (found_pos == std::string_view::npos)
                return std::string_view(text.data() + text.size(), 0);
            return text.substr(found_pos, 1);
        }
        return generic_find(text, _delimiter, pos, literal_policy());
    }

    //
    // by_char
    //

    std::string_view by_char::find(std::string_view text, size_t pos) const {
        size_t found_pos = text.find(c_, pos);
        if (found_pos == std::string_view::npos)
            return std::string_view(text.data() + text.size(), 0);
        return text.substr(found_pos, 1);
    }

    //
    // by_any_char
    //

    by_any_char::by_any_char(std::string_view sp) : _delimiters(sp) {}

    std::string_view by_any_char::find(std::string_view text, size_t pos) const {
        return generic_find(text, _delimiters, pos, any_of_policy());
    }

    //
    //  by_length
    //
    by_length::by_length(ptrdiff_t length) : _length(length) {
        FLARE_DCHECK(length > 0);
    }

    std::string_view by_length::find(std::string_view text,
                                     size_t pos) const {
        pos = std::min(pos, text.size());  // truncate `pos`
        std::string_view substr = text.substr(pos);
        // If the std::string is shorter than the chunk size we say we
        // "can't find the delimiter" so this will be the last chunk.
        if (substr.length() <= static_cast<size_t>(_length))
            return std::string_view(text.data() + text.size(), 0);

        return std::string_view(substr.data() + _length, 0);
    }


}  // namespace flare
