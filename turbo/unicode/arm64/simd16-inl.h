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

template<typename T>
struct simd16;

  template<typename T, typename Mask=simd16<bool>>
  struct base_u16 {
    uint16x8_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    TURBO_FORCE_INLINE base_u16() = default;
    TURBO_FORCE_INLINE base_u16(const uint16x8_t _value) : value(_value) {}
    TURBO_FORCE_INLINE operator const uint16x8_t&() const { return this->value; }
    TURBO_FORCE_INLINE operator uint16x8_t&() { return this->value; }
    // Bit operations
    TURBO_FORCE_INLINE simd16<T> operator|(const simd16<T> other) const { return vorrq_u16(*this, other); }
    TURBO_FORCE_INLINE simd16<T> operator&(const simd16<T> other) const { return vandq_u16(*this, other); }
    TURBO_FORCE_INLINE simd16<T> operator^(const simd16<T> other) const { return veorq_u16(*this, other); }
    TURBO_FORCE_INLINE simd16<T> bit_andnot(const simd16<T> other) const { return vbicq_u16(*this, other); }
    TURBO_FORCE_INLINE simd16<T> operator~() const { return *this ^ 0xFFu; }
    TURBO_FORCE_INLINE simd16<T>& operator|=(const simd16<T> other) { auto this_cast = static_cast<simd16<T>*>(this); *this_cast = *this_cast | other; return *this_cast; }
    TURBO_FORCE_INLINE simd16<T>& operator&=(const simd16<T> other) { auto this_cast = static_cast<simd16<T>*>(this); *this_cast = *this_cast & other; return *this_cast; }
    TURBO_FORCE_INLINE simd16<T>& operator^=(const simd16<T> other) { auto this_cast = static_cast<simd16<T>*>(this); *this_cast = *this_cast ^ other; return *this_cast; }

    TURBO_FORCE_INLINE Mask operator==(const simd16<T> other) const { return vceqq_u16(*this, other); }

    template<int N=1>
    TURBO_FORCE_INLINE simd16<T> prev(const simd16<T> prev_chunk) const {
      return vextq_u18(prev_chunk, *this, 8 - N);
    }
  };

template<typename T, typename Mask=simd16<bool>>
struct base16: base_u16<T> {
  typedef uint16_t bitmask_t;
  typedef uint32_t bitmask2_t;

  TURBO_FORCE_INLINE base16() : base_u16<T>() {}
  TURBO_FORCE_INLINE base16(const uint16x8_t _value) : base_u16<T>(_value) {}
  template <typename Pointer>
  TURBO_FORCE_INLINE base16(const Pointer* ptr) : base16(vld1q_u16(ptr)) {}

  TURBO_FORCE_INLINE Mask operator==(const simd16<T> other) const { return vceqq_u16(*this, other); }

  static const int SIZE = sizeof(base_u16<T>::value);

  template<int N=1>
  TURBO_FORCE_INLINE simd16<T> prev(const simd16<T> prev_chunk) const {
    return vextq_u18(prev_chunk, *this, 8 - N);
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template<>
struct simd16<bool>: base16<bool> {
  static TURBO_FORCE_INLINE simd16<bool> splat(bool _value) { return vmovq_n_u16(uint16_t(-(!!_value))); }

  TURBO_FORCE_INLINE simd16<bool>() : base16() {}
  TURBO_FORCE_INLINE simd16<bool>(const uint16x8_t _value) : base16<bool>(_value) {}
  // Splat constructor
  TURBO_FORCE_INLINE simd16<bool>(bool _value) : base16<bool>(splat(_value)) {}

};

template<typename T>
struct base16_numeric: base16<T> {
  static TURBO_FORCE_INLINE simd16<T> splat(T _value) { return vmovq_n_u16(_value); }
  static TURBO_FORCE_INLINE simd16<T> zero() { return vdupq_n_u16(0); }
  static TURBO_FORCE_INLINE simd16<T> load(const T values[8]) {
    return vld1q_u16(reinterpret_cast<const uint16_t*>(values));
  }

  TURBO_FORCE_INLINE base16_numeric() : base16<T>() {}
  TURBO_FORCE_INLINE base16_numeric(const uint16x8_t _value) : base16<T>(_value) {}

  // Store to array
  TURBO_FORCE_INLINE void store(T dst[8]) const { return vst1q_u16(dst, *this); }

  // Override to distinguish from bool version
  TURBO_FORCE_INLINE simd16<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  TURBO_FORCE_INLINE simd16<T> operator+(const simd16<T> other) const { return vaddq_u8(*this, other); }
  TURBO_FORCE_INLINE simd16<T> operator-(const simd16<T> other) const { return vsubq_u8(*this, other); }
  TURBO_FORCE_INLINE simd16<T>& operator+=(const simd16<T> other) { *this = *this + other; return *static_cast<simd16<T>*>(this); }
  TURBO_FORCE_INLINE simd16<T>& operator-=(const simd16<T> other) { *this = *this - other; return *static_cast<simd16<T>*>(this); }
};

// Signed words
template<>
struct simd16<int16_t> : base16_numeric<int16_t> {
  TURBO_FORCE_INLINE simd16() : base16_numeric<int16_t>() {}
#ifndef SIMDUTF_REGULAR_VISUAL_STUDIO
  TURBO_FORCE_INLINE simd16(const uint16x8_t _value) : base16_numeric<int16_t>(_value) {}
#endif
  TURBO_FORCE_INLINE simd16(const int16x8_t _value) : base16_numeric<int16_t>(vreinterpretq_u16_s16(_value)) {}

  // Splat constructor
  TURBO_FORCE_INLINE simd16(int16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd16(const int16_t* values) : simd16(load(values)) {}
  TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const int16_t*>(values))) {}
  TURBO_FORCE_INLINE operator simd16<uint16_t>() const;
  TURBO_FORCE_INLINE operator const uint16x8_t&() const { return this->value; }
  TURBO_FORCE_INLINE operator const int16x8_t() const { return vreinterpretq_s16_u16(this->value); }

  TURBO_FORCE_INLINE int16_t max_val() const { return vmaxvq_s16(vreinterpretq_s16_u16(this->value)); }
  TURBO_FORCE_INLINE int16_t min_val() const { return vminvq_s16(vreinterpretq_s16_u16(this->value)); }
  // Order-sensitive comparisons
  TURBO_FORCE_INLINE simd16<int16_t> max_val(const simd16<int16_t> other) const { return vmaxq_s16(vreinterpretq_s16_u16(this->value), vreinterpretq_s16_u16(other.value)); }
  TURBO_FORCE_INLINE simd16<int16_t> min_val(const simd16<int16_t> other) const { return vmaxq_s16(vreinterpretq_s16_u16(this->value), vreinterpretq_s16_u16(other.value)); }
  TURBO_FORCE_INLINE simd16<bool> operator>(const simd16<int16_t> other) const { return vcgtq_s16(vreinterpretq_s16_u16(this->value), vreinterpretq_s16_u16(other.value)); }
  TURBO_FORCE_INLINE simd16<bool> operator<(const simd16<int16_t> other) const { return vcltq_s16(vreinterpretq_s16_u16(this->value), vreinterpretq_s16_u16(other.value)); }
};




// Unsigned words
template<>
struct simd16<uint16_t>: base16_numeric<uint16_t>  {
  TURBO_FORCE_INLINE simd16() : base16_numeric<uint16_t>() {}
  TURBO_FORCE_INLINE simd16(const uint16x8_t _value) : base16_numeric<uint16_t>(_value) {}

  // Splat constructor
  TURBO_FORCE_INLINE simd16(uint16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd16(const uint16_t* values) : simd16(load(values)) {}
  TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const uint16_t*>(values))) {}


  TURBO_FORCE_INLINE int16_t max_val() const { return vmaxvq_u16(*this); }
  TURBO_FORCE_INLINE int16_t min_val() const { return vminvq_u16(*this); }
  // Saturated math
  TURBO_FORCE_INLINE simd16<uint16_t> saturating_add(const simd16<uint16_t> other) const { return vqaddq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> saturating_sub(const simd16<uint16_t> other) const { return vqsubq_u16(*this, other); }

  // Order-specific operations
  TURBO_FORCE_INLINE simd16<uint16_t> max_val(const simd16<uint16_t> other) const { return vmaxq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> min_val(const simd16<uint16_t> other) const { return vminq_u16(*this, other); }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd16<uint16_t> gt_bits(const simd16<uint16_t> other) const { return this->saturating_sub(other); }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd16<uint16_t> lt_bits(const simd16<uint16_t> other) const { return other.saturating_sub(*this); }
  TURBO_FORCE_INLINE simd16<bool> operator<=(const simd16<uint16_t> other) const { return vcleq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<bool> operator>=(const simd16<uint16_t> other) const { return vcgeq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<bool> operator>(const simd16<uint16_t> other) const { return  vcgtq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<bool> operator<(const simd16<uint16_t> other) const { return vcltq_u16(*this, other); }

  // Bit-specific operations
  TURBO_FORCE_INLINE simd16<bool> bits_not_set() const { return *this == uint16_t(0); }
  template<int N>
  TURBO_FORCE_INLINE simd16<uint16_t> shr() const { return simd16<uint16_t>(vshrq_n_u16(*this, N)); }
  template<int N>
  TURBO_FORCE_INLINE simd16<uint16_t> shl() const { return simd16<uint16_t>(vshlq_n_u16(*this, N)); }

  // logical operations
  TURBO_FORCE_INLINE simd16<uint16_t> operator|(const simd16<uint16_t> other) const { return vorrq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> operator&(const simd16<uint16_t> other) const { return vandq_u16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> operator^(const simd16<uint16_t> other) const { return veorq_u16(*this, other); }

  // Pack with the unsigned saturation  two uint16_t words into single uint8_t vector
  static TURBO_FORCE_INLINE simd8<uint8_t> pack(const simd16<uint16_t>& v0, const simd16<uint16_t>& v1) {
    return vqmovn_high_u16(vqmovn_u16(v0), v1);
  }

  // Change the endianness
  TURBO_FORCE_INLINE simd16<uint16_t> swap_bytes() const {
    #ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
    const uint8x16_t swap = make_uint8x16_t(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
    #else
    const uint8x16_t swap = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};
    #endif
    return vreinterpretq_u16_u8(vqtbl1q_u8(vreinterpretq_u8_u16(*this), swap));
  }
};
TURBO_FORCE_INLINE simd16<int16_t>::operator simd16<uint16_t>() const { return this->value; }


  template<typename T>
  struct simd16x32 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd16<T>);
    static_assert(NUM_CHUNKS == 4, "ARM kernel should use four registers per 64-byte block.");
    simd16<T> chunks[NUM_CHUNKS];

    simd16x32(const simd16x32<T>& o) = delete; // no copy allowed
    simd16x32<T>& operator=(const simd16<T> other) = delete; // no assignment allowed
    simd16x32() = delete; // no default constructor allowed

    TURBO_FORCE_INLINE simd16x32(const simd16<T> chunk0, const simd16<T> chunk1, const simd16<T> chunk2, const simd16<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    TURBO_FORCE_INLINE simd16x32(const T* ptr) : chunks{simd16<T>::load(ptr), simd16<T>::load(ptr+sizeof(simd16<T>)/sizeof(T)), simd16<T>::load(ptr+2*sizeof(simd16<T>)/sizeof(T)), simd16<T>::load(ptr+3*sizeof(simd16<T>)/sizeof(T))} {}

    TURBO_FORCE_INLINE void store(T* ptr) const {
      this->chunks[0].store(ptr+sizeof(simd16<T>)*0/sizeof(T));
      this->chunks[1].store(ptr+sizeof(simd16<T>)*1/sizeof(T));
      this->chunks[2].store(ptr+sizeof(simd16<T>)*2/sizeof(T));
      this->chunks[3].store(ptr+sizeof(simd16<T>)*3/sizeof(T));
    }

    TURBO_FORCE_INLINE simd16<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }

    TURBO_FORCE_INLINE bool is_ascii() const {
      return reduce_or().is_ascii();
    }

    TURBO_FORCE_INLINE void store_ascii_as_utf16(char16_t * ptr) const {
      this->chunks[0].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*0);
      this->chunks[1].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*1);
      this->chunks[2].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*2);
      this->chunks[3].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*3);
    }

    TURBO_FORCE_INLINE uint64_t to_bitmask() const {
#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask = make_uint8x16_t(
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      );
#else
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
#endif
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(vreinterpretq_u8_u16(this->chunks[0] & vreinterpretq_u16_u8(bit_mask)), vreinterpretq_u8_u16(this->chunks[1] & vreinterpretq_u16_u8(bit_mask)));
      uint8x16_t sum1 = vpaddq_u8(vreinterpretq_u8_u16(this->chunks[2] & vreinterpretq_u16_u8(bit_mask)), vreinterpretq_u8_u16(this->chunks[3] & vreinterpretq_u16_u8(bit_mask)));
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    TURBO_FORCE_INLINE void swap_bytes() {
      this->chunks[0] = this->chunks[0].swap_bytes();
      this->chunks[1] = this->chunks[1].swap_bytes();
      this->chunks[2] = this->chunks[2].swap_bytes();
      this->chunks[3] = this->chunks[3].swap_bytes();
    }

    TURBO_FORCE_INLINE uint64_t eq(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return  simd16x32<bool>(
      this->chunks[0] == mask,
      this->chunks[1] == mask,
      this->chunks[2] == mask,
      this->chunks[3] == mask
    ).to_bitmask();
  }

  TURBO_FORCE_INLINE uint64_t lteq(const T m) const {
    const simd16<T> mask = simd16<T>::splat(m);
    return  simd16x32<bool>(
      this->chunks[0] <= mask,
      this->chunks[1] <= mask,
      this->chunks[2] <= mask,
      this->chunks[3] <= mask
    ).to_bitmask();
  }

    TURBO_FORCE_INLINE uint64_t in_range(const T low, const T high) const {
      const simd16<T> mask_low = simd16<T>::splat(low);
      const simd16<T> mask_high = simd16<T>::splat(high);

      return  simd16x32<bool>(
        (this->chunks[0] <= mask_high) & (this->chunks[0] >= mask_low),
        (this->chunks[1] <= mask_high) & (this->chunks[1] >= mask_low),
        (this->chunks[2] <= mask_high) & (this->chunks[2] >= mask_low),
        (this->chunks[3] <= mask_high) & (this->chunks[3] >= mask_low)
      ).to_bitmask();
    }
    TURBO_FORCE_INLINE uint64_t not_in_range(const T low, const T high) const {
      const simd16<T> mask_low = simd16<T>::splat(low);
      const simd16<T> mask_high = simd16<T>::splat(high);
      return  simd16x32<bool>(
        (this->chunks[0] > mask_high) | (this->chunks[0] < mask_low),
        (this->chunks[1] > mask_high) | (this->chunks[1] < mask_low),
        (this->chunks[2] > mask_high) | (this->chunks[2] < mask_low),
        (this->chunks[3] > mask_high) | (this->chunks[3] < mask_low)
      ).to_bitmask();
    }
    TURBO_FORCE_INLINE uint64_t lt(const T m) const {
      const simd16<T> mask = simd16<T>::splat(m);
      return  simd16x32<bool>(
        this->chunks[0] < mask,
        this->chunks[1] < mask,
        this->chunks[2] < mask,
        this->chunks[3] < mask
      ).to_bitmask();
    }

  }; // struct simd16x32<T>
  template<>
  TURBO_FORCE_INLINE uint64_t simd16x32<uint16_t>::not_in_range(const uint16_t low, const uint16_t high) const {
      const simd16<uint16_t> mask_low = simd16<uint16_t>::splat(low);
      const simd16<uint16_t> mask_high = simd16<uint16_t>::splat(high);
      simd16x32<uint16_t> x(
        simd16<uint16_t>((this->chunks[0] > mask_high) | (this->chunks[0] < mask_low)),
        simd16<uint16_t>((this->chunks[1] > mask_high) | (this->chunks[1] < mask_low)),
        simd16<uint16_t>((this->chunks[2] > mask_high) | (this->chunks[2] < mask_low)),
        simd16<uint16_t>((this->chunks[3] > mask_high) | (this->chunks[3] < mask_low))
      );
      return  x.to_bitmask();
    }
