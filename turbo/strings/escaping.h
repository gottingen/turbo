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
// File: escaping.h
// -----------------------------------------------------------------------------
//
// This header file contains string utilities involved in escaping and
// unescaping strings in various ways.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <turbo/base/attributes.h>
#include <turbo/base/macros.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/str_join.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    // c_decode()
    //
    // Unescapes a `source` string and copies it into `dest`, rewriting C-style
    // escape sequences (https://en.cppreference.com/w/cpp/language/escape) into
    // their proper code point equivalents, returning `true` if successful.
    //
    // The following unescape sequences can be handled:
    //
    //   * ASCII escape sequences ('\n','\r','\\', etc.) to their ASCII equivalents
    //   * Octal escape sequences ('\nnn') to byte nnn. The unescaped value must
    //     resolve to a single byte or an error will occur. E.g. values greater than
    //     0xff will produce an error.
    //   * Hexadecimal escape sequences ('\xnn') to byte nn. While an arbitrary
    //     number of following digits are allowed, the unescaped value must resolve
    //     to a single byte or an error will occur. E.g. '\x0045' is equivalent to
    //     '\x45', but '\x1234' will produce an error.
    //   * Unicode escape sequences ('\unnnn' for exactly four hex digits or
    //     '\Unnnnnnnn' for exactly eight hex digits, which will be encoded in
    //     UTF-8. (E.g., `\u2019` unescapes to the three bytes 0xE2, 0x80, and
    //     0x99).
    //
    // If any errors are encountered, this function returns `false`, leaving the
    // `dest` output parameter in an unspecified state, and stores the first
    // encountered error in `error`. To disable error reporting, set `error` to
    // `nullptr` or use the overload with no error reporting below.
    //
    // Example:
    //
    //   std::string s = "foo\\rbar\\nbaz\\t";
    //   std::string unescaped_s;
    //   if (!turbo::c_decode(s, &unescaped_s)) {
    //     ...
    //   }
    //   EXPECT_EQ(unescaped_s, "foo\rbar\nbaz\t");
    bool c_decode(turbo::string_view source, turbo::Nonnull<std::string *> dest,
                   turbo::Nullable<std::string *> error);

    // Overload of `c_decode()` with no error reporting.
    inline bool c_decode(turbo::string_view source,
                          turbo::Nonnull<std::string *> dest) {
        return c_decode(source, dest, nullptr);
    }

    // c_encode()
    //
    // Escapes a 'src' string using C-style escapes sequences
    // (https://en.cppreference.com/w/cpp/language/escape), escaping other
    // non-printable/non-whitespace bytes as octal sequences (e.g. "\377").
    //
    // Example:
    //
    //   std::string s = "foo\rbar\tbaz\010\011\012\013\014\x0d\n";
    //   std::string escaped_s = turbo::c_encode(s);
    //   EXPECT_EQ(escaped_s, "foo\\rbar\\tbaz\\010\\t\\n\\013\\014\\r\\n");
    std::string c_encode(turbo::string_view src);

    // c_hex_encode()
    //
    // Escapes a 'src' string using C-style escape sequences, escaping
    // other non-printable/non-whitespace bytes as hexadecimal sequences (e.g.
    // "\xFF").
    //
    // Example:
    //
    //   std::string s = "foo\rbar\tbaz\010\011\012\013\014\x0d\n";
    //   std::string escaped_s = turbo::c_hex_encode(s);
    //   EXPECT_EQ(escaped_s, "foo\\rbar\\tbaz\\x08\\t\\n\\x0b\\x0c\\r\\n");
        std::string c_hex_encode(turbo::string_view src);

    // utf8_safe_encode()
    //
    // Escapes a 'src' string using C-style escape sequences, escaping bytes as
    // octal sequences, and passing through UTF-8 characters without conversion.
    // I.e., when encountering any bytes with their high bit set, this function
    // will not escape those values, whether or not they are valid UTF-8.
    std::string utf8_safe_encode(turbo::string_view src);

    // utf8_safe_hex_encode()
    //
    // Escapes a 'src' string using C-style escape sequences, escaping bytes as
    // hexadecimal sequences, and passing through UTF-8 characters without
    // conversion.
    std::string utf8_safe_hex_encode(turbo::string_view src);

    // base64_encode()
    //
    // Encodes a `src` string into a base64-encoded 'dest' string with padding
    // characters. This function conforms with RFC 4648 section 4 (base64) and RFC
    // 2045.
    void base64_encode(turbo::string_view src, turbo::Nonnull<std::string *> dest);

    std::string base64_encode(turbo::string_view src);

    // web_safe_base64_encode()
    //
    // Encodes a `src` string into a base64 string, like base64_encode() does, but
    // outputs '-' instead of '+' and '_' instead of '/', and does not pad 'dest'.
    // This function conforms with RFC 4648 section 5 (base64url).
    void web_safe_base64_encode(turbo::string_view src,
                             turbo::Nonnull<std::string *> dest);

    std::string web_safe_base64_encode(turbo::string_view src);

    // base64_decode()
    //
    // Converts a `src` string encoded in Base64 (RFC 4648 section 4) to its binary
    // equivalent, writing it to a `dest` buffer, returning `true` on success. If
    // `src` contains invalid characters, `dest` is cleared and returns `false`.
    // If padding is included (note that `base64_encode()` does produce it), it must
    // be correct. In the padding, '=' and '.' are treated identically.
    bool base64_decode(turbo::string_view src, turbo::Nonnull<std::string *> dest);

    // web_safe_base64_decode()
    //
    // Converts a `src` string encoded in "web safe" Base64 (RFC 4648 section 5) to
    // its binary equivalent, writing it to a `dest` buffer. If `src` contains
    // invalid characters, `dest` is cleared and returns `false`. If padding is
    // included (note that `web_safe_base64_encode()` does not produce it), it must be
    // correct. In the padding, '=' and '.' are treated identically.
    bool web_safe_base64_decode(turbo::string_view src,
                               turbo::Nonnull<std::string *> dest);

    // hex_string_to_bytes()
    //
    // Converts the hexadecimal encoded data in `hex` into raw bytes in the `bytes`
    // output string.  If `hex` does not consist of valid hexadecimal data, this
    // function returns false and leaves `bytes` in an unspecified state. Returns
    // true on success.
    TURBO_MUST_USE_RESULT bool hex_string_to_bytes(turbo::string_view hex,
                                                turbo::Nonnull<std::string *> bytes);

    // hex_string_to_bytes()
    //
    // Converts an ASCII hex string into bytes, returning binary data of length
    // `from.size()/2`. The input must be valid hexadecimal data, otherwise the
    // return value is unspecified.
    //TURBO_DEPRECATED("Use the hex_string_to_bytes() that returns a bool")

    //std::string hex_string_to_bytes(turbo::string_view from);

    // bytes_to_hex_string()
    //
    // Converts binary data into an ASCII text string, returning a string of size
    // `2*from.size()`.
    std::string bytes_to_hex_string(turbo::string_view from);

}  // namespace turbo
