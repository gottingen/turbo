// Copyright 2023 The Turbo Authors.
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

#ifndef TURBO_UNICODE_AVX2_SIMD_H_
#define TURBO_UNICODE_AVX2_SIMD_H_

#include "turbo/simd/simd.h"
#include "turbo/base/endian.h"
#include "turbo/unicode/simd/fwd.h"
#include "turbo/unicode/avx2/engine.h"

namespace turbo::unicode::simd {

    // Forward-declared so they can be used by splat and friends.
    template<typename Child>
    struct base<Child, avx2_engine> {
        __m256i value;

        // Zero constructor
        TURBO_FORCE_INLINE base() : value{__m256i()} {}

        // Conversion from SIMD register
        TURBO_FORCE_INLINE base(const __m256i _value) : value(_value) {}
        // Conversion to SIMD register
        TURBO_FORCE_INLINE operator const __m256i &() const { return this->value; }

        TURBO_FORCE_INLINE operator __m256i &() { return this->value; }

        template<EndianNess big_endian>
        TURBO_FORCE_INLINE void store_ascii_as_utf16(char16_t *ptr) const {
            __m256i first = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(*this));
            __m256i second = _mm256_cvtepu8_epi16(_mm256_extractf128_si256(*this, 1));
            if (EndianNess::SYS_BIG_ENDIAN == big_endian) {
                const __m256i swap = _mm256_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
                                                      17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30);
                first = _mm256_shuffle_epi8(first, swap);
                second = _mm256_shuffle_epi8(second, swap);
            }
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr), first);
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr + 16), second);
        }

        TURBO_FORCE_INLINE void store_ascii_as_utf32(char32_t *ptr) const {
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr), _mm256_cvtepu8_epi32(_mm256_castsi256_si128(*this)));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr + 8),
                                _mm256_cvtepu8_epi32(_mm256_castsi256_si128(_mm256_srli_si256(*this, 8))));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr + 16),
                                _mm256_cvtepu8_epi32(_mm256_extractf128_si256(*this, 1)));
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(ptr + 24),
                                _mm256_cvtepu8_epi32(_mm_srli_si128(_mm256_extractf128_si256(*this, 1), 8)));
        }
        // Bit operations
        TURBO_FORCE_INLINE Child operator|(const Child other) const { return _mm256_or_si256(*this, other); }

        TURBO_FORCE_INLINE Child operator&(const Child other) const { return _mm256_and_si256(*this, other); }

        TURBO_FORCE_INLINE Child operator^(const Child other) const { return _mm256_xor_si256(*this, other); }

        TURBO_FORCE_INLINE Child bit_andnot(const Child other) const { return _mm256_andnot_si256(other, *this); }

        TURBO_FORCE_INLINE Child &operator|=(const Child other) {
            auto this_cast = static_cast<Child *>(this);
            *this_cast = *this_cast | other;
            return *this_cast;
        }

        TURBO_FORCE_INLINE Child &operator&=(const Child other) {
            auto this_cast = static_cast<Child *>(this);
            *this_cast = *this_cast & other;
            return *this_cast;
        }

        TURBO_FORCE_INLINE Child &operator^=(const Child other) {
            auto this_cast = static_cast<Child *>(this);
            *this_cast = *this_cast ^ other;
            return *this_cast;
        }
    };

    // Forward-declared so they can be used by splat and friends.


    template<typename T, typename Mask>
    struct base8<T, avx2_engine, Mask> : base<simd8<T, avx2_engine>, avx2_engine> {
        typedef uint32_t bitmask_t;
        typedef uint64_t bitmask2_t;

        TURBO_FORCE_INLINE base8() : base<simd8<T, avx2_engine>,avx2_engine>() {}

        TURBO_FORCE_INLINE base8(const __m256i _value) : base<simd8<T,avx2_engine>, avx2_engine>(_value) {}

        TURBO_FORCE_INLINE T first() const { return _mm256_extract_epi8(*this, 0); }

        TURBO_FORCE_INLINE T last() const { return _mm256_extract_epi8(*this, 31); }

        friend TURBO_FORCE_INLINE Mask operator==(const simd8<T,avx2_engine> lhs, const simd8<T,avx2_engine> rhs) {
            return _mm256_cmpeq_epi8(lhs, rhs);
        }

        static const int SIZE = sizeof(base<T,avx2_engine>::value);

        template<int N = 1>
        TURBO_FORCE_INLINE simd8<T,avx2_engine> prev(const simd8<T,avx2_engine> prev_chunk) const {
            return _mm256_alignr_epi8(*this, _mm256_permute2x128_si256(prev_chunk, *this, 0x21), 16 - N);
        }
    };

    // SIMD byte mask type (returned by things like eq and gt)
    template<>
    struct simd8<bool,avx2_engine> : base8<bool,avx2_engine,simd8<bool,avx2_engine>> {
        static TURBO_FORCE_INLINE simd8<bool,avx2_engine> splat(bool _value) { return _mm256_set1_epi8(uint8_t(-(!!_value))); }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine>() : base8() {}

        TURBO_FORCE_INLINE simd8<bool,avx2_engine>(const __m256i _value) : base8<bool,avx2_engine,simd8<bool,avx2_engine>>(_value) {}
        // Splat constructor
        TURBO_FORCE_INLINE simd8<bool,avx2_engine>(bool _value) : base8<bool,avx2_engine,simd8<bool,avx2_engine>>(splat(_value)) {}

        TURBO_FORCE_INLINE uint32_t to_bitmask() const { return uint32_t(_mm256_movemask_epi8(*this)); }

        TURBO_FORCE_INLINE bool any() const { return !_mm256_testz_si256(*this, *this); }

        TURBO_FORCE_INLINE bool none() const { return _mm256_testz_si256(*this, *this); }

        TURBO_FORCE_INLINE bool all() const { return static_cast<uint32_t>(_mm256_movemask_epi8(*this)) == 0xFFFFFFFF; }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator~() const { return *this ^ true; }
    };

    template<typename T>
    struct base8_numeric<T, avx2_engine> : base8<T,avx2_engine,simd8<bool,avx2_engine>> {
        static TURBO_FORCE_INLINE simd8<T,avx2_engine> splat(T _value) { return _mm256_set1_epi8(_value); }

        static TURBO_FORCE_INLINE simd8<T,avx2_engine> zero() { return _mm256_setzero_si256(); }

        static TURBO_FORCE_INLINE simd8<T,avx2_engine> load(const T values[32]) {
            return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
        }

        // Repeat 16 values as many times as necessary (usually for lookup tables)
        static TURBO_FORCE_INLINE simd8<T,avx2_engine> repeat_16(
                T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7,
                T v8, T v9, T v10, T v11, T v12, T v13, T v14, T v15
        ) {
            return simd8<T,avx2_engine>(
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15,
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15
            );
        }

        TURBO_FORCE_INLINE base8_numeric() : base8<T,avx2_engine,simd8<bool,avx2_engine>>() {}

        TURBO_FORCE_INLINE base8_numeric(const __m256i _value) : base8<T,avx2_engine,simd8<bool,avx2_engine>>(_value) {}

        // Store to array
        TURBO_FORCE_INLINE void store(T dst[32]) const {
            return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this);
        }

        // Addition/subtraction are the same for signed and unsigned
        TURBO_FORCE_INLINE simd8<T,avx2_engine> operator+(const simd8<T,avx2_engine> other) const { return _mm256_add_epi8(*this, other); }

        TURBO_FORCE_INLINE simd8<T,avx2_engine> operator-(const simd8<T,avx2_engine> other) const { return _mm256_sub_epi8(*this, other); }

        TURBO_FORCE_INLINE simd8<T,avx2_engine> &operator+=(const simd8<T,avx2_engine> other) {
            *this = *this + other;
            return *static_cast<simd8<T,avx2_engine> *>(this);
        }

        TURBO_FORCE_INLINE simd8<T,avx2_engine> &operator-=(const simd8<T,avx2_engine> other) {
            *this = *this - other;
            return *static_cast<simd8<T,avx2_engine> *>(this);
        }

        // Override to distinguish from bool version
        TURBO_FORCE_INLINE simd8<T,avx2_engine> operator~() const { return *this ^ 0xFFu; }

        // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
        template<typename L>
        TURBO_FORCE_INLINE simd8<L,avx2_engine> lookup_16(simd8<L,avx2_engine> lookup_table) const {
            return _mm256_shuffle_epi8(lookup_table, *this);
        }

        template<typename L>
        TURBO_FORCE_INLINE simd8<L,avx2_engine> lookup_16(
                L replace0, L replace1, L replace2, L replace3,
                L replace4, L replace5, L replace6, L replace7,
                L replace8, L replace9, L replace10, L replace11,
                L replace12, L replace13, L replace14, L replace15) const {
            return lookup_16(simd8<L,avx2_engine>::repeat_16(
                    replace0, replace1, replace2, replace3,
                    replace4, replace5, replace6, replace7,
                    replace8, replace9, replace10, replace11,
                    replace12, replace13, replace14, replace15
            ));
        }
    };


    // Signed bytes
    template<>
    struct simd8<int8_t,avx2_engine> : base8_numeric<int8_t, avx2_engine> {
        TURBO_FORCE_INLINE simd8() : base8_numeric<int8_t,avx2_engine>() {}

        TURBO_FORCE_INLINE simd8(const __m256i _value) : base8_numeric<int8_t,avx2_engine>(_value) {}

        // Splat constructor
        TURBO_FORCE_INLINE simd8(int8_t _value) : simd8(splat(_value)) {}
        // Array constructor
        TURBO_FORCE_INLINE simd8(const int8_t values[32]) : simd8(load(values)) {}

        TURBO_FORCE_INLINE operator simd8<uint8_t,avx2_engine>() const;
        // Member-by-member initialization
        TURBO_FORCE_INLINE simd8(
                int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5, int8_t v6, int8_t v7,
                int8_t v8, int8_t v9, int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15,
                int8_t v16, int8_t v17, int8_t v18, int8_t v19, int8_t v20, int8_t v21, int8_t v22, int8_t v23,
                int8_t v24, int8_t v25, int8_t v26, int8_t v27, int8_t v28, int8_t v29, int8_t v30, int8_t v31
        ) : simd8(_mm256_setr_epi8(
                v0, v1, v2, v3, v4, v5, v6, v7,
                v8, v9, v10, v11, v12, v13, v14, v15,
                v16, v17, v18, v19, v20, v21, v22, v23,
                v24, v25, v26, v27, v28, v29, v30, v31
        )) {}
        // Repeat 16 values as many times as necessary (usually for lookup tables)
        TURBO_FORCE_INLINE static simd8<int8_t,avx2_engine> repeat_16(
                int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5, int8_t v6, int8_t v7,
                int8_t v8, int8_t v9, int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
        ) {
            return simd8<int8_t,avx2_engine>(
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15,
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15
            );
        }

        TURBO_FORCE_INLINE bool is_ascii() const { return _mm256_movemask_epi8(*this) == 0; }
        // Order-sensitive comparisons
        TURBO_FORCE_INLINE simd8<int8_t,avx2_engine> max_val(const simd8<int8_t,avx2_engine> other) const {
            return _mm256_max_epi8(*this, other);
        }

        TURBO_FORCE_INLINE simd8<int8_t,avx2_engine> min_val(const simd8<int8_t,avx2_engine> other) const {
            return _mm256_min_epi8(*this, other);
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator>(const simd8<int8_t,avx2_engine> other) const {
            return _mm256_cmpgt_epi8(*this, other);
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator<(const simd8<int8_t,avx2_engine> other) const {
            return _mm256_cmpgt_epi8(other, *this);
        }
    };

    // Unsigned bytes
    template<>
    struct simd8<uint8_t,avx2_engine> : base8_numeric<uint8_t,avx2_engine> {
        TURBO_FORCE_INLINE simd8() : base8_numeric<uint8_t,avx2_engine>() {}

        TURBO_FORCE_INLINE simd8(const __m256i _value) : base8_numeric<uint8_t,avx2_engine>(_value) {}
        // Splat constructor
        TURBO_FORCE_INLINE simd8(uint8_t _value) : simd8(splat(_value)) {}
        // Array constructor
        TURBO_FORCE_INLINE simd8(const uint8_t values[32]) : simd8(load(values)) {}
        // Member-by-member initialization
        TURBO_FORCE_INLINE simd8(
                uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5, uint8_t v6, uint8_t v7,
                uint8_t v8, uint8_t v9, uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15,
                uint8_t v16, uint8_t v17, uint8_t v18, uint8_t v19, uint8_t v20, uint8_t v21, uint8_t v22, uint8_t v23,
                uint8_t v24, uint8_t v25, uint8_t v26, uint8_t v27, uint8_t v28, uint8_t v29, uint8_t v30, uint8_t v31
        ) : simd8(_mm256_setr_epi8(
                v0, v1, v2, v3, v4, v5, v6, v7,
                v8, v9, v10, v11, v12, v13, v14, v15,
                v16, v17, v18, v19, v20, v21, v22, v23,
                v24, v25, v26, v27, v28, v29, v30, v31
        )) {}
        // Repeat 16 values as many times as necessary (usually for lookup tables)
        TURBO_FORCE_INLINE static simd8<uint8_t,avx2_engine> repeat_16(
                uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5, uint8_t v6, uint8_t v7,
                uint8_t v8, uint8_t v9, uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
        ) {
            return simd8<uint8_t,avx2_engine>(
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15,
                    v0, v1, v2, v3, v4, v5, v6, v7,
                    v8, v9, v10, v11, v12, v13, v14, v15
            );
        }


        // Saturated math
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> saturating_add(const simd8<uint8_t,avx2_engine> other) const {
            return _mm256_adds_epu8(*this, other);
        }

        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> saturating_sub(const simd8<uint8_t,avx2_engine> other) const {
            return _mm256_subs_epu8(*this, other);
        }

        // Order-specific operations
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> max_val(const simd8<uint8_t,avx2_engine> other) const {
            return _mm256_max_epu8(*this, other);
        }

        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> min_val(const simd8<uint8_t,avx2_engine> other) const {
            return _mm256_min_epu8(other, *this);
        }
        // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> gt_bits(const simd8<uint8_t,avx2_engine> other) const {
            return this->saturating_sub(other);
        }
        // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> lt_bits(const simd8<uint8_t,avx2_engine> other) const {
            return other.saturating_sub(*this);
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator<=(const simd8<uint8_t,avx2_engine> other) const {
            return other.max_val(*this) == other;
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator>=(const simd8<uint8_t,avx2_engine> other) const {
            return other.min_val(*this) == other;
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator>(const simd8<uint8_t,avx2_engine> other) const {
            return this->gt_bits(other).any_bits_set();
        }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> operator<(const simd8<uint8_t,avx2_engine> other) const {
            return this->lt_bits(other).any_bits_set();
        }

        // Bit-specific operations
        TURBO_FORCE_INLINE simd8<bool,avx2_engine> bits_not_set() const { return *this == uint8_t(0); }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> bits_not_set(simd8<uint8_t,avx2_engine> bits) const { return (*this & bits).bits_not_set(); }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> any_bits_set() const { return ~this->bits_not_set(); }

        TURBO_FORCE_INLINE simd8<bool,avx2_engine> any_bits_set(simd8<uint8_t,avx2_engine> bits) const { return ~this->bits_not_set(bits); }

        TURBO_FORCE_INLINE bool is_ascii() const { return _mm256_movemask_epi8(*this) == 0; }

        TURBO_FORCE_INLINE bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }

        TURBO_FORCE_INLINE bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }

        TURBO_FORCE_INLINE bool bits_not_set_anywhere(simd8<uint8_t,avx2_engine> bits) const {
            return _mm256_testz_si256(*this, bits);
        }

        TURBO_FORCE_INLINE bool any_bits_set_anywhere(simd8<uint8_t,avx2_engine> bits) const {
            return !bits_not_set_anywhere(bits);
        }

        template<int N>
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> shr() const {
            return simd8<uint8_t,avx2_engine>(_mm256_srli_epi16(*this, N)) & uint8_t(0xFFu >> N);
        }

        template<int N>
        TURBO_FORCE_INLINE simd8<uint8_t,avx2_engine> shl() const {
            return simd8<uint8_t,avx2_engine>(_mm256_slli_epi16(*this, N)) & uint8_t(0xFFu << N);
        }

        // Get one of the bits and make a bitmask out of it.
        // e.g. value.get_bit<7>() gets the high bit
        template<int N>
        TURBO_FORCE_INLINE int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 7 - N)); }
    };

    TURBO_FORCE_INLINE simd8<int8_t,avx2_engine>::operator simd8<uint8_t,avx2_engine>() const { return this->value; }


    template<typename T>
    struct simd8x64<T, avx2_engine> {
        static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T,avx2_engine>);
        static_assert(NUM_CHUNKS == 2, "Haswell kernel should use two registers per 64-byte block.");
        simd8<T,avx2_engine> chunks[NUM_CHUNKS];

        simd8x64(const simd8x64<T, avx2_engine> &o) = delete; // no copy allowed
        simd8x64<T, avx2_engine> &operator=(const simd8<T,avx2_engine> other) = delete; // no assignment allowed
        simd8x64() = delete; // no default constructor allowed

        TURBO_FORCE_INLINE simd8x64(const simd8<T,avx2_engine> chunk0, const simd8<T,avx2_engine> chunk1) : chunks{chunk0, chunk1} {}

        TURBO_FORCE_INLINE simd8x64(const T *ptr) : chunks{simd8<T,avx2_engine>::load(ptr),
                                                           simd8<T,avx2_engine>::load(ptr + sizeof(simd8<T,avx2_engine>) / sizeof(T))} {}

        TURBO_FORCE_INLINE void store(T *ptr) const {
            this->chunks[0].store(ptr + sizeof(simd8<T,avx2_engine>) * 0 / sizeof(T));
            this->chunks[1].store(ptr + sizeof(simd8<T,avx2_engine>) * 1 / sizeof(T));
        }

        TURBO_FORCE_INLINE uint64_t to_bitmask() const {
            uint64_t r_lo = uint32_t(this->chunks[0].to_bitmask());
            uint64_t r_hi = this->chunks[1].to_bitmask();
            return r_lo | (r_hi << 32);
        }

        TURBO_FORCE_INLINE simd8x64<T, avx2_engine> &operator|=(const simd8x64<T, avx2_engine> &other) {
            this->chunks[0] |= other.chunks[0];
            this->chunks[1] |= other.chunks[1];
            return *this;
        }

        TURBO_FORCE_INLINE simd8<T,avx2_engine> reduce_or() const {
            return this->chunks[0] | this->chunks[1];
        }

        TURBO_FORCE_INLINE bool is_ascii() const {
            return this->reduce_or().is_ascii();
        }

        template<EndianNess endian>
        TURBO_FORCE_INLINE void store_ascii_as_utf16(char16_t *ptr) const {
            this->chunks[0].template store_ascii_as_utf16<endian>(ptr + sizeof(simd8<T,avx2_engine>) * 0);
            this->chunks[1].template store_ascii_as_utf16<endian>(ptr + sizeof(simd8<T,avx2_engine>) * 1);
        }

        TURBO_FORCE_INLINE void store_ascii_as_utf32(char32_t *ptr) const {
            this->chunks[0].store_ascii_as_utf32(ptr + sizeof(simd8<T,avx2_engine>) * 0);
            this->chunks[1].store_ascii_as_utf32(ptr + sizeof(simd8<T,avx2_engine>) * 1);
        }

        TURBO_FORCE_INLINE simd8x64<T, avx2_engine> bit_or(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<T, avx2_engine>(
                    this->chunks[0] | mask,
                    this->chunks[1] | mask
            );
        }

        TURBO_FORCE_INLINE uint64_t eq(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] == mask,
                    this->chunks[1] == mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t eq(const simd8x64<uint8_t, avx2_engine> &other) const {
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] == other.chunks[0],
                    this->chunks[1] == other.chunks[1]
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t lteq(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] <= mask,
                    this->chunks[1] <= mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t in_range(const T low, const T high) const {
            const simd8<T,avx2_engine> mask_low = simd8<T,avx2_engine>::splat(low);
            const simd8<T,avx2_engine> mask_high = simd8<T,avx2_engine>::splat(high);

            return simd8x64<bool, avx2_engine>(
                    (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
                    (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low)
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t not_in_range(const T low, const T high) const {
            const simd8<T,avx2_engine> mask_low = simd8<T,avx2_engine>::splat(low);
            const simd8<T,avx2_engine> mask_high = simd8<T,avx2_engine>::splat(high);
            return simd8x64<bool, avx2_engine>(
                    (this->chunks[0] > mask_high) | (this->chunks[0] < mask_low),
                    (this->chunks[1] > mask_high) | (this->chunks[1] < mask_low)
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t lt(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] < mask,
                    this->chunks[1] < mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t gt(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] > mask,
                    this->chunks[1] > mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t gteq(const T m) const {
            const simd8<T,avx2_engine> mask = simd8<T,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    this->chunks[0] >= mask,
                    this->chunks[1] >= mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t gteq_unsigned(const uint8_t m) const {
            const simd8<uint8_t,avx2_engine> mask = simd8<uint8_t,avx2_engine>::splat(m);
            return simd8x64<bool, avx2_engine>(
                    (simd8<uint8_t,avx2_engine>(__m256i(this -> chunks[0])) >= mask),
                    (simd8<uint8_t,avx2_engine>(__m256i(this -> chunks[1])) >= mask)
            ).to_bitmask();
        }
    }; // struct simd8x64<T, avx2_engine>

    template<typename T, typename Mask>
    struct base16<T, avx2_engine, Mask>: base<simd16<T,avx2_engine>,avx2_engine> {
        using bitmask_type = uint32_t;
        TURBO_FORCE_INLINE base16() : base<simd16<T, avx2_engine>,avx2_engine>() {}
        TURBO_FORCE_INLINE base16(const __m256i _value) : base<simd16<T, avx2_engine>, avx2_engine>(_value) {}
        template <typename Pointer>
        TURBO_FORCE_INLINE base16(const Pointer* ptr) : base16(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr))) {}
        friend TURBO_FORCE_INLINE Mask operator==(const simd16<T, avx2_engine> lhs, const simd16<T, avx2_engine> rhs) { return _mm256_cmpeq_epi16(lhs, rhs); }

        /// the size of vector in bytes
        static const int SIZE = sizeof(base<simd16<T, avx2_engine>,avx2_engine>::value);

        /// the number of elements of type T a vector can hold
        static const int ELEMENTS = SIZE / sizeof(T);

        template<int N=1>
        TURBO_FORCE_INLINE simd16<T, avx2_engine> prev(const simd16<T, avx2_engine> prev_chunk) const {
            return _mm256_alignr_epi8(*this, prev_chunk, 16 - N);
        }
    };

// SIMD byte mask type (returned by things like eq and gt)
    template<>
    struct simd16<bool, avx2_engine>: base16<bool,avx2_engine,simd16<bool, avx2_engine>> {
        static TURBO_FORCE_INLINE simd16<bool, avx2_engine> splat(bool _value) { return _mm256_set1_epi16(uint16_t(-(!!_value))); }

        TURBO_FORCE_INLINE simd16<bool, avx2_engine>() : base16() {}
        TURBO_FORCE_INLINE simd16<bool, avx2_engine>(const __m256i _value) : base16<bool,avx2_engine, simd16<bool, avx2_engine>>(_value) {}
        // Splat constructor
        TURBO_FORCE_INLINE simd16<bool, avx2_engine>(bool _value) : base16<bool,avx2_engine,simd16<bool, avx2_engine>>(splat(_value)) {}

        TURBO_FORCE_INLINE bitmask_type to_bitmask() const { return _mm256_movemask_epi8(*this); }
        TURBO_FORCE_INLINE bool any() const { return !_mm256_testz_si256(*this, *this); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator~() const { return *this ^ true; }
    };

    template<typename T>
    struct base16_numeric<T, avx2_engine>: base16<T,avx2_engine,simd16<bool, avx2_engine>> {
        static TURBO_FORCE_INLINE simd16<T, avx2_engine> splat(T _value) { return _mm256_set1_epi16(_value); }
        static TURBO_FORCE_INLINE simd16<T, avx2_engine> zero() { return _mm256_setzero_si256(); }
        static TURBO_FORCE_INLINE simd16<T, avx2_engine> load(const T values[8]) {
            return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
        }

        TURBO_FORCE_INLINE base16_numeric() : base16<T,avx2_engine,simd16<bool, avx2_engine>>() {}
        TURBO_FORCE_INLINE base16_numeric(const __m256i _value) : base16<T,avx2_engine,simd16<bool, avx2_engine>>(_value) {}

        // Store to array
        TURBO_FORCE_INLINE void store(T dst[8]) const { return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this); }

        // Override to distinguish from bool version
        TURBO_FORCE_INLINE simd16<T, avx2_engine> operator~() const { return *this ^ 0xFFFFu; }

        // Addition/subtraction are the same for signed and unsigned
        TURBO_FORCE_INLINE simd16<T, avx2_engine> operator+(const simd16<T, avx2_engine> other) const { return _mm256_add_epi16(*this, other); }
        TURBO_FORCE_INLINE simd16<T, avx2_engine> operator-(const simd16<T, avx2_engine> other) const { return _mm256_sub_epi16(*this, other); }
        TURBO_FORCE_INLINE simd16<T, avx2_engine>& operator+=(const simd16<T, avx2_engine> other) { *this = *this + other; return *static_cast<simd16<T, avx2_engine>*>(this); }
        TURBO_FORCE_INLINE simd16<T, avx2_engine>& operator-=(const simd16<T, avx2_engine> other) { *this = *this - other; return *static_cast<simd16<T, avx2_engine>*>(this); }
    };

    // Signed code units
    template<>
    struct simd16<int16_t,avx2_engine> : base16_numeric<int16_t,avx2_engine> {
        TURBO_FORCE_INLINE simd16() : base16_numeric<int16_t,  avx2_engine>() {}
        TURBO_FORCE_INLINE simd16(const __m256i _value) : base16_numeric<int16_t,  avx2_engine>(_value) {}
        // Splat constructor
        TURBO_FORCE_INLINE simd16(int16_t _value) : simd16(splat(_value)) {}
        // Array constructor
        TURBO_FORCE_INLINE simd16(const int16_t* values) : simd16(load(values)) {}
        TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const int16_t*>(values))) {}
        // Order-sensitive comparisons
        TURBO_FORCE_INLINE simd16<int16_t,  avx2_engine> max_val(const simd16<int16_t,  avx2_engine> other) const { return _mm256_max_epi16(*this, other); }
        TURBO_FORCE_INLINE simd16<int16_t,  avx2_engine> min_val(const simd16<int16_t,  avx2_engine> other) const { return _mm256_min_epi16(*this, other); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator>(const simd16<int16_t,  avx2_engine> other) const { return _mm256_cmpgt_epi16(*this, other); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator<(const simd16<int16_t,  avx2_engine> other) const { return _mm256_cmpgt_epi16(other, *this); }
    };

    // Unsigned code units
    template<>
    struct simd16<uint16_t,  avx2_engine>: base16_numeric<uint16_t,  avx2_engine>  {
        TURBO_FORCE_INLINE simd16() : base16_numeric<uint16_t,  avx2_engine>() {}
        TURBO_FORCE_INLINE simd16(const __m256i _value) : base16_numeric<uint16_t,  avx2_engine>(_value) {}

        // Splat constructor
        TURBO_FORCE_INLINE simd16(uint16_t _value) : simd16(splat(_value)) {}
        // Array constructor
        TURBO_FORCE_INLINE simd16(const uint16_t* values) : simd16(load(values)) {}
        TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const uint16_t*>(values))) {}

        // Saturated math
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> saturating_add(const simd16<uint16_t,  avx2_engine> other) const { return _mm256_adds_epu16(*this, other); }
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> saturating_sub(const simd16<uint16_t,  avx2_engine> other) const { return _mm256_subs_epu16(*this, other); }

        // Order-specific operations
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> max_val(const simd16<uint16_t,  avx2_engine> other) const { return _mm256_max_epu16(*this, other); }
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> min_val(const simd16<uint16_t,  avx2_engine> other) const { return _mm256_min_epu16(*this, other); }
        // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> gt_bits(const simd16<uint16_t,  avx2_engine> other) const { return this->saturating_sub(other); }
        // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> lt_bits(const simd16<uint16_t,  avx2_engine> other) const { return other.saturating_sub(*this); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator<=(const simd16<uint16_t,  avx2_engine> other) const { return other.max_val(*this) == other; }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator>=(const simd16<uint16_t,  avx2_engine> other) const { return other.min_val(*this) == other; }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator>(const simd16<uint16_t,  avx2_engine> other) const { return this->gt_bits(other).any_bits_set(); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> operator<(const simd16<uint16_t,  avx2_engine> other) const { return this->gt_bits(other).any_bits_set(); }

        // Bit-specific operations
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> bits_not_set() const { return *this == uint16_t(0); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> bits_not_set(simd16<uint16_t,  avx2_engine> bits) const { return (*this & bits).bits_not_set(); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> any_bits_set() const { return ~this->bits_not_set(); }
        TURBO_FORCE_INLINE simd16<bool, avx2_engine> any_bits_set(simd16<uint16_t,  avx2_engine> bits) const { return ~this->bits_not_set(bits); }

        TURBO_FORCE_INLINE bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }
        TURBO_FORCE_INLINE bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
        TURBO_FORCE_INLINE bool bits_not_set_anywhere(simd16<uint16_t,  avx2_engine> bits) const { return _mm256_testz_si256(*this, bits); }
        TURBO_FORCE_INLINE bool any_bits_set_anywhere(simd16<uint16_t,  avx2_engine> bits) const { return !bits_not_set_anywhere(bits); }
        template<int N>
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> shr() const { return simd16<uint16_t,  avx2_engine>(_mm256_srli_epi16(*this, N)); }
        template<int N>
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> shl() const { return simd16<uint16_t,  avx2_engine>(_mm256_slli_epi16(*this, N)); }
        // Get one of the bits and make a bitmask out of it.
        // e.g. value.get_bit<7>() gets the high bit
        template<int N>
        TURBO_FORCE_INLINE int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 15-N)); }

        // Change the endianness
        TURBO_FORCE_INLINE simd16<uint16_t,  avx2_engine> swap_bytes() const {
            const __m256i swap = _mm256_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
                                                  17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30);
            return _mm256_shuffle_epi8(*this, swap);
        }

        // Pack with the unsigned saturation two uint16_t code units into single uint8_t vector
        static TURBO_FORCE_INLINE simd8<uint8_t, avx2_engine> pack(const simd16<uint16_t,  avx2_engine>& v0, const simd16<uint16_t,  avx2_engine>& v1) {
            // Note: the AVX2 variant of pack operates on 128-bit lanes, thus
            //       we have to shuffle lanes in order to produce bytes in the
            //       correct order.

            // get the 0th lanes
            const __m128i lo_0 = _mm256_extracti128_si256(v0, 0);
            const __m128i lo_1 = _mm256_extracti128_si256(v1, 0);

            // get the 1st lanes
            const __m128i hi_0 = _mm256_extracti128_si256(v0, 1);
            const __m128i hi_1 = _mm256_extracti128_si256(v1, 1);

            // build new vectors (shuffle lanes)
            const __m256i t0 = _mm256_set_m128i(lo_1, lo_0);
            const __m256i t1 = _mm256_set_m128i(hi_1, hi_0);

            // pack code units in linear order from v0 and v1
            return _mm256_packus_epi16(t0, t1);
        }
    };


    template<typename T>
    struct simd16x32<T, avx2_engine> {
        static constexpr int NUM_CHUNKS = 64 / sizeof(simd16<T, avx2_engine>);
        static_assert(NUM_CHUNKS == 2, "Haswell kernel should use two registers per 64-byte block.");
        simd16<T, avx2_engine> chunks[NUM_CHUNKS];

        simd16x32(const simd16x32<T,  avx2_engine>& o) = delete; // no copy allowed
        simd16x32<T,  avx2_engine>& operator=(const simd16<T, avx2_engine> other) = delete; // no assignment allowed
        simd16x32() = delete; // no default constructor allowed

        TURBO_FORCE_INLINE simd16x32(const simd16<T, avx2_engine> chunk0, const simd16<T, avx2_engine> chunk1) : chunks{chunk0, chunk1} {}
        TURBO_FORCE_INLINE simd16x32(const T* ptr) : chunks{simd16<T, avx2_engine>::load(ptr), simd16<T, avx2_engine>::load(ptr+sizeof(simd16<T, avx2_engine>)/sizeof(T))} {}

        TURBO_FORCE_INLINE void store(T* ptr) const {
            this->chunks[0].store(ptr+sizeof(simd16<T, avx2_engine>)*0/sizeof(T));
            this->chunks[1].store(ptr+sizeof(simd16<T, avx2_engine>)*1/sizeof(T));
        }

        TURBO_FORCE_INLINE uint64_t to_bitmask() const {
            uint64_t r_lo = uint32_t(this->chunks[0].to_bitmask());
            uint64_t r_hi =                       this->chunks[1].to_bitmask();
            return r_lo | (r_hi << 32);
        }

        TURBO_FORCE_INLINE simd16<T, avx2_engine> reduce_or() const {
            return this->chunks[0] | this->chunks[1];
        }

        TURBO_FORCE_INLINE bool is_ascii() const {
            return this->reduce_or().is_ascii();
        }

        TURBO_FORCE_INLINE void store_ascii_as_utf16(char16_t * ptr) const {
            this->chunks[0].store_ascii_as_utf16(ptr+sizeof(simd16<T, avx2_engine>)*0);
            this->chunks[1].store_ascii_as_utf16(ptr+sizeof(simd16<T, avx2_engine>));
        }

        TURBO_FORCE_INLINE simd16x32<T,  avx2_engine> bit_or(const T m) const {
            const simd16<T, avx2_engine> mask = simd16<T, avx2_engine>::splat(m);
            return simd16x32<T,  avx2_engine>(
                    this->chunks[0] | mask,
                            this->chunks[1] | mask
            );
        }

        TURBO_FORCE_INLINE void swap_bytes() {
            this->chunks[0] = this->chunks[0].swap_bytes();
            this->chunks[1] = this->chunks[1].swap_bytes();
        }

        TURBO_FORCE_INLINE uint64_t eq(const T m) const {
            const simd16<T, avx2_engine> mask = simd16<T, avx2_engine>::splat(m);
            return  simd16x32<bool,  avx2_engine>(
                    this->chunks[0] == mask,
                            this->chunks[1] == mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t eq(const simd16x32<uint16_t,  avx2_engine> &other) const {
            return  simd16x32<bool,  avx2_engine>(
                    this->chunks[0] == other.chunks[0],
                            this->chunks[1] == other.chunks[1]
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t lteq(const T m) const {
            const simd16<T, avx2_engine> mask = simd16<T, avx2_engine>::splat(m);
            return  simd16x32<bool,  avx2_engine>(
                    this->chunks[0] <= mask,
                            this->chunks[1] <= mask
            ).to_bitmask();
        }

        TURBO_FORCE_INLINE uint64_t in_range(const T low, const T high) const {
            const simd16<T, avx2_engine> mask_low = simd16<T, avx2_engine>::splat(low);
            const simd16<T, avx2_engine> mask_high = simd16<T, avx2_engine>::splat(high);

            return  simd16x32<bool,  avx2_engine>(
                    (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
                            (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low)
            ).to_bitmask();
        }
        TURBO_FORCE_INLINE uint64_t not_in_range(const T low, const T high) const {
            const simd16<T, avx2_engine> mask_low = simd16<T, avx2_engine>::splat(static_cast<T>(low-1));
            const simd16<T, avx2_engine> mask_high = simd16<T, avx2_engine>::splat(static_cast<T>(high+1));
            return simd16x32<bool,  avx2_engine>(
                    (this->chunks[0] >= mask_high) | (this->chunks[0] <= mask_low),
                            (this->chunks[1] >= mask_high) | (this->chunks[1] <= mask_low)
            ).to_bitmask();
        }
        TURBO_FORCE_INLINE uint64_t lt(const T m) const {
            const simd16<T, avx2_engine> mask = simd16<T, avx2_engine>::splat(m);
            return  simd16x32<bool,  avx2_engine>(
                    this->chunks[0] < mask,
                            this->chunks[1] < mask
            ).to_bitmask();
        }
    }; // struct simd16x32<T,  avx2_engine>

    template<typename Engine>
    TURBO_FORCE_INLINE bool is_ascii(const simd8x64<uint8_t, Engine>& input) {
        return input.reduce_or().is_ascii();
    }

    template<typename Engine>
    TURBO_FORCE_INLINE simd8<bool, Engine> must_be_2_3_continuation(const simd8<uint8_t,Engine> prev2, const simd8<uint8_t, Engine> prev3) {
        simd8<uint8_t,Engine> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
        simd8<uint8_t, Engine> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
        // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
        return simd8<int8_t, Engine>(is_third_byte | is_fourth_byte) > int8_t(0);
    }
}  // namespace turbo::unicode::simd

#endif // TURBO_UNICODE_AVX2_SIMD_H_
