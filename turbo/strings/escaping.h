//
// Copyright 2020 The Turbo Authors.
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
// -----------------------------------------------------------------------------
// File: escaping.h
// -----------------------------------------------------------------------------
//
// This header file contains string utilities involved in escaping and
// unescaping strings in various ways.

#ifndef TURBO_STRINGS_ESCAPING_H_
#define TURBO_STRINGS_ESCAPING_H_

#include <cstddef>
#include <string>
#include <vector>

#include "turbo/platform/port.h"
#include "turbo/strings/ascii.h"
#include "turbo/strings/string_view.h"
#include "turbo/strings/internal/escaping.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN

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
    //   if (!turbo::c_decode(s, &unescaped_s) {
    //     ...
    //   }
    //   EXPECT_EQ(unescaped_s, "foo\rbar\nbaz\t");
    bool c_decode(std::string_view source, std::string *dest, std::string *error);

    // Overload of `c_decode()` with no error reporting.
    inline bool c_decode(std::string_view source, std::string *dest) {
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
    std::string c_encode(std::string_view src);

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
    std::string c_hex_encode(std::string_view src);

    // utf8_safe_c_encode()
    //
    // Escapes a 'src' string using C-style escape sequences, escaping bytes as
    // octal sequences, and passing through UTF-8 characters without conversion.
    // I.e., when encountering any bytes with their high bit set, this function
    // will not escape those values, whether or not they are valid UTF-8.
    std::string utf8_safe_c_encode(std::string_view src);

    // utf8_safe_c_hex_encode()
    //
    // Escapes a 'src' string using C-style escape sequences, escaping bytes as
    // hexadecimal sequences, and passing through UTF-8 characters without
    // conversion.
    std::string utf8_safe_c_hex_encode(std::string_view src);

    // base64_encode()
    //
    // Encodes a `src` string into a base64-encoded 'dest' string with padding
    // characters. This function conforms with RFC 4648 section 4 (base64) and RFC
    // 2045. See also CalculateBase64EscapedLen().
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    base64_encode(std::string_view src, String *dest);

    template<typename String = std::string>
    typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    base64_encode(std::string_view src) {
        String dest;
        strings_internal::base64_encode_internal(
                reinterpret_cast<const unsigned char *>(src.data()), src.size(), &dest,
                true, strings_internal::kBase64Chars);
        return dest;
    }

    // web_safe_base64_encode()
    //
    // Encodes a `src` string into a base64 string, like base64_encode() does, but
    // outputs '-' instead of '+' and '_' instead of '/', and does not pad 'dest'.
    // This function conforms with RFC 4648 section 5 (base64url).
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    web_safe_base64_encode(std::string_view src, String *dest);

    template<typename String = std::string>
    typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    web_safe_base64_encode(std::string_view src) {
        String dest;
        web_safe_base64_encode(src, &dest);
        return dest;
    }

    // base64_decode()
    //
    // Converts a `src` string encoded in Base64 (RFC 4648 section 4) to its binary
    // equivalent, writing it to a `dest` buffer, returning `true` on success. If
    // `src` contains invalid characters, `dest` is cleared and returns `false`.
    // If padding is included (note that `base64_encode()` does produce it), it must
    // be correct. In the padding, '=' and '.' are treated identically.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value, bool>::type
    base64_decode(std::string_view src, String *dest);

    // web_safe_base64_decode()
    //
    // Converts a `src` string encoded in "web safe" Base64 (RFC 4648 section 5) to
    // its binary equivalent, writing it to a `dest` buffer. If `src` contains
    // invalid characters, `dest` is cleared and returns `false`. If padding is
    // included (note that `web_safe_base64_encode()` does not produce it), it must be
    // correct. In the padding, '=' and '.' are treated identically.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value, bool>::type
    web_safe_base64_decode(std::string_view src, String *dest);

    // hex_to_bytes()
    //
    // Converts an ASCII hex string into bytes, returning binary data of length
    // `from.size()/2`.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    hex_to_bytes(std::string_view from, String *dest);

    template<typename String = std::string>
    typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    hex_to_bytes(std::string_view from) {
        String result;
        hex_to_bytes(from, &result);
        return result;
    }

    // bytes_to_hex()
    //
    // Converts binary data into an ASCII text string, returning a string of size
    // `2*from.size()`.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    bytes_to_hex(std::string_view from, String *dest);

    template<typename String = std::string>
    typename std::enable_if<turbo::is_string_type<String>::value, String>::type
    bytes_to_hex(std::string_view from) {
        String result;
        bytes_to_hex(from, &result);
        return result;
    }

    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_ESCAPING_H_
