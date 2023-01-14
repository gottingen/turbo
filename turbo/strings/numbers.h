
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: numbers.h
// -----------------------------------------------------------------------------
//
// This package contains functions for converting strings to numbers. For
// converting numbers to strings, use `string_cat()` or `string_append()` in str_cat.h,
// which automatically detect and convert most number values appropriately.

#ifndef TURBO_STRINGS_NUMBERS_H_
#define TURBO_STRINGS_NUMBERS_H_

#ifdef __SSE4_2__
#include <x86intrin.h>
#endif

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <type_traits>

#ifdef __SSE4_2__
// TODO(yinbinli): Remove this when we figure out the right way
// to swap bytes on SSE 4.2 that works with the compilers
// we claim to support.  Also, add tests for the compiler
// that doesn't support the Intel _bswap64 intrinsic but
// does support all the SSE 4.2 intrinsics
#include "turbo/base/endian.h"
#endif

#include "turbo/base/profile.h"
#include "turbo/base/int128.h"
#include <string_view>
#include "turbo/base/math.h"

namespace turbo {


// simple_atoi()
//
// Converts the given string (optionally followed or preceded by ASCII
// whitespace) into an integer value, returning `true` if successful. The string
// must reflect a base-10 integer whose value falls within the range of the
// integer type (optionally preceded by a `+` or `-`). If any errors are
// encountered, this function returns `false`, leaving `out` in an unspecified
// state.
    template<typename int_type>
    TURBO_MUST_USE_RESULT bool simple_atoi(std::string_view str, int_type *out);

// simple_atof()
//
// Converts the given string (optionally followed or preceded by ASCII
// whitespace) into a float, which may be rounded on overflow or underflow,
// returning `true` if successful.
// See https://en.ccreference.com/w/c/string/byte/strtof for details about the
// allowed formats for `str`, except simple_atof() is locale-independent and will
// always use the "C" locale. If any errors are encountered, this function
// returns `false`, leaving `out` in an unspecified state.
    TURBO_MUST_USE_RESULT bool simple_atof(std::string_view str, float *out);

// simple_atod()
//
// Converts the given string (optionally followed or preceded by ASCII
// whitespace) into a double, which may be rounded on overflow or underflow,
// returning `true` if successful.
// See https://en.ccreference.com/w/c/string/byte/strtof for details about the
// allowed formats for `str`, except simple_atod is locale-independent and will
// always use the "C" locale. If any errors are encountered, this function
// returns `false`, leaving `out` in an unspecified state.
    TURBO_MUST_USE_RESULT bool simple_atod(std::string_view str, double *out);

// simple_atob()
//
// Converts the given string into a boolean, returning `true` if successful.
// The following case-insensitive strings are interpreted as boolean `true`:
// "true", "t", "yes", "y", "1". The following case-insensitive strings
// are interpreted as boolean `false`: "false", "f", "no", "n", "0". If any
// errors are encountered, this function returns `false`, leaving `out` in an
// unspecified state.
    TURBO_MUST_USE_RESULT bool simple_atob(std::string_view str, bool *out);


}  // namespace turbo

// End of public API.  Implementation details follow.

namespace turbo {

    namespace numbers_internal {

// Digit conversion.
        extern const char kHexChar[17];    // 0123456789abcdef
        extern const char kHexTable[513];  // 000102030405060708090a0b0c0d0e0f1011...
        extern const char two_ASCII_digits[100][2];  // 00, 01, 02, 03...

// Writes a two-character representation of 'i' to 'buf'. 'i' must be in the
// range 0 <= i < 100, and buf must have space for two characters. Example:
//   char buf[2];
//   put_two_digits(42, buf);
//   // buf[0] == '4'
//   // buf[1] == '2'
        TURBO_FORCE_INLINE void put_two_digits(size_t i, char *buf) {
            assert(i < 100);
            memcpy(buf, two_ASCII_digits[i], 2);
        }

// safe_strto?() functions for implementing simple_atoi()
        bool safe_strto32_base(std::string_view text, int32_t *value, int base);

        bool safe_strto64_base(std::string_view text, int64_t *value, int base);

        bool safe_strtou32_base(std::string_view text, uint32_t *value, int base);

        bool safe_strtou64_base(std::string_view text, uint64_t *value, int base);

        bool safe_strtou128_base(std::string_view text, turbo::uint128 *value,
                                 int base);

        static const int kFastToBufferSize = 32;
        static const int kSixDigitsToBufferSize = 16;

// Helper function for fast formatting of floating-point values.
// The result is the same as printf's "%g", a.k.a. "%.6g"; that is, six
// significant digits are returned, trailing zeros are removed, and numbers
// outside the range 0.0001-999999 are output using scientific notation
// (1.23456e+06). This routine is heavily optimized.
// Required buffer size is `kSixDigitsToBufferSize`.
        size_t six_digits_to_buffer(double d, char *buffer);

// These functions are intended for speed. All functions take an output buffer
// as an argument and return a pointer to the last byte they wrote, which is the
// terminating '\0'. At most `kFastToBufferSize` bytes are written.
        char *fast_int_to_buffer(int32_t, char *);

        char *fast_int_to_buffer(uint32_t, char *);

        char *fast_int_to_buffer(int64_t, char *);

        char *fast_int_to_buffer(uint64_t, char *);

// For enums and integer types that are not an exact match for the types above,
// use templates to call the appropriate one of the four overloads above.
        template<typename int_type>
        char *fast_int_to_buffer(int_type i, char *buffer) {
            static_assert(sizeof(i) <= 64 / 8,
                          "fast_int_to_buffer works only with 64-bit-or-less integers.");
            // TODO(yinbinli): This signed-ness check is used because it works correctly
            // with enums, and it also serves to check that int_type is not a pointer.
            // If one day something like std::is_signed<enum E> works, switch to it.
            if (static_cast<int_type>(1) - 2 < 0) {  // Signed
                if (sizeof(i) > 32 / 8) {           // 33-bit to 64-bit
                    return fast_int_to_buffer(static_cast<int64_t>(i), buffer);
                } else {  // 32-bit or less
                    return fast_int_to_buffer(static_cast<int32_t>(i), buffer);
                }
            } else {                     // Unsigned
                if (sizeof(i) > 32 / 8) {  // 33-bit to 64-bit
                    return fast_int_to_buffer(static_cast<uint64_t>(i), buffer);
                } else {  // 32-bit or less
                    return fast_int_to_buffer(static_cast<uint32_t>(i), buffer);
                }
            }
        }

// Implementation of simple_atoi, generalized to support arbitrary base (used
// with base different from 10 elsewhere in turbo implementation).
        template<typename int_type>
        TURBO_MUST_USE_RESULT bool safe_strtoi_base(std::string_view s, int_type *out,
                                                   int base) {
            static_assert(sizeof(*out) == 4 || sizeof(*out) == 8,
                          "simple_atoi works only with 32-bit or 64-bit integers.");
            static_assert(!std::is_floating_point<int_type>::value,
                          "Use simple_atof or simple_atod instead.");
            bool parsed;
            // TODO(yinbinli): This signed-ness check is used because it works correctly
            // with enums, and it also serves to check that int_type is not a pointer.
            // If one day something like std::is_signed<enum E> works, switch to it.
            if (static_cast<int_type>(1) - 2 < 0) {  // Signed
                if (sizeof(*out) == 64 / 8) {       // 64-bit
                    int64_t val;
                    parsed = numbers_internal::safe_strto64_base(s, &val, base);
                    *out = static_cast<int_type>(val);
                } else {  // 32-bit
                    int32_t val;
                    parsed = numbers_internal::safe_strto32_base(s, &val, base);
                    *out = static_cast<int_type>(val);
                }
            } else {                         // Unsigned
                if (sizeof(*out) == 64 / 8) {  // 64-bit
                    uint64_t val;
                    parsed = numbers_internal::safe_strtou64_base(s, &val, base);
                    *out = static_cast<int_type>(val);
                } else {  // 32-bit
                    uint32_t val;
                    parsed = numbers_internal::safe_strtou32_base(s, &val, base);
                    *out = static_cast<int_type>(val);
                }
            }
            return parsed;
        }

// fast_hex_to_buffer_zero_pad16()
//
// Outputs `val` into `out` as if by `snprintf(out, 17, "%016x", val)` but
// without the terminating null character. Thus `out` must be of length >= 16.
// Returns the number of non-pad digits of the output (it can never be zero
// since 0 has one digit).
        TURBO_FORCE_INLINE size_t
        fast_hex_to_buffer_zero_pad16(uint64_t
        val,
        char *out
        ) {
#ifdef __SSE4_2__
        uint64_t be = turbo::base::big_endian::from_host64(val);
        const auto kNibbleMask = _mm_set1_epi8(0xf);
        const auto kHexDigits = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7',
                                              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
        auto v = _mm_loadl_epi64(reinterpret_cast<__m128i*>(&be));  // load lo dword
        auto v4 = _mm_srli_epi64(v, 4);                            // shift 4 right
        auto il = _mm_unpacklo_epi8(v4, v);                        // interleave bytes
        auto m = _mm_and_si128(il, kNibbleMask);                   // mask out nibbles
        auto hexchars = _mm_shuffle_epi8(kHexDigits, m);           // hex chars
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out), hexchars);
#else
        for (
        int i = 0;
        i < 8; ++i) {
        auto byte = (val >> (56 - 8 * i)) & 0xFF;
        auto *hex = &turbo::numbers_internal::kHexTable[byte * 2];
        std::memcpy(out
        + 2 * i, hex, 2);
    }
#endif
    // | 0x1 so that even 0 has 1 digit.
    return 16 -
    turbo::base::countl_zero(val| 0x1) / 4;
}

}  // namespace numbers_internal

// simple_atoi()
//
// Converts a string to an integer, using `safe_strto?()` functions for actual
// parsing, returning `true` if successful. The `safe_strto?()` functions apply
// strict checking; the string must be a base-10 integer, optionally followed or
// preceded by ASCII whitespace, with a value in the range of the corresponding
// integer type.
template<typename int_type>
TURBO_MUST_USE_RESULT bool simple_atoi(std::string_view str, int_type *out) {
    return numbers_internal::safe_strtoi_base(str, out, 10);
}

TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE

bool simple_atoi(std::string_view str,
                 turbo::uint128 *out) {
    return numbers_internal::safe_strtou128_base(str, out, 10);
}


}  // namespace turbo

#endif  // TURBO_STRINGS_NUMBERS_H_
