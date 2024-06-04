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


#include <turbo/crypto/md5.h>
#include <turbo/container/array_ref.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <turbo/strings/escaping.h>

// The basic MD5 functions.

// F and G are optimized compared to their RFC 1321 definitions for
// architectures that lack an AND-NOT instruction, just like in Colin Plumb's
// implementation.
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

// The MD5 transformation for all four rounds.
#define STEP(f, a, b, c, d, x, t, s)                                           \
  (a) += f((b), (c), (d)) + (x) + (t);                                         \
  (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s))));                   \
  (a) += (b);

// SET reads 4 input bytes in little-endian byte order and stores them
// in a properly aligned word in host byte order.
#define SET(n)                                                                 \
  (InternalState.block[(n)] = (MD5_u32plus)ptr[(n)*4] |                        \
                              ((MD5_u32plus)ptr[(n)*4 + 1] << 8) |             \
                              ((MD5_u32plus)ptr[(n)*4 + 2] << 16) |            \
                              ((MD5_u32plus)ptr[(n)*4 + 3] << 24))
#define GET(n) (InternalState.block[(n)])

namespace turbo {

/// This processes one or more 64-byte data blocks, but does NOT update
///the bit counters.  There are no alignment requirements.
    const uint8_t *MD5::body(ArrayRef <uint8_t> Data) {
        const uint8_t *ptr;
        MD5_u32plus a, b, c, d;
        MD5_u32plus saved_a, saved_b, saved_c, saved_d;
        unsigned long Size = Data.size();

        ptr = Data.data();

        a = InternalState.a;
        b = InternalState.b;
        c = InternalState.c;
        d = InternalState.d;

        do {
            saved_a = a;
            saved_b = b;
            saved_c = c;
            saved_d = d;

            // Round 1
            STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
            STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
            STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
            STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
            STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
            STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
            STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
            STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
            STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
            STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
            STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
            STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
            STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
            STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
            STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
            STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

            // Round 2
            STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
            STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
            STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
            STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
            STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
            STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
            STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
            STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
            STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
            STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
            STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
            STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
            STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
            STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
            STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
            STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

            // Round 3
            STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
            STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
            STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
            STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
            STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
            STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
            STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
            STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
            STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
            STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
            STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
            STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
            STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
            STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
            STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
            STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)

            // Round 4
            STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
            STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
            STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
            STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
            STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
            STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
            STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
            STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
            STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
            STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
            STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
            STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
            STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
            STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
            STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
            STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

            a += saved_a;
            b += saved_b;
            c += saved_c;
            d += saved_d;

            ptr += 64;
        } while (Size -= 64);

        InternalState.a = a;
        InternalState.b = b;
        InternalState.c = c;
        InternalState.d = d;

        return ptr;
    }

    MD5::MD5() = default;

/// Incrementally add the bytes in \p Data to the hash.
    void MD5::update(ArrayRef <uint8_t> Data) {
        MD5_u32plus saved_lo;
        unsigned long used, free;
        const uint8_t *Ptr = Data.data();
        unsigned long Size = Data.size();

        saved_lo = InternalState.lo;
        if ((InternalState.lo = (saved_lo + Size) & 0x1fffffff) < saved_lo)
            InternalState.hi++;
        InternalState.hi += Size >> 29;

        used = saved_lo & 0x3f;

        if (used) {
            free = 64 - used;

            if (Size < free) {
                memcpy(&InternalState.buffer[used], Ptr, Size);
                return;
            }

            memcpy(&InternalState.buffer[used], Ptr, free);
            Ptr = Ptr + free;
            Size -= free;
            body(ArrayRef(InternalState.buffer, 64));
        }

        if (Size >= 64) {
            Ptr = body(ArrayRef(Ptr, Size & ~(unsigned long) 0x3f));
            Size &= 0x3f;
        }

        memcpy(InternalState.buffer, Ptr, Size);
    }

    /// Add the bytes in the StringRef \p Str to the hash.
    // Note that this isn't a string and so this won't include any trailing NULL
    // bytes.
    void MD5::update(turbo::string_view Str) {
        ArrayRef <uint8_t> SVal((const uint8_t *) Str.data(), Str.size());
        update(SVal);
    }

    /// Finish the hash and place the resulting hash into \p result.
    /// \param Result is assumed to be a minimum of 16-bytes in size.
    void MD5::final(MD5Result &Result) {
        unsigned long used, free;

        used = InternalState.lo & 0x3f;

        InternalState.buffer[used++] = 0x80;

        free = 64 - used;

        if (free < 8) {
            memset(&InternalState.buffer[used], 0, free);
            body(ArrayRef(InternalState.buffer, 64));
            used = 0;
            free = 64;
        }

        memset(&InternalState.buffer[used], 0, free - 8);

        InternalState.lo <<= 3;
        turbo::little_endian::store32(&InternalState.buffer[56], InternalState.lo);
        turbo::little_endian::store32(&InternalState.buffer[60], InternalState.hi);

        body(ArrayRef(InternalState.buffer, 64));

        turbo::little_endian::store32(&Result[0], InternalState.a);
        turbo::little_endian::store32(&Result[4], InternalState.b);
        turbo::little_endian::store32(&Result[8], InternalState.c);
        turbo::little_endian::store32(&Result[12], InternalState.d);
    }

    MD5::MD5Result MD5::final() {
        MD5Result Result;
        final(Result);
        return Result;
    }

    MD5::MD5Result MD5::result() {
        auto StateToRestore = InternalState;

        auto Hash = final();

        // Restore the state
        InternalState = StateToRestore;

        return Hash;
    }

    std::string MD5::MD5Result::digest() const {
        std::string Str = turbo::bytes_to_hex_string(std::string_view(reinterpret_cast<const char*>(this->data()), this->size()));
        return Str;
    }

    MD5::MD5Result MD5::hash(ArrayRef <uint8_t> Data) {
        MD5 Hash;
        Hash.update(Data);
        MD5::MD5Result Res;
        Hash.final(Res);

        return Res;
    }


}