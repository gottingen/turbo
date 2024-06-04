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
// File: str_cat.h
// -----------------------------------------------------------------------------
//
// This package contains functions for efficiently concatenating and appending
// strings: `str_cat()` and `str_append()`. Most of the work within these routines
// is actually handled through use of a special AlphaNum type, which was
// designed to be used as a parameter type that efficiently manages conversion
// to strings and avoids copies in the above operations.
//
// Any routine accepting either a string or a number may accept `AlphaNum`.
// The basic idea is that by accepting a `const AlphaNum &` as an argument
// to your function, your callers will automagically convert bools, integers,
// and floating point values to strings for you.
//
// NOTE: Use of `AlphaNum` outside of the //turbo/strings package is unsupported
// except for the specific case of function parameters of type `AlphaNum` or
// `const AlphaNum &`. In particular, instantiating `AlphaNum` directly as a
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
// `Hex` type contained here. To do so, pass `Hex(my_int)` as a parameter to
// `str_cat()` or `str_append()`. You may specify a minimum hex field width using
// a `PadSpec` enum.
//
// User-defined types can be formatted with the `turbo_stringify()` customization
// point. The API relies on detecting an overload in the user-defined type's
// namespace of a free (non-member) `turbo_stringify()` function as a definition
// (typically declared as a friend and implemented in-line.
// with the following signature:
//
// class MyClass { ... };
//
// template <typename Sink>
// void turbo_stringify(Sink& sink, const MyClass& value);
//
// An `turbo_stringify()` overload for a type should only be declared in the same
// file and namespace as said type.
//
// Note that `turbo_stringify()` also supports use with `turbo::str_format()` and
// `turbo::substitute()`.
//
// Example:
//
// struct Point {
//   // To add formatting support to `Point`, we simply need to add a free
//   // (non-member) function `turbo_stringify()`. This method specifies how
//   // Point should be printed when turbo::str_cat() is called on it. You can add
//   // such a free function using a friend declaration within the body of the
//   // class. The sink parameter is a templated type to avoid requiring
//   // dependencies.
//   template <typename Sink> friend void turbo_stringify(Sink&
//   sink, const Point& p) {
//     turbo::format(&sink, "(%v, %v)", p.x, p.y);
//   }
//
//   int x;
//   int y;
// };
// -----------------------------------------------------------------------------

#pragma  once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <turbo/base/attributes.h>
#include <turbo/base/nullability.h>
#include <turbo/base/port.h>
#include <turbo/meta/type_traits.h>
#include <turbo/strings/has_stringify.h>
#include <turbo/strings/internal/resize_uninitialized.h>
#include <turbo/strings/internal/stringify_sink.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    namespace strings_internal {
        // AlphaNumBuffer allows a way to pass a string to str_cat without having to do
        // memory allocation.  It is simply a pair of a fixed-size character array, and
        // a size.  Please don't use outside of turbo, yet.
        template<size_t max_size>
        struct AlphaNumBuffer {
            std::array<char, max_size> data;
            size_t size;
        };

    }  // namespace strings_internal

    // Enum that specifies the number of significant digits to return in a `Hex` or
    // `Dec` conversion and fill character to use. A `kZeroPad2` value, for example,
    // would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
    // would produce hexadecimal strings such as "    a","    f".
    enum PadSpec : uint8_t {
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
    // Hex
    // -----------------------------------------------------------------------------
    //
    // `Hex` stores a set of hexadecimal string conversion parameters for use
    // within `AlphaNum` string conversions.
    struct Hex {
        uint64_t value;
        uint8_t width;
        char fill;

        template<typename Int>
        explicit Hex(
                Int v, PadSpec spec = turbo::kNoPad,
                typename std::enable_if<sizeof(Int) == 1 &&
                                        !std::is_pointer<Int>::value>::type * = nullptr)
                : Hex(spec, static_cast<uint8_t>(v)) {}

        template<typename Int>
        explicit Hex(
                Int v, PadSpec spec = turbo::kNoPad,
                typename std::enable_if<sizeof(Int) == 2 &&
                                        !std::is_pointer<Int>::value>::type * = nullptr)
                : Hex(spec, static_cast<uint16_t>(v)) {}

        template<typename Int>
        explicit Hex(
                Int v, PadSpec spec = turbo::kNoPad,
                typename std::enable_if<sizeof(Int) == 4 &&
                                        !std::is_pointer<Int>::value>::type * = nullptr)
                : Hex(spec, static_cast<uint32_t>(v)) {}

        template<typename Int>
        explicit Hex(
                Int v, PadSpec spec = turbo::kNoPad,
                typename std::enable_if<sizeof(Int) == 8 &&
                                        !std::is_pointer<Int>::value>::type * = nullptr)
                : Hex(spec, static_cast<uint64_t>(v)) {}

        template<typename Pointee>
        explicit Hex(turbo::Nullable<Pointee *> v, PadSpec spec = turbo::kNoPad)
                : Hex(spec, reinterpret_cast<uintptr_t>(v)) {}

        template<typename S>
        friend void turbo_stringify(S &sink, Hex hex) {
            static_assert(
                    numbers_internal::kFastToBufferSize >= 32,
                    "This function only works when output buffer >= 32 bytes long");
            char buffer[numbers_internal::kFastToBufferSize];
            char *const end = &buffer[numbers_internal::kFastToBufferSize];
            auto real_width =
                    turbo::numbers_internal::FastHexToBufferZeroPad16(hex.value, end - 16);
            if (real_width >= hex.width) {
                sink.Append(turbo::string_view(end - real_width, real_width));
            } else {
                // Pad first 16 chars because FastHexToBufferZeroPad16 pads only to 16 and
                // max pad width can be up to 20.
                std::memset(end - 32, hex.fill, 16);
                // Patch up everything else up to the real_width.
                std::memset(end - real_width - 16, hex.fill, 16);
                sink.Append(turbo::string_view(end - hex.width, hex.width));
            }
        }

    private:
        Hex(PadSpec spec, uint64_t v)
                : value(v),
                  width(spec == turbo::kNoPad
                        ? 1
                        : spec >= turbo::kSpacePad2 ? spec - turbo::kSpacePad2 + 2
                                                    : spec - turbo::kZeroPad2 + 2),
                  fill(spec >= turbo::kSpacePad2 ? ' ' : '0') {}
    };

// -----------------------------------------------------------------------------
// Dec
// -----------------------------------------------------------------------------
//
// `Dec` stores a set of decimal string conversion parameters for use
// within `AlphaNum` string conversions.  Dec is slower than the default
// integer conversion, so use it only if you need padding.
    struct Dec {
        uint64_t value;
        uint8_t width;
        char fill;
        bool neg;

        template<typename Int>
        explicit Dec(Int v, PadSpec spec = turbo::kNoPad,
                     typename std::enable_if<(sizeof(Int) <= 8)>::type * = nullptr)
                : value(v >= 0 ? static_cast<uint64_t>(v)
                               : uint64_t{0} - static_cast<uint64_t>(v)),
                  width(spec == turbo::kNoPad ? 1
                                              : spec >= turbo::kSpacePad2 ? spec - turbo::kSpacePad2 + 2
                                                                          : spec - turbo::kZeroPad2 + 2),
                  fill(spec >= turbo::kSpacePad2 ? ' ' : '0'),
                  neg(v < 0) {}

        template<typename S>
        friend void turbo_stringify(S &sink, Dec dec) {
            assert(dec.width <= numbers_internal::kFastToBufferSize);
            char buffer[numbers_internal::kFastToBufferSize];
            char *const end = &buffer[numbers_internal::kFastToBufferSize];
            char *const minfill = end - dec.width;
            char *writer = end;
            uint64_t val = dec.value;
            while (val > 9) {
                *--writer = '0' + (val % 10);
                val /= 10;
            }
            *--writer = '0' + static_cast<char>(val);
            if (dec.neg) *--writer = '-';

            ptrdiff_t fillers = writer - minfill;
            if (fillers > 0) {
                // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
                // But...: if the fill character is '0', then it's <+/-><fill><digits>
                bool add_sign_again = false;
                if (dec.neg && dec.fill == '0') {  // If filling with '0',
                    ++writer;                    // ignore the sign we just added
                    add_sign_again = true;       // and re-add the sign later.
                }
                writer -= fillers;
                std::fill_n(writer, fillers, dec.fill);
                if (add_sign_again) *--writer = '-';
            }

            sink.Append(turbo::string_view(writer, static_cast<size_t>(end - writer)));
        }
    };

// -----------------------------------------------------------------------------
// AlphaNum
// -----------------------------------------------------------------------------
//
// The `AlphaNum` class acts as the main parameter type for `str_cat()` and
// `str_append()`, providing efficient conversion of numeric, boolean, decimal,
// and hexadecimal values (through the `Dec` and `Hex` types) into strings.
// `AlphaNum` should only be used as a function parameter. Do not instantiate
//  `AlphaNum` directly as a stack variable.

    class AlphaNum {
    public:
        // No bool ctor -- bools convert to an integral type.
        // A bool ctor would also convert incoming pointers (bletch).

        // Prevent brace initialization
        template<typename T>
        AlphaNum(std::initializer_list<T>) = delete;  // NOLINT(runtime/explicit)

        AlphaNum(int x)  // NOLINT(runtime/explicit)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(unsigned int x)  // NOLINT(runtime/explicit)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(long x)  // NOLINT(*)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(unsigned long x)  // NOLINT(*)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(long long x)  // NOLINT(*)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(unsigned long long x)  // NOLINT(*)
                : piece_(digits_, static_cast<size_t>(
                numbers_internal::FastIntToBuffer(x, digits_) -
                &digits_[0])) {}

        AlphaNum(float f)  // NOLINT(runtime/explicit)
                : piece_(digits_, numbers_internal::SixDigitsToBuffer(f, digits_)) {}

        AlphaNum(double f)  // NOLINT(runtime/explicit)
                : piece_(digits_, numbers_internal::SixDigitsToBuffer(f, digits_)) {}

        template<size_t size>
        AlphaNum(  // NOLINT(runtime/explicit)
                const strings_internal::AlphaNumBuffer<size> &buf
                TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : piece_(&buf.data[0], buf.size) {}

        AlphaNum(turbo::Nullable<const char *> c_str  // NOLINT(runtime/explicit)
                 TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : piece_(NullSafeStringView(c_str)) {}

        AlphaNum(turbo::string_view pc  // NOLINT(runtime/explicit)
                 TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : piece_(pc) {}

        template<typename T, typename = typename std::enable_if<
                HasTurboStringify<T>::value>::type>
        AlphaNum(  // NOLINT(runtime/explicit)
                const T &v TURBO_ATTRIBUTE_LIFETIME_BOUND,
                strings_internal::StringifySink &&sink TURBO_ATTRIBUTE_LIFETIME_BOUND = {})
                : piece_(strings_internal::ExtractStringification(sink, v)) {}

        template<typename Allocator>
        AlphaNum(  // NOLINT(runtime/explicit)
                const std::basic_string<char, std::char_traits<char>, Allocator> &str
                TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : piece_(str) {}

        // Use string literals ":" instead of character literals ':'.
        AlphaNum(char c) = delete;  // NOLINT(runtime/explicit)

        AlphaNum(const AlphaNum &) = delete;

        AlphaNum &operator=(const AlphaNum &) = delete;

        turbo::string_view::size_type size() const { return piece_.size(); }

        turbo::Nullable<const char *> data() const { return piece_.data(); }

        turbo::string_view Piece() const { return piece_; }

        // Match unscoped enums.  Use integral promotion so that a `char`-backed
        // enum becomes a wider integral type AlphaNum will accept.
        template<typename T,
                typename = typename std::enable_if<
                        std::is_enum<T>{} && std::is_convertible<T, int>{} &&
                        !HasTurboStringify<T>::value>::type>
        AlphaNum(T e)  // NOLINT(runtime/explicit)
                : AlphaNum(+e) {}

        // This overload matches scoped enums.  We must explicitly cast to the
        // underlying type, but use integral promotion for the same reason as above.
        template<typename T,
                typename std::enable_if<std::is_enum<T>{} &&
                                        !std::is_convertible<T, int>{} &&
                                        !HasTurboStringify<T>::value,
                        char *>::type = nullptr>
        AlphaNum(T e)  // NOLINT(runtime/explicit)
                : AlphaNum(+static_cast<typename std::underlying_type<T>::type>(e)) {}

        // vector<bool>::reference and const_reference require special help to
        // convert to `AlphaNum` because it requires two user defined conversions.
        template<
                typename T,
                typename std::enable_if<
                        std::is_class<T>::value &&
                        (std::is_same<T, std::vector<bool>::reference>::value ||
                         std::is_same<T, std::vector<bool>::const_reference>::value)>::type * =
                nullptr>
        AlphaNum(T e) : AlphaNum(static_cast<bool>(e)) {}  // NOLINT(runtime/explicit)

    private:
        turbo::string_view piece_;
        char digits_[numbers_internal::kFastToBufferSize];
    };

    // -----------------------------------------------------------------------------
    // str_cat()
    // -----------------------------------------------------------------------------
    //
    // Merges given strings or numbers, using no delimiter(s), returning the merged
    // result as a string.
    //
    // `str_cat()` is designed to be the fastest possible way to construct a string
    // out of a mix of raw C strings, string_views, strings, bool values,
    // and numeric values.
    //
    // Don't use `str_cat()` for user-visible strings. The localization process
    // works poorly on strings built up out of fragments.
    //
    // For clarity and performance, don't use `str_cat()` when appending to a
    // string. Use `str_append()` instead. In particular, avoid using any of these
    // (anti-)patterns:
    //
    //   str.append(str_cat(...))
    //   str += str_cat(...)
    //   str = str_cat(str, ...)
    //
    // The last case is the worst, with a potential to change a loop
    // from a linear time operation with O(1) dynamic allocations into a
    // quadratic time operation with O(n) dynamic allocations.
    //
    // See `str_append()` below for more information.

    namespace strings_internal {

        // Do not call directly - this is not part of the public API.
        std::string CatPieces(std::initializer_list<turbo::string_view> pieces);

        void AppendPieces(turbo::Nonnull<std::string *> dest,
                          std::initializer_list<turbo::string_view> pieces);

        template<typename Integer>
        std::string IntegerToString(Integer i) {
            // Any integer (signed/unsigned) up to 64 bits can be formatted into a buffer
            // with 22 bytes (including NULL at the end).
            constexpr size_t kMaxDigits10 = 22;
            std::string result;
            strings_internal::STLStringResizeUninitialized(&result, kMaxDigits10);
            char *start = &result[0];
            // note: this can be optimized to not write last zero.
            char *end = numbers_internal::FastIntToBuffer(i, start);
            auto size = static_cast<size_t>(end - start);
            assert((size < result.size()) &&
                   "str_cat(Integer) does not fit into kMaxDigits10");
            result.erase(size);
            return result;
        }

        template<typename Float>
        std::string FloatToString(Float f) {
            std::string result;
            strings_internal::STLStringResizeUninitialized(
                    &result, numbers_internal::kSixDigitsToBufferSize);
            char *start = &result[0];
            result.erase(numbers_internal::SixDigitsToBuffer(f, start));
            return result;
        }

        // `SingleArgStrCat` overloads take built-in `int`, `long` and `long long` types
        // (signed / unsigned) to avoid ambiguity on the call side. If we used int32_t
        // and int64_t, then at least one of the three (`int` / `long` / `long long`)
        // would have been ambiguous when passed to `SingleArgStrCat`.
        inline std::string SingleArgStrCat(int x) { return IntegerToString(x); }

        inline std::string SingleArgStrCat(unsigned int x) {
            return IntegerToString(x);
        }

        // NOLINTNEXTLINE
        inline std::string SingleArgStrCat(long x) { return IntegerToString(x); }

        // NOLINTNEXTLINE
        inline std::string SingleArgStrCat(unsigned long x) {
            return IntegerToString(x);
        }

        // NOLINTNEXTLINE
        inline std::string SingleArgStrCat(long long x) { return IntegerToString(x); }

        // NOLINTNEXTLINE
        inline std::string SingleArgStrCat(unsigned long long x) {
            return IntegerToString(x);
        }

        inline std::string SingleArgStrCat(float x) { return FloatToString(x); }

        inline std::string SingleArgStrCat(double x) { return FloatToString(x); }

// As of September 2023, the SingleArgStrCat() optimization is only enabled for
// libc++. The reasons for this are:
// 1) The SSO size for libc++ is 23, while libstdc++ and MSSTL have an SSO size
// of 15. Since IntegerToString unconditionally resizes the string to 22 bytes,
// this causes both libstdc++ and MSSTL to allocate.
// 2) strings_internal::STLStringResizeUninitialized() only has an
// implementation that avoids initialization when using libc++. This isn't as
// relevant as (1), and the cost should be benchmarked if (1) ever changes on
// libstc++ or MSSTL.
#ifdef _LIBCPP_VERSION
#define TURBO_INTERNAL_STRCAT_ENABLE_FAST_CASE true
#else
#define TURBO_INTERNAL_STRCAT_ENABLE_FAST_CASE false
#endif

        template<typename T, typename = std::enable_if_t<
                TURBO_INTERNAL_STRCAT_ENABLE_FAST_CASE &&
                std::is_arithmetic<T>{} && !std::is_same<T, char>{}>>
        using EnableIfFastCase = T;

#undef TURBO_INTERNAL_STRCAT_ENABLE_FAST_CASE

    }  // namespace strings_internal

    TURBO_MUST_USE_RESULT inline std::string str_cat() { return std::string(); }

    template<typename T>
    TURBO_MUST_USE_RESULT inline std::string str_cat(
            strings_internal::EnableIfFastCase<T> a) {
        return strings_internal::SingleArgStrCat(a);
    }

    TURBO_MUST_USE_RESULT inline std::string str_cat(const AlphaNum &a) {
        return std::string(a.data(), a.size());
    }

    TURBO_MUST_USE_RESULT std::string str_cat(const AlphaNum &a, const AlphaNum &b);

    TURBO_MUST_USE_RESULT std::string str_cat(const AlphaNum &a, const AlphaNum &b,
                                             const AlphaNum &c);

    TURBO_MUST_USE_RESULT std::string str_cat(const AlphaNum &a, const AlphaNum &b,
                                             const AlphaNum &c, const AlphaNum &d);

    // Support 5 or more arguments
    template<typename... AV>
    TURBO_MUST_USE_RESULT inline std::string str_cat(
            const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
            const AlphaNum &e, const AV &... args) {
        return strings_internal::CatPieces(
                {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                 static_cast<const AlphaNum &>(args).Piece()...});
    }

    // -----------------------------------------------------------------------------
    // str_append()
    // -----------------------------------------------------------------------------
    //
    // Appends a string or set of strings to an existing string, in a similar
    // fashion to `str_cat()`.
    //
    // WARNING: `str_append(&str, a, b, c, ...)` requires that none of the
    // a, b, c, parameters be a reference into str. For speed, `str_append()` does
    // not try to check each of its input arguments to be sure that they are not
    // a subset of the string being appended to. That is, while this will work:
    //
    //   std::string s = "foo";
    //   s += s;
    //
    // This output is undefined:
    //
    //   std::string s = "foo";
    //   str_append(&s, s);
    //
    // This output is undefined as well, since `turbo::string_view` does not own its
    // data:
    //
    //   std::string s = "foobar";
    //   turbo::string_view p = s;
    //   str_append(&s, p);

    inline void str_append(turbo::Nonnull<std::string *>) {}

    void str_append(turbo::Nonnull<std::string *> dest, const AlphaNum &a);

    void str_append(turbo::Nonnull<std::string *> dest, const AlphaNum &a,
                   const AlphaNum &b);

    void str_append(turbo::Nonnull<std::string *> dest, const AlphaNum &a,
                   const AlphaNum &b, const AlphaNum &c);

    void str_append(turbo::Nonnull<std::string *> dest, const AlphaNum &a,
                   const AlphaNum &b, const AlphaNum &c, const AlphaNum &d);

    // Support 5 or more arguments
    template<typename... AV>
    inline void str_append(turbo::Nonnull<std::string *> dest, const AlphaNum &a,
                          const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                          const AlphaNum &e, const AV &... args) {
        strings_internal::AppendPieces(
                dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                       static_cast<const AlphaNum &>(args).Piece()...});
    }

    // Helper function for the future str_cat default floating-point format, %.6g
    // This is fast.
    inline strings_internal::AlphaNumBuffer<
            numbers_internal::kSixDigitsToBufferSize>
    six_digits(double d) {
        strings_internal::AlphaNumBuffer<numbers_internal::kSixDigitsToBufferSize>
                result;
        result.size = numbers_internal::SixDigitsToBuffer(d, &result.data[0]);
        return result;
    }

}  // namespace turbo
