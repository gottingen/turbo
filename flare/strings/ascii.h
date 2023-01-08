
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_STERINGS_ASCII_H_
#define FLARE_STERINGS_ASCII_H_

#include "flare/base/profile.h"
#include <cstdint>

namespace flare {

    enum class character_properties : uint32_t {
        eNone = 0x0,
        eControl = 0x0001,
        eSpace = 0x0002,
        ePunct = 0x0004,
        eDigit = 0x0008,
        eHexDigit = 0x0010,
        eAlpha = 0x0020,
        eLower = 0x0040,
        eUpper = 0x0080,
        eGraph = 0x0100,
        ePrint = 0x0200
    };

    constexpr character_properties operator&(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) &
                static_cast<uint32_t>(rhs)
        );
    }

    constexpr character_properties operator|(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) |
                static_cast<uint32_t>(rhs)
        );
    }

    constexpr character_properties operator~(character_properties lhs) {
        return static_cast<character_properties>(~static_cast<uint32_t>(lhs));
    }

    constexpr character_properties operator^(character_properties lhs, character_properties rhs) {
        return static_cast<character_properties>(
                static_cast<uint32_t>(lhs) ^
                static_cast<uint32_t>(rhs)
        );
    }

    constexpr character_properties &operator&=(character_properties &lhs, character_properties rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    constexpr character_properties &operator|=(character_properties &lhs, character_properties rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr character_properties &operator^=(character_properties &lhs, character_properties rhs) {
        lhs = lhs ^ rhs;
        return lhs;
    }

    class ascii {
    public:

        /**
         * @brief get all the properties of character
         * @param ch the input char
         * @return properties
         */
        static constexpr character_properties properties(unsigned char ch) noexcept;

        /**
         * @brief has_properties
         * @param ch the input char
         * @param properties properties
         * @return true or false
         */
        static constexpr bool has_properties(unsigned char ch, character_properties properties) noexcept;

        /**
         * @brief has_some_properties
         * @param ch the input char
         * @param properties  properties
         * @return true or false
         */
        static constexpr bool has_some_properties(unsigned char ch, character_properties properties) noexcept;

        /**
         * @brief is_graph
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_graph(unsigned char ch) noexcept;

        /**
         * @brief check if the given character is digit
         * @param ch the input char the character
         * @return true if the given character is digit
         */
        static constexpr bool is_digit(unsigned char ch) noexcept;

        /**
         * @brief is_white
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_white(unsigned char ch) noexcept;

        static constexpr bool is_blank(unsigned char ch) noexcept;

        /**
         * @brief is_ascii
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_ascii(unsigned char ch) noexcept;

        /**
         * @brief is_space
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_space(unsigned char ch) noexcept;

        /**
         * @brief is_hex_digit
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_hex_digit(unsigned char ch) noexcept;

        /**
         * @brief is_punct
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_punct(unsigned char ch) noexcept;

        /**
         * @brief is_print
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_print(unsigned char ch) noexcept;

        /**
         * @brief is_alpha
         * @param ch the input char the input char
         * @return true or false
         */
        static constexpr bool is_alpha(unsigned char ch) noexcept;

        /**
         * @brief is_control
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_control(unsigned char ch) noexcept;

        /**
         * @brief is_alpha_numeric
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_alpha_numeric(unsigned char ch) noexcept;

        /**
         * @brief is_lower
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_lower(unsigned char ch) noexcept;

        /**
         * @brief is_upper
         * @param ch the input char
         * @return true or false
         */
        static constexpr bool is_upper(unsigned char ch) noexcept;

        /**
         * @brief to_upper
         * @param ch the input char
         * @return true or false
         */
        static constexpr char to_upper(unsigned char ch) noexcept;

        /**
         * @brief to_lower
         * @param ch the input char
         * @return true or false
         */
        static constexpr char to_lower(unsigned char ch) noexcept;

    private:
        static const character_properties kCharacterProperties[128];
        static constexpr char kToUpper[256] = {
                '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
                '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
                '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
                '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
                '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
                '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
                '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
                '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
                '\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47',
                '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',
                '\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57',
                '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
                '\x60', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
                'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
                'X', 'Y', 'Z', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
                '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
                '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
                '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
                '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
                '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
                '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
                '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
                '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
                '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
                '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
                '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
                '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
                '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
                '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
                '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
                '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
        };
        static constexpr char kToLower[256] = {
                '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
                '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',
                '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
                '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',
                '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
                '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',
                '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
                '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',
                '\x40', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
                'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
                'x', 'y', 'z', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',
                '\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',
                '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
                '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
                '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',
                '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
                '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
                '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
                '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',
                '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
                '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',
                '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7',
                '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',
                '\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7',
                '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',
                '\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7',
                '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
                '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7',
                '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',
                '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7',
                '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff',
        };
    };

/// @brief FLARE_CONSTEXPR_FUNCTIONs

    constexpr bool ascii::is_white(unsigned char ch) noexcept {
        return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
    }

    constexpr bool ascii::is_blank(unsigned char ch) noexcept {
        return ch == ' ' || ch == '\t';
    }

    constexpr character_properties ascii::properties(unsigned char ch) noexcept {
        if (is_ascii(ch)) {
            return kCharacterProperties[ch];
        } else {
            return character_properties::eNone;
        }
    }

    constexpr bool ascii::is_ascii(unsigned char ch) noexcept {
        return (static_cast<uint32_t>(ch) & 0xFFFFFF80) == 0;
    }

    constexpr bool ascii::has_properties(unsigned char ch, character_properties prop) noexcept {
        return (properties(ch) & prop) == prop;
    }

    constexpr bool ascii::has_some_properties(unsigned char ch, character_properties prop) noexcept {
        return (properties(ch) & prop) != character_properties::eNone;
    }

    constexpr bool ascii::is_space(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eSpace);
    }

    constexpr bool ascii::is_print(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::ePrint);
    }

    constexpr bool ascii::is_graph(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eGraph);
    }

    constexpr bool ascii::is_digit(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eDigit);
    }

    constexpr bool ascii::is_hex_digit(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eHexDigit);
    }

    constexpr bool ascii::is_punct(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::ePunct);
    }

    constexpr bool ascii::is_alpha(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eAlpha);
    }

    constexpr bool ascii::is_control(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eControl);
    }

    constexpr bool ascii::is_alpha_numeric(unsigned char ch) noexcept {
        return has_some_properties(ch, character_properties::eAlpha | character_properties::eDigit);
    }

    constexpr bool ascii::is_lower(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eLower);
    }

    constexpr bool ascii::is_upper(unsigned char ch) noexcept {
        return has_properties(ch, character_properties::eUpper);
    }

    constexpr char ascii::to_lower(unsigned char ch) noexcept {
        return kToLower[ch];
    }

    constexpr char ascii::to_upper(unsigned char ch) noexcept {
        return kToUpper[ch];
    }

}  // namespace flare

#endif // FLARE_BASE_STERING_ASCII_H_
