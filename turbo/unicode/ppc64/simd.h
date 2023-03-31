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

#ifndef TURBO_UNICODE_PPC64_SIMD_H_
#define TURBO_UNICODE_PPC64_SIMD_H_

#include "turbo/unicode/utf.h"
#include "turbo/unicode/ppc64/bitmanipulation.h"
#include <type_traits>

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {
namespace simd {

using __m128i = __vector unsigned char;

template <typename Child> struct base {
  __m128i value;

  // Zero constructor
  TURBO_FORCE_INLINE base() : value{__m128i()} {}

  // Conversion from SIMD register
  TURBO_FORCE_INLINE base(const __m128i _value) : value(_value) {}

  // Conversion to SIMD register
  TURBO_FORCE_INLINE operator const __m128i &() const {
    return this->value;
  }
  TURBO_FORCE_INLINE operator __m128i &() { return this->value; }

  // Bit operations
  TURBO_FORCE_INLINE Child operator|(const Child other) const {
    return vec_or(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE Child operator&(const Child other) const {
    return vec_and(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE Child operator^(const Child other) const {
    return vec_xor(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE Child bit_andnot(const Child other) const {
    return vec_andc(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE Child &operator|=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast | other;
    return *this_cast;
  }
  TURBO_FORCE_INLINE Child &operator&=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast & other;
    return *this_cast;
  }
  TURBO_FORCE_INLINE Child &operator^=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast ^ other;
    return *this_cast;
  }
};

// Forward-declared so they can be used by splat and friends.
template <typename T> struct simd8;

template <typename T, typename Mask = simd8<bool>>
struct base8 : base<simd8<T>> {
  typedef uint16_t bitmask_t;
  typedef uint32_t bitmask2_t;

  TURBO_FORCE_INLINE base8() : base<simd8<T>>() {}
  TURBO_FORCE_INLINE base8(const __m128i _value) : base<simd8<T>>(_value) {}

  TURBO_FORCE_INLINE Mask operator==(const simd8<T> other) const {
    return (__m128i)vec_cmpeq(this->value, (__m128i)other);
  }

  static const int SIZE = sizeof(base<simd8<T>>::value);

  template <int N = 1>
  TURBO_FORCE_INLINE simd8<T> prev(simd8<T> prev_chunk) const {
    __m128i chunk = this->value;
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve(this->value);
    prev_chunk = (__m128i)vec_reve((__m128i)prev_chunk);
#endif
    chunk = (__m128i)vec_sld((__m128i)prev_chunk, (__m128i)chunk, 16 - N);
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve((__m128i)chunk);
#endif
    return chunk;
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd8<bool> : base8<bool> {
  static TURBO_FORCE_INLINE simd8<bool> splat(bool _value) {
    return (__m128i)vec_splats((unsigned char)(-(!!_value)));
  }

  TURBO_FORCE_INLINE simd8<bool>() : base8() {}
  TURBO_FORCE_INLINE simd8<bool>(const __m128i _value)
      : base8<bool>(_value) {}
  // Splat constructor
  TURBO_FORCE_INLINE simd8<bool>(bool _value)
      : base8<bool>(splat(_value)) {}

  TURBO_FORCE_INLINE int to_bitmask() const {
    __vector unsigned long long result;
    const __m128i perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40,
                               0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00};

    result = ((__vector unsigned long long)vec_vbpermq((__m128i)this->value,
                                                       (__m128i)perm_mask));
#ifdef __LITTLE_ENDIAN__
    return static_cast<int>(result[1]);
#else
    return static_cast<int>(result[0]);
#endif
  }
  TURBO_FORCE_INLINE bool any() const {
    return !vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  TURBO_FORCE_INLINE simd8<bool> operator~() const {
    return this->value ^ (__m128i)splat(true);
  }
};

template <typename T> struct base8_numeric : base8<T> {
  static TURBO_FORCE_INLINE simd8<T> splat(T value) {
    (void)value;
    return (__m128i)vec_splats(value);
  }
  static TURBO_FORCE_INLINE simd8<T> zero() { return splat(0); }
  static TURBO_FORCE_INLINE simd8<T> load(const T values[16]) {
    return (__m128i)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(values)));
  }
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  static TURBO_FORCE_INLINE simd8<T> repeat_16(T v0, T v1, T v2, T v3, T v4,
                                                   T v5, T v6, T v7, T v8, T v9,
                                                   T v10, T v11, T v12, T v13,
                                                   T v14, T v15) {
    return simd8<T>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                    v14, v15);
  }

  TURBO_FORCE_INLINE base8_numeric() : base8<T>() {}
  TURBO_FORCE_INLINE base8_numeric(const __m128i _value)
      : base8<T>(_value) {}

  // Store to array
  TURBO_FORCE_INLINE void store(T dst[16]) const {
    vec_vsx_st(this->value, 0, reinterpret_cast<__m128i *>(dst));
  }

  // Override to distinguish from bool version
  TURBO_FORCE_INLINE simd8<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  TURBO_FORCE_INLINE simd8<T> operator+(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value + (__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<T> operator-(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value - (__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<T> &operator+=(const simd8<T> other) {
    *this = *this + other;
    return *static_cast<simd8<T> *>(this);
  }
  TURBO_FORCE_INLINE simd8<T> &operator-=(const simd8<T> other) {
    *this = *this - other;
    return *static_cast<simd8<T> *>(this);
  }

  // Perform a lookup assuming the value is between 0 and 16 (undefined behavior
  // for out of range values)
  template <typename L>
  TURBO_FORCE_INLINE simd8<L> lookup_16(simd8<L> lookup_table) const {
    return (__m128i)vec_perm((__m128i)lookup_table, (__m128i)lookup_table, this->value);
  }

  template <typename L>
  TURBO_FORCE_INLINE simd8<L>
  lookup_16(L replace0, L replace1, L replace2, L replace3, L replace4,
            L replace5, L replace6, L replace7, L replace8, L replace9,
            L replace10, L replace11, L replace12, L replace13, L replace14,
            L replace15) const {
    return lookup_16(simd8<L>::repeat_16(
        replace0, replace1, replace2, replace3, replace4, replace5, replace6,
        replace7, replace8, replace9, replace10, replace11, replace12,
        replace13, replace14, replace15));
  }
};

// Signed bytes
template <> struct simd8<int8_t> : base8_numeric<int8_t> {
  TURBO_FORCE_INLINE simd8() : base8_numeric<int8_t>() {}
  TURBO_FORCE_INLINE simd8(const __m128i _value)
      : base8_numeric<int8_t>(_value) {}

  // Splat constructor
  TURBO_FORCE_INLINE simd8(int8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd8(const int8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  TURBO_FORCE_INLINE simd8(int8_t v0, int8_t v1, int8_t v2, int8_t v3,
                               int8_t v4, int8_t v5, int8_t v6, int8_t v7,
                               int8_t v8, int8_t v9, int8_t v10, int8_t v11,
                               int8_t v12, int8_t v13, int8_t v14, int8_t v15)
      : simd8((__m128i)(__vector signed char){v0, v1, v2, v3, v4, v5, v6, v7,
                                              v8, v9, v10, v11, v12, v13, v14,
                                              v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  TURBO_FORCE_INLINE static simd8<int8_t>
  repeat_16(int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5,
            int8_t v6, int8_t v7, int8_t v8, int8_t v9, int8_t v10, int8_t v11,
            int8_t v12, int8_t v13, int8_t v14, int8_t v15) {
    return simd8<int8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                         v13, v14, v15);
  }

  // Order-sensitive comparisons
  TURBO_FORCE_INLINE simd8<int8_t>
  max_val(const simd8<int8_t> other) const {
    return (__m128i)vec_max((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<int8_t>
  min_val(const simd8<int8_t> other) const {
    return (__m128i)vec_min((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator>(const simd8<int8_t> other) const {
    return (__m128i)vec_cmpgt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator<(const simd8<int8_t> other) const {
    return (__m128i)vec_cmplt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
};

// Unsigned bytes
template <> struct simd8<uint8_t> : base8_numeric<uint8_t> {
  TURBO_FORCE_INLINE simd8() : base8_numeric<uint8_t>() {}
  TURBO_FORCE_INLINE simd8(const __m128i _value)
      : base8_numeric<uint8_t>(_value) {}
  // Splat constructor
  TURBO_FORCE_INLINE simd8(uint8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd8(const uint8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  TURBO_FORCE_INLINE
  simd8(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5,
        uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10,
        uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15)
      : simd8((__m128i){v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                        v13, v14, v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  TURBO_FORCE_INLINE static simd8<uint8_t>
  repeat_16(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4,
            uint8_t v5, uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9,
            uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14,
            uint8_t v15) {
    return simd8<uint8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                          v13, v14, v15);
  }

  // Saturated math
  TURBO_FORCE_INLINE simd8<uint8_t>
  saturating_add(const simd8<uint8_t> other) const {
    return (__m128i)vec_adds(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<uint8_t>
  saturating_sub(const simd8<uint8_t> other) const {
    return (__m128i)vec_subs(this->value, (__m128i)other);
  }

  // Order-specific operations
  TURBO_FORCE_INLINE simd8<uint8_t>
  max_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_max(this->value, (__m128i)other);
  }
  TURBO_FORCE_INLINE simd8<uint8_t>
  min_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_min(this->value, (__m128i)other);
  }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd8<uint8_t>
  gt_bits(const simd8<uint8_t> other) const {
    return this->saturating_sub(other);
  }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd8<uint8_t>
  lt_bits(const simd8<uint8_t> other) const {
    return other.saturating_sub(*this);
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator<=(const simd8<uint8_t> other) const {
    return other.max_val(*this) == other;
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator>=(const simd8<uint8_t> other) const {
    return other.min_val(*this) == other;
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator>(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }
  TURBO_FORCE_INLINE simd8<bool>
  operator<(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }

  // Bit-specific operations
  TURBO_FORCE_INLINE simd8<bool> bits_not_set() const {
    return (__m128i)vec_cmpeq(this->value, (__m128i)vec_splats(uint8_t(0)));
  }
  TURBO_FORCE_INLINE simd8<bool> bits_not_set(simd8<uint8_t> bits) const {
    return (*this & bits).bits_not_set();
  }
  TURBO_FORCE_INLINE simd8<bool> any_bits_set() const {
    return ~this->bits_not_set();
  }
  TURBO_FORCE_INLINE simd8<bool> any_bits_set(simd8<uint8_t> bits) const {
    return ~this->bits_not_set(bits);
  }

  TURBO_FORCE_INLINE bool is_ascii() const {
      return this->saturating_sub(0b01111111u).bits_not_set_anywhere();
  }

  TURBO_FORCE_INLINE bool bits_not_set_anywhere() const {
    return vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  TURBO_FORCE_INLINE bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }
  TURBO_FORCE_INLINE bool bits_not_set_anywhere(simd8<uint8_t> bits) const {
    return vec_all_eq(vec_and(this->value, (__m128i)bits),
                      (__m128i)vec_splats(0));
  }
  TURBO_FORCE_INLINE bool any_bits_set_anywhere(simd8<uint8_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> TURBO_FORCE_INLINE simd8<uint8_t> shr() const {
    return simd8<uint8_t>(
        (__m128i)vec_sr(this->value, (__m128i)vec_splat_u8(N)));
  }
  template <int N> TURBO_FORCE_INLINE simd8<uint8_t> shl() const {
    return simd8<uint8_t>(
        (__m128i)vec_sl(this->value, (__m128i)vec_splat_u8(N)));
  }
};

template <typename T> struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
  static_assert(NUM_CHUNKS == 4,
                "PPC64 kernel should use four registers per 64-byte block.");
  simd8<T> chunks[NUM_CHUNKS];

  simd8x64(const simd8x64<T> &o) = delete; // no copy allowed
  simd8x64<T> &
  operator=(const simd8<T> other) = delete; // no assignment allowed
  simd8x64() = delete;                      // no default constructor allowed

  TURBO_FORCE_INLINE simd8x64(const simd8<T> chunk0, const simd8<T> chunk1,
                                  const simd8<T> chunk2, const simd8<T> chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}

  TURBO_FORCE_INLINE simd8x64(const T* ptr) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+sizeof(simd8<T>)/sizeof(T)), simd8<T>::load(ptr+2*sizeof(simd8<T>)/sizeof(T)), simd8<T>::load(ptr+3*sizeof(simd8<T>)/sizeof(T))} {}

  TURBO_FORCE_INLINE void store(T* ptr) const {
    this->chunks[0].store(ptr + sizeof(simd8<T>) * 0/sizeof(T));
    this->chunks[1].store(ptr + sizeof(simd8<T>) * 1/sizeof(T));
    this->chunks[2].store(ptr + sizeof(simd8<T>) * 2/sizeof(T));
    this->chunks[3].store(ptr + sizeof(simd8<T>) * 3/sizeof(T));
  }


  TURBO_FORCE_INLINE simd8x64<T>& operator |=(const simd8x64<T> &other) {
      this->chunks[0] |= other.chunks[0];
      this->chunks[1] |= other.chunks[1];
      this->chunks[2] |= other.chunks[2];
      this->chunks[3] |= other.chunks[3];
      return *this;
    }

  TURBO_FORCE_INLINE simd8<T> reduce_or() const {
    return (this->chunks[0] | this->chunks[1]) |
           (this->chunks[2] | this->chunks[3]);
  }


  TURBO_FORCE_INLINE bool is_ascii() const {
    return input.reduce_or().is_ascii();
  }

  TURBO_FORCE_INLINE uint64_t to_bitmask() const {
    uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r1 = this->chunks[1].to_bitmask();
    uint64_t r2 = this->chunks[2].to_bitmask();
    uint64_t r3 = this->chunks[3].to_bitmask();
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  TURBO_FORCE_INLINE uint64_t eq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] == mask, this->chunks[1] == mask,
                          this->chunks[2] == mask, this->chunks[3] == mask)
        .to_bitmask();
  }

  TURBO_FORCE_INLINE uint64_t eq(const simd8x64<uint8_t> &other) const {
    return simd8x64<bool>(this->chunks[0] == other.chunks[0],
                          this->chunks[1] == other.chunks[1],
                          this->chunks[2] == other.chunks[2],
                          this->chunks[3] == other.chunks[3])
        .to_bitmask();
  }

  TURBO_FORCE_INLINE uint64_t lteq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask,
                          this->chunks[2] <= mask, this->chunks[3] <= mask)
        .to_bitmask();
  }

  TURBO_FORCE_INLINE uint64_t in_range(const T low, const T high) const {
      const simd8<T> mask_low = simd8<T>::splat(low);
      const simd8<T> mask_high = simd8<T>::splat(high);

      return  simd8x64<bool>(
        (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
        (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low),
        (this->chunks[2] <= mask_high) & (this->chunks[2] >= mask_low),
        (this->chunks[3] <= mask_high) & (this->chunks[3] >= mask_low)
      ).to_bitmask();
  }
  TURBO_FORCE_INLINE uint64_t not_in_range(const T low, const T high) const {
      const simd8<T> mask_low = simd8<T>::splat(low);
      const simd8<T> mask_high = simd8<T>::splat(high);
      return  simd8x64<bool>(
        (this->chunks[0] > mask_high) | (this->chunks[0] < mask_low),
        (this->chunks[1] > mask_high) | (this->chunks[1] < mask_low),
        (this->chunks[2] > mask_high) | (this->chunks[2] < mask_low),
        (this->chunks[3] > mask_high) | (this->chunks[3] < mask_low)
      ).to_bitmask();
  }
  TURBO_FORCE_INLINE uint64_t lt(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] < mask, this->chunks[1] < mask,
                          this->chunks[2] < mask, this->chunks[3] < mask)
        .to_bitmask();
  }

  TURBO_FORCE_INLINE uint64_t gt(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] > mask,
        this->chunks[1] > mask,
        this->chunks[2] > mask,
        this->chunks[3] > mask
      ).to_bitmask();
  }
  TURBO_FORCE_INLINE uint64_t gteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] >= mask,
        this->chunks[1] >= mask,
        this->chunks[2] >= mask,
        this->chunks[3] >= mask
      ).to_bitmask();
  }
  TURBO_FORCE_INLINE uint64_t gteq_unsigned(const uint8_t m) const {
      const simd8<uint8_t> mask = simd8<uint8_t>::splat(m);
      return  simd8x64<bool>(
        simd8<uint8_t>(this->chunks[0]) >= mask,
        simd8<uint8_t>(this->chunks[1]) >= mask,
        simd8<uint8_t>(this->chunks[2]) >= mask,
        simd8<uint8_t>(this->chunks[3]) >= mask
      ).to_bitmask();
  }
}; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#endif // TURBO_UNICODE_PPC64_SIMD_H_
