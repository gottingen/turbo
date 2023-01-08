
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: escaping.h
// -----------------------------------------------------------------------------
//
// This header file contains string utilities involved in escaping and
// unescaping strings in various ways.

#ifndef FLARE_STRINGS_ESCAPING_H_
#define FLARE_STRINGS_ESCAPING_H_

#include <cstddef>
#include <string>
#include <vector>

#include "flare/base/profile.h"
#include "flare/strings/ascii.h"
#include "flare/strings/str_join.h"
#include <string_view>

namespace flare {


// unescape()
//
// Unescapes a `source` string and copies it into `dest`, rewriting C-style
// escape sequences (https://en.ccreference.com/w/cpp/language/escape) into
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
//   if (!flare::unescape(s, &unescaped_s) {
//     ...
//   }
//   EXPECT_EQ(unescaped_s, "foo\rbar\nbaz\t");
bool unescape(std::string_view source, std::string *dest, std::string *error);

// Overload of `unescape()` with no error reporting.
FLARE_FORCE_INLINE bool unescape(std::string_view source, std::string *dest) {
    return unescape(source, dest, nullptr);
}

// escape()
//
// Escapes a 'src' string using C-style escapes sequences
// (https://en.ccreference.com/w/cpp/language/escape), escaping other
// non-printable/non-whitespace bytes as octal sequences (e.g. "\377").
//
// Example:
//
//   std::string s = "foo\rbar\tbaz\010\011\012\013\014\x0d\n";
//   std::string escaped_s = flare::escape(s);
//   EXPECT_EQ(escaped_s, "foo\\rbar\\tbaz\\010\\t\\n\\013\\014\\r\\n");
std::string escape(std::string_view src);

// hex_escape()
//
// Escapes a 'src' string using C-style escape sequences, escaping
// other non-printable/non-whitespace bytes as hexadecimal sequences (e.g.
// "\xFF").
//
// Example:
//
//   std::string s = "foo\rbar\tbaz\010\011\012\013\014\x0d\n";
//   std::string escaped_s = flare::hex_escape(s);
//   EXPECT_EQ(escaped_s, "foo\\rbar\\tbaz\\x08\\t\\n\\x0b\\x0c\\r\\n");
std::string hex_escape(std::string_view src);

// utf8_safe_escape()
//
// Escapes a 'src' string using C-style escape sequences, escaping bytes as
// octal sequences, and passing through UTF-8 characters without conversion.
// I.e., when encountering any bytes with their high bit set, this function
// will not escape those values, whether or not they are valid UTF-8.
std::string utf8_safe_escape(std::string_view src);

// utf8_safe_hex_escape()
//
// Escapes a 'src' string using C-style escape sequences, escaping bytes as
// hexadecimal sequences, and passing through UTF-8 characters without
// conversion.
std::string utf8_safe_hex_escape(std::string_view src);

// base64_unescape()
//
// Converts a `src` string encoded in Base64 to its binary equivalent, writing
// it to a `dest` buffer, returning `true` on success. If `src` contains invalid
// characters, `dest` is cleared and returns `false`.
bool base64_unescape(std::string_view src, std::string *dest);

// web_safe_base64_unescape()
//
// Converts a `src` string encoded in Base64 to its binary equivalent, writing
// it to a `dest` buffer, but using '-' instead of '+', and '_' instead of '/'.
// If `src` contains invalid characters, `dest` is cleared and returns `false`.
bool web_safe_base64_unescape(std::string_view src, std::string *dest);

// base64_escape()
//
// Encodes a `src` string into a base64-encoded string, with padding characters.
// This function conforms with RFC 4648 section 4 (base64).
void base64_escape(std::string_view src, std::string *dest);

std::string base64_escape(std::string_view src);

// web_safe_base64_escape()
//
// Encodes a `src` string into a base64-like string, using '-' instead of '+'
// and '_' instead of '/', and without padding. This function conforms with RFC
// 4648 section 5 (base64url).
void web_safe_base64_escape(std::string_view src, std::string *dest);

std::string web_safe_base64_escape(std::string_view src);

// hex_string_to_bytes()
//
// Converts an ASCII hex string into bytes, returning binary data of length
// `from.size()/2`.
std::string hex_string_to_bytes(std::string_view from);

// bytes_to_hex_string()
//
// Converts binary data into an ASCII text string, returning a string of size
// `2*from.size()`.
std::string bytes_to_hex_string(std::string_view from);


}  // namespace flare

#endif  // FLARE_STRINGS_ESCAPING_H_
