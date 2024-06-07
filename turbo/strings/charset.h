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
// File: charset.h
// -----------------------------------------------------------------------------
//
// This file contains turbo::CharSet, a fast, bit-vector set of 8-bit unsigned
// characters.
//
// Instances can be initialized as constexpr constants. For example:
//
//   constexpr turbo::CharSet kJustX = turbo::CharSet::Char('x');
//   constexpr turbo::CharSet kMySymbols = turbo::CharSet("$@!");
//   constexpr turbo::CharSet kLetters = turbo::CharSet::Range('a', 'z');
//
// Multiple instances can be combined that still forms a constexpr expression.
// For example:
//
//   constexpr turbo::CharSet kLettersAndNumbers =
//       turbo::CharSet::Range('a', 'z') | turbo::CharSet::Range('0', '9');
//
// Several pre-defined character classes are available that mirror the methods
// from <cctype>. For example:
//
//   constexpr turbo::CharSet kLettersAndWhitespace =
//       turbo::CharSet::AsciiAlphabet() | turbo::CharSet::AsciiWhitespace();
//
// To check membership, use the .contains method, e.g.
//
//   turbo::CharSet hex_letters("abcdef");
//   hex_letters.contains('a');  // true
//   hex_letters.contains('g');  // false

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <turbo/base/macros.h>
#include <turbo/base/port.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    class CharSet {
    public:
        constexpr CharSet() : m_() {}

        // Initializes with a given string_view.
        constexpr explicit CharSet(std::string_view str) : m_() {
            for (char c: str) {
                SetChar(static_cast<unsigned char>(c));
            }
        }

        constexpr bool contains(char c) const {
            return ((m_[static_cast<unsigned char>(c) / 64] >>
                                                            (static_cast<unsigned char>(c) % 64)) &
                    0x1) == 0x1;
        }

        constexpr bool empty() const {
            for (uint64_t c: m_) {
                if (c != 0) return false;
            }
            return true;
        }

        // Containing only a single specified char.
        static constexpr CharSet Char(char x) {
            return CharSet(CharMaskForWord(x, 0), CharMaskForWord(x, 1),
                           CharMaskForWord(x, 2), CharMaskForWord(x, 3));
        }

        // Containing all the chars in the closed interval [lo,hi].
        static constexpr CharSet Range(char lo, char hi) {
            return CharSet(RangeForWord(lo, hi, 0), RangeForWord(lo, hi, 1),
                           RangeForWord(lo, hi, 2), RangeForWord(lo, hi, 3));
        }

        friend constexpr CharSet operator&(const CharSet &a, const CharSet &b) {
            return CharSet(a.m_[0] & b.m_[0], a.m_[1] & b.m_[1], a.m_[2] & b.m_[2],
                           a.m_[3] & b.m_[3]);
        }

        friend constexpr CharSet operator|(const CharSet &a, const CharSet &b) {
            return CharSet(a.m_[0] | b.m_[0], a.m_[1] | b.m_[1], a.m_[2] | b.m_[2],
                           a.m_[3] | b.m_[3]);
        }

        friend constexpr CharSet operator~(const CharSet &a) {
            return CharSet(~a.m_[0], ~a.m_[1], ~a.m_[2], ~a.m_[3]);
        }

        // Mirrors the char-classifying predicates in <cctype>.
        static constexpr CharSet AsciiUppercase() { return CharSet::Range('A', 'Z'); }

        static constexpr CharSet AsciiLowercase() { return CharSet::Range('a', 'z'); }

        static constexpr CharSet AsciiDigits() { return CharSet::Range('0', '9'); }

        static constexpr CharSet AsciiAlphabet() {
            return AsciiLowercase() | AsciiUppercase();
        }

        static constexpr CharSet AsciiAlphanumerics() {
            return AsciiDigits() | AsciiAlphabet();
        }

        static constexpr CharSet AsciiHexDigits() {
            return AsciiDigits() | CharSet::Range('A', 'F') | CharSet::Range('a', 'f');
        }

        static constexpr CharSet AsciiPrintable() {
            return CharSet::Range(0x20, 0x7e);
        }

        static constexpr CharSet AsciiWhitespace() { return CharSet("\t\n\v\f\r "); }

        static constexpr CharSet AsciiPunctuation() {
            return AsciiPrintable() & ~AsciiWhitespace() & ~AsciiAlphanumerics();
        }

    private:
        constexpr CharSet(uint64_t b0, uint64_t b1, uint64_t b2, uint64_t b3)
                : m_{b0, b1, b2, b3} {}

        static constexpr uint64_t RangeForWord(char lo, char hi, uint64_t word) {
            return OpenRangeFromZeroForWord(static_cast<unsigned char>(hi) + 1, word) &
                   ~OpenRangeFromZeroForWord(static_cast<unsigned char>(lo), word);
        }

        // All the chars in the specified word of the range [0, upper).
        static constexpr uint64_t OpenRangeFromZeroForWord(uint64_t upper,
                                                           uint64_t word) {
            return (upper <= 64 * word) ? 0
                                        : (upper >= 64 * (word + 1))
                                          ? ~static_cast<uint64_t>(0)
                                          : (~static_cast<uint64_t>(0) >> (64 - upper % 64));
        }

        static constexpr uint64_t CharMaskForWord(char x, uint64_t word) {
            return (static_cast<unsigned char>(x) / 64 == word)
                   ? (static_cast<uint64_t>(1)
                            << (static_cast<unsigned char>(x) % 64))
                   : 0;
        }

        constexpr void SetChar(unsigned char c) {
            m_[c / 64] |= static_cast<uint64_t>(1) << (c % 64);
        }

        uint64_t m_[4];
    };

}  // namespace turbo
