
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: str_cat.h
// -----------------------------------------------------------------------------
//
// This package contains functions for efficiently concatenating and appending
// strings: `string_cat()` and `string_append()`. Most of the work within these routines
// is actually handled through use of a special alpha_num type, which was
// designed to be used as a parameter type that efficiently manages conversion
// to strings and avoids copies in the above operations.
//
// Any routine accepting either a string or a number may accept `alpha_num`.
// The basic idea is that by accepting a `const alpha_num &` as an argument
// to your function, your callers will automagically convert bools, integers,
// and floating point values to strings for you.
//
// NOTE: Use of `alpha_num` outside of the //flare/strings package is unsupported
// except for the specific case of function parameters of type `alpha_num` or
// `const alpha_num &`. In particular, instantiating `alpha_num` directly as a
// stack variable is not supported.
//
// Conversion from 8-bit values is not accepted because, if it were, then an
// attempt to pass ':' instead of ":" might result in a 58 ending up in your
// result.
//
// Bools convert to "0" or "1". Pointers to types other than `char *` are not
// valid inputs. No output is generated for null `char *` pointers.
//
// Floating point numbers are formatted with six-digit precision, which is
// the default for "std::cout <<" or printf "%g" (the same as "%.6g").
//
// You can convert to hexadecimal output rather than decimal output using the
// `hex` type contained here. To do so, pass `hex(my_int)` as a parameter to
// `string_cat()` or `string_append()`. You may specify a minimum hex field width using
// a `pad_spec` enum.
//
// -----------------------------------------------------------------------------

#ifndef FLARE_STRINGS_STR_CAT_H_
#define FLARE_STRINGS_STR_CAT_H_

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
#include "flare/base/profile.h"
#include "flare/strings/numbers.h"
#include <string_view>

namespace flare {


namespace strings_internal {
// alpha_num_buffer allows a way to pass a string to string_cat without having to do
// memory allocation.  It is simply a pair of a fixed-size character array, and
// a size.  Please don't use outside of flare, yet.
template<size_t max_size>
struct alpha_num_buffer {
    std::array<char, max_size> data;
    size_t size;
};

}  // namespace strings_internal

// Enum that specifies the number of significant digits to return in a `hex` or
// `dec` conversion and fill character to use. A `kZeroPad2` value, for example,
// would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
// would produce hexadecimal strings such as "    a","    f".
enum pad_spec : uint8_t {
    kNoPad = 1,
    kZeroPad2,
    kZeroPad3,
    kZeroPad4,
    kZeroPad5,
    kZeroPad6,
    kZeroPad7,
    kZeroPad8,
    kZeroPad9,
    kZeroPad10,
    kZeroPad11,
    kZeroPad12,
    kZeroPad13,
    kZeroPad14,
    kZeroPad15,
    kZeroPad16,
    kZeroPad17,
    kZeroPad18,
    kZeroPad19,
    kZeroPad20,

    kSpacePad2 = kZeroPad2 + 64,
    kSpacePad3,
    kSpacePad4,
    kSpacePad5,
    kSpacePad6,
    kSpacePad7,
    kSpacePad8,
    kSpacePad9,
    kSpacePad10,
    kSpacePad11,
    kSpacePad12,
    kSpacePad13,
    kSpacePad14,
    kSpacePad15,
    kSpacePad16,
    kSpacePad17,
    kSpacePad18,
    kSpacePad19,
    kSpacePad20,
};

// -----------------------------------------------------------------------------
// hex
// -----------------------------------------------------------------------------
//
// `hex` stores a set of hexadecimal string conversion parameters for use
// within `alpha_num` string conversions.
struct hex {
    uint64_t value;
    uint8_t width;
    char fill;

    template<typename Int>
    explicit hex(
            Int v, pad_spec spec = flare::kNoPad,
            typename std::enable_if<sizeof(Int) == 1 &&
                                    !std::is_pointer<Int>::value>::type * = nullptr)
            : hex(spec, static_cast<uint8_t>(v)) {}

    template<typename Int>
    explicit hex(
            Int v, pad_spec spec = flare::kNoPad,
            typename std::enable_if<sizeof(Int) == 2 &&
                                    !std::is_pointer<Int>::value>::type * = nullptr)
            : hex(spec, static_cast<uint16_t>(v)) {}

    template<typename Int>
    explicit hex(
            Int v, pad_spec spec = flare::kNoPad,
            typename std::enable_if<sizeof(Int) == 4 &&
                                    !std::is_pointer<Int>::value>::type * = nullptr)
            : hex(spec, static_cast<uint32_t>(v)) {}

    template<typename Int>
    explicit hex(
            Int v, pad_spec spec = flare::kNoPad,
            typename std::enable_if<sizeof(Int) == 8 &&
                                    !std::is_pointer<Int>::value>::type * = nullptr)
            : hex(spec, static_cast<uint64_t>(v)) {}

    template<typename Pointee>
    explicit hex(Pointee *v, pad_spec spec = flare::kNoPad)
            : hex(spec, reinterpret_cast<uintptr_t>(v)) {}

  private:
    hex(pad_spec spec, uint64_t v)
            : value(v),
              width(spec == flare::kNoPad
                    ? 1
                    : spec >= flare::kSpacePad2 ? spec - flare::kSpacePad2 + 2
                                               : spec - flare::kZeroPad2 + 2),
              fill(spec >= flare::kSpacePad2 ? ' ' : '0') {}
};

// -----------------------------------------------------------------------------
// dec
// -----------------------------------------------------------------------------
//
// `dec` stores a set of decimal string conversion parameters for use
// within `alpha_num` string conversions.  dec is slower than the default
// integer conversion, so use it only if you need padding.
struct dec {
    uint64_t value;
    uint8_t width;
    char fill;
    bool neg;

    template<typename Int>
    explicit dec(Int v, pad_spec spec = flare::kNoPad,
                 typename std::enable_if<(sizeof(Int) <= 8)>::type * = nullptr)
            : value(v >= 0 ? static_cast<uint64_t>(v)
                           : uint64_t{0} - static_cast<uint64_t>(v)),
              width(spec == flare::kNoPad
                    ? 1
                    : spec >= flare::kSpacePad2 ? spec - flare::kSpacePad2 + 2
                                               : spec - flare::kZeroPad2 + 2),
              fill(spec >= flare::kSpacePad2 ? ' ' : '0'),
              neg(v < 0) {}
};

// -----------------------------------------------------------------------------
// alpha_num
// -----------------------------------------------------------------------------
//
// The `alpha_num` class acts as the main parameter type for `string_cat()` and
// `string_append()`, providing efficient conversion of numeric, boolean, and
// hexadecimal values (through the `hex` type) into strings.

class alpha_num {
  public:
    // No bool ctor -- bools convert to an integral type.
    // A bool ctor would also convert incoming pointers (bletch).

    alpha_num(int x)  // NOLINT(runtime/explicit)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(unsigned int x)  // NOLINT(runtime/explicit)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(long x)  // NOLINT(*)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(unsigned long x)  // NOLINT(*)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(long long x)  // NOLINT(*)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(unsigned long long x)  // NOLINT(*)
            : piece_(digits_,
                     numbers_internal::fast_int_to_buffer(x, digits_) - &digits_[0]) {}

    alpha_num(float f)  // NOLINT(runtime/explicit)
            : piece_(digits_, numbers_internal::six_digits_to_buffer(f, digits_)) {}

    alpha_num(double f)  // NOLINT(runtime/explicit)
            : piece_(digits_, numbers_internal::six_digits_to_buffer(f, digits_)) {}

    alpha_num(hex h);  // NOLINT(runtime/explicit)
    alpha_num(dec d);  // NOLINT(runtime/explicit)

    template<size_t size>
    alpha_num(  // NOLINT(runtime/explicit)
            const strings_internal::alpha_num_buffer<size> &buf)
            : piece_(&buf.data[0], buf.size) {}

    alpha_num(const char *c_str) : piece_(c_str) {}  // NOLINT(runtime/explicit)
    alpha_num(std::string_view pc) : piece_(pc) {}  // NOLINT(runtime/explicit)

    template<typename Allocator>
    alpha_num(  // NOLINT(runtime/explicit)
            const std::basic_string<char, std::char_traits<char>, Allocator> &str)
            : piece_(str) {}

    // Use std::string literals ":" instead of character literals ':'.
    alpha_num(char c) = delete;  // NOLINT(runtime/explicit)

    alpha_num(const alpha_num &) = delete;

    alpha_num &operator=(const alpha_num &) = delete;

    std::string_view::size_type size() const { return piece_.size(); }

    const char *data() const { return piece_.data(); }

    std::string_view Piece() const { return piece_; }

    // Normal enums are already handled by the integer formatters.
    // This overload matches only scoped enums.
    template<typename T,
            typename = typename std::enable_if<
                    std::is_enum<T>{} && !std::is_convertible<T, int>{}>::type>
    alpha_num(T e)  // NOLINT(runtime/explicit)
            : alpha_num(static_cast<typename std::underlying_type<T>::type>(e)) {}

    // vector<bool>::reference and const_reference require special help to
    // convert to `alpha_num` because it requires two user defined conversions.
    template<
            typename T,
            typename std::enable_if<
                    std::is_class<T>::value &&
                    (std::is_same<T, std::vector<bool>::reference>::value ||
                     std::is_same<T, std::vector<bool>::const_reference>::value)>::type * =
            nullptr>
    alpha_num(T e) : alpha_num(static_cast<bool>(e)) {}  // NOLINT(runtime/explicit)

  private:
    std::string_view piece_;
    char digits_[numbers_internal::kFastToBufferSize];
};

// -----------------------------------------------------------------------------
// string_cat()
// -----------------------------------------------------------------------------
//
// Merges given strings or numbers, using no delimiter(s), returning the merged
// result as a string.
//
// `string_cat()` is designed to be the fastest possible way to construct a string
// out of a mix of raw C strings, string_views, strings, bool values,
// and numeric values.
//
// Don't use `string_cat()` for user-visible strings. The localization process
// works poorly on strings built up out of fragments.
//
// For clarity and performance, don't use `string_cat()` when appending to a
// string. Use `string_append()` instead. In particular, avoid using any of these
// (anti-)patterns:
//
//   str.append(string_cat(...))
//   str += string_cat(...)
//   str = string_cat(str, ...)
//
// The last case is the worst, with a potential to change a loop
// from a linear time operation with O(1) dynamic allocations into a
// quadratic time operation with O(n) dynamic allocations.
//
// See `string_append()` below for more information.

namespace strings_internal {

// Do not call directly - this is not part of the public API.
std::string CatPieces(std::initializer_list<std::string_view> pieces);

void append_pieces(std::string *dest,
                   std::initializer_list<std::string_view> pieces);

}  // namespace strings_internal

FLARE_MUST_USE_RESULT FLARE_FORCE_INLINE std::string string_cat() { return std::string(); }

FLARE_MUST_USE_RESULT FLARE_FORCE_INLINE std::string string_cat(const alpha_num &a) {
    return std::string(a.data(), a.size());
}

FLARE_MUST_USE_RESULT std::string string_cat(const alpha_num &a, const alpha_num &b);

FLARE_MUST_USE_RESULT std::string string_cat(const alpha_num &a, const alpha_num &b,
                                            const alpha_num &c);

FLARE_MUST_USE_RESULT std::string string_cat(const alpha_num &a, const alpha_num &b,
                                            const alpha_num &c, const alpha_num &d);

// Support 5 or more arguments
template<typename... AV>
FLARE_MUST_USE_RESULT FLARE_FORCE_INLINE std::string string_cat(
        const alpha_num &a, const alpha_num &b, const alpha_num &c, const alpha_num &d,
        const alpha_num &e, const AV &... args) {
    return strings_internal::CatPieces(
            {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
             static_cast<const alpha_num &>(args).Piece()...});
}

// -----------------------------------------------------------------------------
// string_append()
// -----------------------------------------------------------------------------
//
// Appends a string or set of strings to an existing string, in a similar
// fashion to `string_cat()`.
//
// WARNING: `string_append(&str, a, b, c, ...)` requires that none of the
// a, b, c, parameters be a reference into str. For speed, `string_append()` does
// not try to check each of its input arguments to be sure that they are not
// a subset of the string being appended to. That is, while this will work:
//
//   std::string s = "foo";
//   s += s;
//
// This output is undefined:
//
//   std::string s = "foo";
//   string_append(&s, s);
//
// This output is undefined as well, since `std::string_view` does not own its
// data:
//
//   std::string s = "foobar";
//   std::string_view p = s;
//   string_append(&s, p);

FLARE_FORCE_INLINE void string_append(std::string *) {}

void string_append(std::string *dest, const alpha_num &a);

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b);

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b,
                   const alpha_num &c);

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b,
                   const alpha_num &c, const alpha_num &d);

// Support 5 or more arguments
template<typename... AV>
FLARE_FORCE_INLINE void string_append(std::string *dest, const alpha_num &a, const alpha_num &b,
                                     const alpha_num &c, const alpha_num &d, const alpha_num &e,
                                     const AV &... args) {
    strings_internal::append_pieces(
            dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                   static_cast<const alpha_num &>(args).Piece()...});
}

// Helper function for the future string_cat default floating-point format, %.6g
// This is fast.
FLARE_FORCE_INLINE strings_internal::alpha_num_buffer<
        numbers_internal::kSixDigitsToBufferSize>
SixDigits(double d) {
    strings_internal::alpha_num_buffer<numbers_internal::kSixDigitsToBufferSize>
            result;
    result.size = numbers_internal::six_digits_to_buffer(d, &result.data[0]);
    return result;
}


}  // namespace flare

#endif  // FLARE_STRINGS_STR_CAT_H_
