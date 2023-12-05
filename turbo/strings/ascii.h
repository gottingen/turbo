//
// Copyright 2022 The Turbo Authors.
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
// File: ascii.h
// -----------------------------------------------------------------------------
//
// This package contains functions operating on characters and strings
// restricted to standard ASCII. These include character classification
// functions analogous to those found in the ANSI C Standard Library <ctype.h>
// header file.
//
// C++ implementations provide <ctype.h> functionality based on their
// C environment locale. In general, reliance on such a locale is not ideal, as
// the locale standard is problematic (and may not return invariant information
// for the same character set, for example). These `ascii_*()` functions are
// hard-wired for standard ASCII, much faster, and guaranteed to behave
// consistently.  They will never be overloaded, nor will their function
// signature change.
//
// `ascii_isalnum()`, `ascii_isalpha()`, `ascii_isascii()`, `ascii_isblank()`,
// `ascii_iscntrl()`, `ascii_isdigit()`, `ascii_isgraph()`, `ascii_islower()`,
// `ascii_isprint()`, `ascii_ispunct()`, `ascii_isspace()`, `ascii_isupper()`,
// `ascii_isxdigit()`
//   Analogous to the <ctype.h> functions with similar names, these
//   functions take an unsigned char and return a bool, based on whether the
//   character matches the condition specified.
//
//   If the input character has a numerical value greater than 127, these
//   functions return `false`.
//
// `ascii_tolower()`, `ascii_toupper()`
//   Analogous to the <ctype.h> functions with similar names, these functions
//   take an unsigned char and return a char.
//
//   If the input character is not an ASCII {lower,upper}-case letter (including
//   numerical values greater than 127) then the functions return the same value
//   as the input character.

#ifndef TURBO_STRINGS_ASCII_H_
#define TURBO_STRINGS_ASCII_H_

#include <algorithm>
#include <string>

#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"
#include "turbo/meta/type_traits.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace ascii_internal {

        // Declaration for an array of bitfields holding character information.
        TURBO_DLL extern const unsigned char kPropertyBits[256];

        // Declaration for the array of characters to upper-case characters.
        TURBO_DLL extern const char kToUpper[256];

        // Declaration for the array of characters to lower-case characters.
        TURBO_DLL extern const char kToLower[256];

    }  // namespace ascii_internal

    /*
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is an alphabetic character.
     */
    inline bool ascii_isalpha(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x01) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is an alphanumeric character.
     */
    inline bool ascii_isalnum(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x04) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is a whitespace character (space,
     * tab, vertical tab, formfeed, linefeed, or carriage return).
     */
    inline bool ascii_isspace(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x08) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is a punctuation character.
     */
    inline bool ascii_ispunct(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x10) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is a blank character (tab or space).
     */
    inline bool ascii_isblank(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x20) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is a control character.
     */
    inline bool ascii_iscntrl(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x40) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character can be represented as a hexadecimal
     * digit character (i.e. {0-9} or {A-F}).
     */
    inline bool ascii_isxdigit(unsigned char c) {
        return (ascii_internal::kPropertyBits[c] & 0x80) != 0;
    }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character can be represented as a decimal
     * digit character (i.e. {0-9}).
     */
    inline bool ascii_isdigit(unsigned char c) { return c >= '0' && c <= '9'; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is printable, including spaces.
     */
    inline bool ascii_isprint(unsigned char c) { return c >= 32 && c < 127; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character has a graphical representation.
     */
    inline bool ascii_isgraph(unsigned char c) { return c > 32 && c < 127; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is uppercase.
     */
    inline bool ascii_isupper(unsigned char c) { return c >= 'A' && c <= 'Z'; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is lowercase.
     */
    inline bool ascii_islower(unsigned char c) { return c >= 'a' && c <= 'z'; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Determines whether the given character is ASCII.
     */
    inline bool ascii_isascii(unsigned char c) { return c < 128; }

    /**
     * @ingroup turbo_strings_ascii
     * @brief Returns the ASCII character, converting to lower-case if upper-case is
     * passed. Note that characters values > 127 are simply returned.
     */
    inline char ascii_tolower(unsigned char c) {
        return ascii_internal::kToLower[c];
    }


    /**
     * @ingroup turbo_strings_ascii
     * @brief Returns the ASCII character, converting to upper-case if lower-case is
     * passed. Note that characters values > 127 are simply returned.
     */
    inline char ascii_toupper(unsigned char c) {
        return ascii_internal::kToUpper[c];
    }


    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_ASCII_H_
