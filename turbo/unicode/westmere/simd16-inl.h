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
struct base16: base<simd16<T>> {
  typedef uint16_t bitmask_t;
  typedef uint32_t bitmask2_t;

  TURBO_FORCE_INLINE base16() : base<simd16<T>>() {}
  TURBO_FORCE_INLINE base16(const __m128i _value) : base<simd16<T>>(_value) {}
  template <typename Pointer>
  TURBO_FORCE_INLINE base16(const Pointer* ptr) : base16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr))) {}

  TURBO_FORCE_INLINE Mask operator==(const simd16<T> other) const { return _mm_cmpeq_epi16(*this, other); }

  static const int SIZE = sizeof(base<simd16<T>>::value);

  template<int N=1>
  TURBO_FORCE_INLINE simd16<T> prev(const simd16<T> prev_chunk) const {
    return _mm_alignr_epi8(*this, prev_chunk, 16 - N);
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template<>
struct simd16<bool>: base16<bool> {
  static TURBO_FORCE_INLINE simd16<bool> splat(bool _value) { return _mm_set1_epi16(uint16_t(-(!!_value))); }

  TURBO_FORCE_INLINE simd16<bool>() : base16() {}
  TURBO_FORCE_INLINE simd16<bool>(const __m128i _value) : base16<bool>(_value) {}
  // Splat constructor
  TURBO_FORCE_INLINE simd16<bool>(bool _value) : base16<bool>(splat(_value)) {}

  TURBO_FORCE_INLINE int to_bitmask() const { return _mm_movemask_epi8(*this); }
  TURBO_FORCE_INLINE bool any() const { return !_mm_testz_si128(*this, *this); }
  TURBO_FORCE_INLINE simd16<bool> operator~() const { return *this ^ true; }
};

template<typename T>
struct base16_numeric: base16<T> {
  static TURBO_FORCE_INLINE simd16<T> splat(T _value) { return _mm_set1_epi16(_value); }
  static TURBO_FORCE_INLINE simd16<T> zero() { return _mm_setzero_si128(); }
  static TURBO_FORCE_INLINE simd16<T> load(const T values[8]) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i *>(values));
  }

  TURBO_FORCE_INLINE base16_numeric() : base16<T>() {}
  TURBO_FORCE_INLINE base16_numeric(const __m128i _value) : base16<T>(_value) {}

  // Store to array
  TURBO_FORCE_INLINE void store(T dst[8]) const { return _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), *this); }

  // Override to distinguish from bool version
  TURBO_FORCE_INLINE simd16<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  TURBO_FORCE_INLINE simd16<T> operator+(const simd16<T> other) const { return _mm_add_epi16(*this, other); }
  TURBO_FORCE_INLINE simd16<T> operator-(const simd16<T> other) const { return _mm_sub_epi16(*this, other); }
  TURBO_FORCE_INLINE simd16<T>& operator+=(const simd16<T> other) { *this = *this + other; return *static_cast<simd16<T>*>(this); }
  TURBO_FORCE_INLINE simd16<T>& operator-=(const simd16<T> other) { *this = *this - other; return *static_cast<simd16<T>*>(this); }
};

// Signed words
template<>
struct simd16<int16_t> : base16_numeric<int16_t> {
  TURBO_FORCE_INLINE simd16() : base16_numeric<int16_t>() {}
  TURBO_FORCE_INLINE simd16(const __m128i _value) : base16_numeric<int16_t>(_value) {}
  // Splat constructor
  TURBO_FORCE_INLINE simd16(int16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd16(const int16_t* values) : simd16(load(values)) {}
  TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const int16_t*>(values))) {}
  // Member-by-member initialization
  TURBO_FORCE_INLINE simd16(
    int16_t v0, int16_t v1, int16_t v2, int16_t v3, int16_t v4, int16_t v5, int16_t v6, int16_t v7)
    : simd16(_mm_setr_epi16(v0, v1, v2, v3, v4, v5, v6, v7)) {}
  TURBO_FORCE_INLINE operator simd16<uint16_t>() const;

  // Order-sensitive comparisons
  TURBO_FORCE_INLINE simd16<int16_t> max_val(const simd16<int16_t> other) const { return _mm_max_epi16(*this, other); }
  TURBO_FORCE_INLINE simd16<int16_t> min_val(const simd16<int16_t> other) const { return _mm_min_epi16(*this, other); }
  TURBO_FORCE_INLINE simd16<bool> operator>(const simd16<int16_t> other) const { return _mm_cmpgt_epi16(*this, other); }
  TURBO_FORCE_INLINE simd16<bool> operator<(const simd16<int16_t> other) const { return _mm_cmpgt_epi16(other, *this); }
};

// Unsigned words
template<>
struct simd16<uint16_t>: base16_numeric<uint16_t>  {
  TURBO_FORCE_INLINE simd16() : base16_numeric<uint16_t>() {}
  TURBO_FORCE_INLINE simd16(const __m128i _value) : base16_numeric<uint16_t>(_value) {}

  // Splat constructor
  TURBO_FORCE_INLINE simd16(uint16_t _value) : simd16(splat(_value)) {}
  // Array constructor
  TURBO_FORCE_INLINE simd16(const uint16_t* values) : simd16(load(values)) {}
  TURBO_FORCE_INLINE simd16(const char16_t* values) : simd16(load(reinterpret_cast<const uint16_t*>(values))) {}
  // Member-by-member initialization
  TURBO_FORCE_INLINE simd16(
    uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4, uint16_t v5, uint16_t v6, uint16_t v7)
  : simd16(_mm_setr_epi16(v0, v1, v2, v3, v4, v5, v6, v7)) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  TURBO_FORCE_INLINE static simd16<uint16_t> repeat_16(
    uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4, uint16_t v5, uint16_t v6, uint16_t v7
  ) {
    return simd16<uint16_t>(v0, v1, v2, v3, v4, v5, v6, v7);
  }

  // Saturated math
  TURBO_FORCE_INLINE simd16<uint16_t> saturating_add(const simd16<uint16_t> other) const { return _mm_adds_epu16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> saturating_sub(const simd16<uint16_t> other) const { return _mm_subs_epu16(*this, other); }

  // Order-specific operations
  TURBO_FORCE_INLINE simd16<uint16_t> max_val(const simd16<uint16_t> other) const { return _mm_max_epu16(*this, other); }
  TURBO_FORCE_INLINE simd16<uint16_t> min_val(const simd16<uint16_t> other) const { return _mm_min_epu16(*this, other); }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd16<uint16_t> gt_bits(const simd16<uint16_t> other) const { return this->saturating_sub(other); }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  TURBO_FORCE_INLINE simd16<uint16_t> lt_bits(const simd16<uint16_t> other) const { return other.saturating_sub(*this); }
  TURBO_FORCE_INLINE simd16<bool> operator<=(const simd16<uint16_t> other) const { return other.max_val(*this) == other; }
  TURBO_FORCE_INLINE simd16<bool> operator>=(const simd16<uint16_t> other) const { return other.min_val(*this) == other; }
  TURBO_FORCE_INLINE simd16<bool> operator>(const simd16<uint16_t> other) const { return this->gt_bits(other).any_bits_set(); }
  TURBO_FORCE_INLINE simd16<bool> operator<(const simd16<uint16_t> other) const { return this->gt_bits(other).any_bits_set(); }

  // Bit-specific operations
  TURBO_FORCE_INLINE simd16<bool> bits_not_set() const { return *this == uint16_t(0); }
  TURBO_FORCE_INLINE simd16<bool> bits_not_set(simd16<uint16_t> bits) const { return (*this & bits).bits_not_set(); }
  TURBO_FORCE_INLINE simd16<bool> any_bits_set() const { return ~this->bits_not_set(); }
  TURBO_FORCE_INLINE simd16<bool> any_bits_set(simd16<uint16_t> bits) const { return ~this->bits_not_set(bits); }

  TURBO_FORCE_INLINE bool bits_not_set_anywhere() const { return _mm_testz_si128(*this, *this); }
  TURBO_FORCE_INLINE bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
  TURBO_FORCE_INLINE bool bits_not_set_anywhere(simd16<uint16_t> bits) const { return _mm_testz_si128(*this, bits); }
  TURBO_FORCE_INLINE bool any_bits_set_anywhere(simd16<uint16_t> bits) const { return !bits_not_set_anywhere(bits); }
  template<int N>
  TURBO_FORCE_INLINE simd16<uint16_t> shr() const { return simd16<uint16_t>(_mm_srli_epi16(*this, N)); }
  template<int N>
  TURBO_FORCE_INLINE simd16<uint16_t> shl() const { return simd16<uint16_t>(_mm_slli_epi16(*this, N)); }
  // Get one of the bits and make a bitmask out of it.
  // e.g. value.get_bit<7>() gets the high bit
  template<int N>
  TURBO_FORCE_INLINE int get_bit() const { return _mm_movemask_epi8(_mm_slli_epi16(*this, 7-N)); }

  // Change the endianness
  TURBO_FORCE_INLINE simd16<uint16_t> swap_bytes() const {
    const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
    return _mm_shuffle_epi8(*this, swap);
  }

  // Pack with the unsigned saturation  two uint16_t words into single uint8_t vector
  static TURBO_FORCE_INLINE simd8<uint8_t> pack(const simd16<uint16_t>& v0, const simd16<uint16_t>& v1) {
    return _mm_packus_epi16(v0, v1);
  }
};
TURBO_FORCE_INLINE simd16<int16_t>::operator simd16<uint16_t>() const { return this->value; }

template<typename T>
  struct simd16x32 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd16<T>);
    static_assert(NUM_CHUNKS == 4, "Westmere kernel should use four registers per 64-byte block.");
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
      return this->reduce_or().is_ascii();
    }

    TURBO_FORCE_INLINE void store_ascii_as_utf16(char16_t * ptr) const {
      this->chunks[0].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*0);
      this->chunks[1].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*1);
      this->chunks[2].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*2);
      this->chunks[3].store_ascii_as_utf16(ptr+sizeof(simd16<T>)*3);
    }

    TURBO_FORCE_INLINE uint64_t to_bitmask() const {
      uint64_t r0 = uint32_t(this->chunks[0].to_bitmask() );
      uint64_t r1 =          this->chunks[1].to_bitmask() ;
      uint64_t r2 =          this->chunks[2].to_bitmask() ;
      uint64_t r3 =          this->chunks[3].to_bitmask() ;
      return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
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

    TURBO_FORCE_INLINE uint64_t eq(const simd16x32<uint16_t> &other) const {
      return  simd16x32<bool>(
        this->chunks[0] == other.chunks[0],
        this->chunks[1] == other.chunks[1],
        this->chunks[2] == other.chunks[2],
        this->chunks[3] == other.chunks[3]
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
      const simd16<T> mask_low = simd16<T>::splat(static_cast<T>(low-1));
      const simd16<T> mask_high = simd16<T>::splat(static_cast<T>(high+1));
      return simd16x32<bool>(
        (this->chunks[0] >= mask_high) | (this->chunks[0] <= mask_low),
        (this->chunks[1] >= mask_high) | (this->chunks[1] <= mask_low),
        (this->chunks[2] >= mask_high) | (this->chunks[2] <= mask_low),
        (this->chunks[3] >= mask_high) | (this->chunks[3] <= mask_low)
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