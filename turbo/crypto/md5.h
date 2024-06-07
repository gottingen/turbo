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
// Created by jeff on 24-6-5.
//

#pragma once

#include <turbo/strings/string_view.h>
#include <turbo/base/endian.h>
#include <array>
#include <cstdint>

namespace turbo {

    template <unsigned N> class SmallString;
    template <typename T> class ArrayRef;

    class MD5 {
    public:
        struct MD5Result : public std::array<uint8_t, 16> {
            std::string digest() const;

            uint64_t low() const {
                return turbo::little_endian::load64(data());

            }

            uint64_t high() const {
                return turbo::little_endian::load64(data() + 8);
            }
            std::pair<uint64_t, uint64_t> words() const {
                return std::make_pair(high(), low());
            }
        };

        MD5();

        /// Updates the hash for the byte stream provided.
        void update(ArrayRef<uint8_t> Data);

        /// Updates the hash for the StringRef provided.
        void update(std::string_view Str);

        /// Finishes off the hash and puts the result in result.
        void final(MD5Result &Result);

        /// Finishes off the hash, and returns the 16-byte hash data.
        MD5Result final();

        /// Finishes off the hash, and returns the 16-byte hash data.
        /// This is suitable for getting the MD5 at any time without invalidating the
        /// internal state, so that more calls can be made into `update`.
        MD5Result result();

        /// Computes the hash for a given bytes.
        static MD5Result hash(ArrayRef<uint8_t> Data);

    private:
        // Any 32-bit or wider unsigned integer data type will do.
        typedef uint32_t MD5_u32plus;

        // Internal State
        struct {
            MD5_u32plus a = 0x67452301;
            MD5_u32plus b = 0xefcdab89;
            MD5_u32plus c = 0x98badcfe;
            MD5_u32plus d = 0x10325476;
            MD5_u32plus hi = 0;
            MD5_u32plus lo = 0;
            uint8_t buffer[64];
            MD5_u32plus block[16];
        } InternalState;

        const uint8_t *body(ArrayRef<uint8_t> Data);
    };

    /// Helper to compute and return lower 64 bits of the given string's MD5 hash.
    inline uint64_t md5_hash(std::string_view Str) {
        MD5 Hash;
        Hash.update(Str);
        MD5::MD5Result Result;
        Hash.final(Result);
        // Return the least significant word.
        return Result.low();
    }

    template<typename Sink>
    void turbo_stringify(Sink &sink, const MD5::MD5Result &md5) {
        sink << md5.digest();
    }

} // end namespace turbo
