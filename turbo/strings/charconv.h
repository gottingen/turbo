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

#include <system_error>  // NOLINT(build/c++11)

#include <turbo/base/config.h>
#include <turbo/base/nullability.h>

namespace turbo {

// Workalike compatibility version of std::chars_format from C++17.
//
// This is an bitfield enumerator which can be passed to turbo::from_chars to
// configure the string-to-float conversion.
    enum class chars_format {
        scientific = 1,
        fixed = 2,
        hex = 4,
        general = fixed | scientific,
    };

// The return result of a string-to-number conversion.
//
// `ec` will be set to `invalid_argument` if a well-formed number was not found
// at the start of the input range, `result_out_of_range` if a well-formed
// number was found, but it was out of the representable range of the requested
// type, or to std::errc() otherwise.
//
// If a well-formed number was found, `ptr` is set to one past the sequence of
// characters that were successfully parsed.  If none was found, `ptr` is set
// to the `first` argument to from_chars.
    struct from_chars_result {
        turbo::Nonnull<const char *> ptr;
        std::errc ec;
    };

// Workalike compatibility version of std::from_chars from C++17.  Currently
// this only supports the `double` and `float` types.
//
// This interface incorporates the proposed resolutions for library issues
// DR 3080 and DR 3081.  If these are adopted with different wording,
// Turbo's behavior will change to match the standard.  (The behavior most
// likely to change is for DR 3081, which says what `value` will be set to in
// the case of overflow and underflow.  Code that wants to avoid possible
// breaking changes in this area should not depend on `value` when the returned
// from_chars_result indicates a range error.)
//
// Searches the range [first, last) for the longest matching pattern beginning
// at `first` that represents a floating point number.  If one is found, store
// the result in `value`.
//
// The matching pattern format is almost the same as that of strtod(), except
// that (1) C locale is not respected, (2) an initial '+' character in the
// input range will never be matched, and (3) leading whitespaces are not
// ignored.
//
// If `fmt` is set, it must be one of the enumerator values of the chars_format.
// (This is despite the fact that chars_format is a bitmask type.)  If set to
// `scientific`, a matching number must contain an exponent.  If set to `fixed`,
// then an exponent will never match.  (For example, the string "1e5" will be
// parsed as "1".)  If set to `hex`, then a hexadecimal float is parsed in the
// format that strtod() accepts, except that a "0x" prefix is NOT matched.
// (In particular, in `hex` mode, the input "0xff" results in the largest
// matching pattern "0".)
    turbo::from_chars_result from_chars(turbo::Nonnull<const char *> first,
                                        turbo::Nonnull<const char *> last,
                                        double &value,  // NOLINT
                                        chars_format fmt = chars_format::general);

    turbo::from_chars_result from_chars(turbo::Nonnull<const char *> first,
                                        turbo::Nonnull<const char *> last,
                                        float &value,  // NOLINT
                                        chars_format fmt = chars_format::general);

// std::chars_format is specified as a bitmask type, which means the following
// operations must be provided:
    inline constexpr chars_format operator&(chars_format lhs, chars_format rhs) {
        return static_cast<chars_format>(static_cast<int>(lhs) &
                                         static_cast<int>(rhs));
    }

    inline constexpr chars_format operator|(chars_format lhs, chars_format rhs) {
        return static_cast<chars_format>(static_cast<int>(lhs) |
                                         static_cast<int>(rhs));
    }

    inline constexpr chars_format operator^(chars_format lhs, chars_format rhs) {
        return static_cast<chars_format>(static_cast<int>(lhs) ^
                                         static_cast<int>(rhs));
    }

    inline constexpr chars_format operator~(chars_format arg) {
        return static_cast<chars_format>(~static_cast<int>(arg));
    }

    inline chars_format &operator&=(chars_format &lhs, chars_format rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    inline chars_format &operator|=(chars_format &lhs, chars_format rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    inline chars_format &operator^=(chars_format &lhs, chars_format rhs) {
        lhs = lhs ^ rhs;
        return lhs;
    }

}  // namespace turbo
