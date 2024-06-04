//
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
// Created by jeff on 24-6-1.
//

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <type_traits>
#include <turbo/base/macros.h>
#include <turbo/base/endian.h>

namespace turbo {

    /// \brief Class representing an IEEE half-precision float, encoded as a `uint16_t`
    ///
    /// The exact format is as follows (from LSB to MSB):
    /// - bits 0-10:  mantissa
    /// - bits 10-15: exponent
    /// - bit 15:     sign
    ///
    class TURBO_DLL Float16 {
    public:
        Float16() = default;

        explicit Float16(float f) : Float16(from_float(f)) {}

        explicit Float16(double d) : Float16(from_double(d)) {}

        template<typename T,
                typename std::enable_if_t<std::is_convertible_v<T, double>> * = nullptr>
        explicit Float16(T v) : Float16(static_cast<double>(v)) {}

        /// \brief Create a `Float16` from its exact binary representation
        constexpr static Float16 from_bits(uint16_t bits) { return Float16{bits, bool{}}; }

        /// \brief Create a `Float16` from a 32-bit float (may lose precision)
        static Float16 from_float(float f);

        /// \brief Create a `Float16` from a 64-bit float (may lose precision)
        static Float16 from_double(double d);

        /// \brief Read a `Float16` from memory in native-endian byte order
        static Float16 from_bytes(const uint8_t *src) {
            return from_bits(turbo::load16(src));
        }

        /// \brief Read a `Float16` from memory in little-endian byte order
        static Float16 from_little_endian(const uint8_t *src) {
            return from_bits(turbo::little_endian::load16(src));
        }

        /// \brief Read a `Float16` from memory in big-endian byte order
        static Float16 from_big_endian(const uint8_t *src) {
            return from_bits(turbo::big_endian::load16(src));
        }

        /// \brief Return the value's binary representation as a `uint16_t`
        constexpr uint16_t bits() const { return bits_; }

        /// \brief Return true if the value is negative (sign bit is set)
        constexpr bool signbit() const { return (bits_ & 0x8000) != 0; }

        /// \brief Return true if the value is NaN
        constexpr bool is_nan() const { return (bits_ & 0x7fff) > 0x7c00; }

        /// \brief Return true if the value is positive/negative infinity
        constexpr bool is_infinity() const { return (bits_ & 0x7fff) == 0x7c00; }

        /// \brief Return true if the value is finite and not NaN
        constexpr bool is_finite() const { return (bits_ & 0x7c00) != 0x7c00; }

        /// \brief Return true if the value is positive/negative zero
        constexpr bool is_zero() const { return (bits_ & 0x7fff) == 0; }

        /// \brief Convert to a 32-bit float
        float to_float() const;

        /// \brief Convert to a 64-bit float
        double to_double() const;

        explicit operator float() const { return to_float(); }

        explicit operator double() const { return to_double(); }

        /// \brief Copy the value's bytes in native-endian byte order
        void to_bytes(uint8_t *dest) const { std::memcpy(dest, &bits_, sizeof(bits_)); }

        /// \brief Return the value's bytes in native-endian byte order
        constexpr std::array<uint8_t, 2> to_bytes() const {
#if TURBO_IS_LITTLE_ENDIAN
            return to_little_endian();
#else
            return to_big_endian();
#endif
        }

        /// \brief Copy the value's bytes in little-endian byte order
        void to_little_endian(uint8_t *dest) const {
            const auto bytes = to_little_endian();
            std::memcpy(dest, bytes.data(), bytes.size());
        }

        /// \brief Return the value's bytes in little-endian byte order
        constexpr std::array<uint8_t, 2> to_little_endian() const {
#if TURBO_IS_LITTLE_ENDIAN
            return {uint8_t(bits_ & 0xff), uint8_t(bits_ >> 8)};
#else
            return {uint8_t(bits_ >> 8), uint8_t(bits_ & 0xff)};
#endif
        }

        /// \brief Copy the value's bytes in big-endian byte order
        void to_big_endian(uint8_t *dest) const {
            const auto bytes = to_big_endian();
            std::memcpy(dest, bytes.data(), bytes.size());
        }

        /// \brief Return the value's bytes in big-endian byte order
        constexpr std::array<uint8_t, 2> to_big_endian() const {
#if TURBO_IS_LITTLE_ENDIAN
            return {uint8_t(bits_ >> 8), uint8_t(bits_ & 0xff)};
#else
            return {uint8_t(bits_ & 0xff), uint8_t(bits_ >> 8)};
#endif
        }

        constexpr Float16 operator-() const { return from_bits(bits_ ^ 0x8000); }

        constexpr Float16 operator+() const { return from_bits(bits_); }

        friend constexpr bool operator==(Float16 lhs, Float16 rhs) {
            if (lhs.is_nan() || rhs.is_nan()) return false;
            return Float16::compare_eq(lhs, rhs);
        }

        friend constexpr bool operator!=(Float16 lhs, Float16 rhs) { return !(lhs == rhs); }

        friend constexpr bool operator<(Float16 lhs, Float16 rhs) {
            if (lhs.is_nan() || rhs.is_nan()) return false;
            return Float16::compare_lt(lhs, rhs);
        }

        friend constexpr bool operator>(Float16 lhs, Float16 rhs) { return rhs < lhs; }

        friend constexpr bool operator<=(Float16 lhs, Float16 rhs) {
            if (lhs.is_nan() || rhs.is_nan()) return false;
            return !Float16::compare_lt(rhs, lhs);
        }

        friend constexpr bool operator>=(Float16 lhs, Float16 rhs) { return rhs <= lhs; }

        friend std::ostream &operator<<(std::ostream &os, Float16 arg);
        // Support for turbo::Hash.
        template <typename H>
        friend H turbo_hash_value(H h, Float16 v) {
            return H::combine(std::move(h), v.to_float());
        }

        // Support for turbo::str_cat() etc.
        template <typename Sink>
        friend void turbo_stringify(Sink& sink, Float16 v) {
            sink.Append(v.to_float());
        }

    protected:
        uint16_t bits_;

    private:
        constexpr Float16(uint16_t bits, bool) : bits_(bits) {}

        // Comparison helpers that assume neither operand is NaN
        static constexpr bool compare_eq(Float16 lhs, Float16 rhs) {
            return (lhs.bits() == rhs.bits()) || (lhs.is_zero() && rhs.is_zero());
        }

        static constexpr bool compare_lt(Float16 lhs, Float16 rhs) {
            if (lhs.signbit()) {
                if (rhs.signbit()) {
                    // Both are negative
                    return lhs.bits() > rhs.bits();
                } else {
                    // Handle +/-0
                    return !lhs.is_zero() || rhs.bits() != 0;
                }
            } else if (rhs.signbit()) {
                return false;
            } else {
                // Both are positive
                return lhs.bits() < rhs.bits();
            }
        }
    };

    static_assert(std::is_trivial_v<Float16>);
}  // namespace turbo

// TODO: Not complete
template<>
class std::numeric_limits<turbo::Float16> {
    using T = turbo::Float16;

public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool has_infinity = true;
    static constexpr bool has_quiet_NaN = true;

    static constexpr T min() { return T::from_bits(0b0000010000000000); }

    static constexpr T max() { return T::from_bits(0b0111101111111111); }

    static constexpr T lowest() { return -max(); }

    static constexpr T infinity() { return T::from_bits(0b0111110000000000); }

    static constexpr T quiet_NaN() { return T::from_bits(0b0111111111111111); }
};
