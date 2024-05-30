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
// File: substitute.h
// -----------------------------------------------------------------------------
//
// This package contains functions for efficiently performing string
// substitutions using a format string with positional notation:
// `substitute()` and `substitute_and_append()`.
//
// Unlike printf-style format specifiers, `substitute()` functions do not need
// to specify the type of the substitution arguments. Supported arguments
// following the format string, such as strings, string_views, ints,
// floats, and bools, are automatically converted to strings during the
// substitution process. (See below for a full list of supported types.)
//
// `substitute()` does not allow you to specify *how* to format a value, beyond
// the default conversion to string. For example, you cannot format an integer
// in hex.
//
// The format string uses positional identifiers indicated by a dollar sign ($)
// and single digit positional ids to indicate which substitution arguments to
// use at that location within the format string.
//
// A '$$' sequence in the format string causes a literal '$' character to be
// output.
//
// Example 1:
//   std::string s = substitute("$1 purchased $0 $2 for $$10. Thanks $1!",
//                              5, "Bob", "Apples");
//   EXPECT_EQ("Bob purchased 5 Apples for $10. Thanks Bob!", s);
//
// Example 2:
//   std::string s = "Hi. ";
//   substitute_and_append(&s, "My name is $0 and I am $1 years old.", "Bob", 5);
//   EXPECT_EQ("Hi. My name is Bob and I am 5 years old.", s);
//
// Supported types:
//   * turbo::string_view, std::string, const char* (null is equivalent to "")
//   * int32_t, int64_t, uint32_t, uint64_t
//   * float, double
//   * bool (Printed as "true" or "false")
//   * pointer types other than char* (Printed as "0x<lower case hex string>",
//     except that null is printed as "NULL")
//   * user-defined types via the `turbo_stringify()` customization point. See the
//     documentation for `turbo::str_cat` for an explanation on how to use this.
//
// If an invalid format string is provided, substitute returns an empty string
// and substitute_and_append does not change the provided output string.
// A format string is invalid if it:
//   * ends in an unescaped $ character,
//     e.g. "Hello $", or
//   * calls for a position argument which is not provided,
//     e.g. substitute("Hello $2", "world"), or
//   * specifies a non-digit, non-$ character after an unescaped $ character,
//     e.g. "Hello $f".
// In debug mode, i.e. #ifndef NDEBUG, such errors terminate the program.

#pragma once

#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include <turbo/base/macros.h>
#include <turbo/base/nullability.h>
#include <turbo/base/port.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/internal/stringify_sink.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/strip.h>

namespace turbo {
    namespace substitute_internal {

        // Arg
        //
        // This class provides an argument type for `turbo::substitute()` and
        // `turbo::substitute_and_append()`. `Arg` handles implicit conversion of various
        // types to a string. (`Arg` is very similar to the `AlphaNum` class in
        // `str_cat()`.)
        //
        // This class has implicit constructors.
        class Arg {
        public:
            // Overloads for string-y things
            //
            // Explicitly overload `const char*` so the compiler doesn't cast to `bool`.
            Arg(turbo::Nullable<const char *> value)  // NOLINT(google-explicit-constructor)
                    : piece_(turbo::NullSafeStringView(value)) {}

            template<typename Allocator>
            Arg(  // NOLINT
                    const std::basic_string<char, std::char_traits<char>, Allocator> &
                    value) noexcept
                    : piece_(value) {}

            Arg(turbo::string_view value)  // NOLINT(google-explicit-constructor)
                    : piece_(value) {}

            // Overloads for primitives
            //
            // No overloads are available for signed and unsigned char because if people
            // are explicitly declaring their chars as signed or unsigned then they are
            // probably using them as 8-bit integers and would probably prefer an integer
            // representation. However, we can't really know, so we make the caller decide
            // what to do.
            Arg(char value)  // NOLINT(google-explicit-constructor)
                    : piece_(scratch_, 1) {
                scratch_[0] = value;
            }

            Arg(short value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(unsigned short value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(int value)  // NOLINT(google-explicit-constructor)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(unsigned int value)  // NOLINT(google-explicit-constructor)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(long value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(unsigned long value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(long long value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(unsigned long long value)  // NOLINT(*)
                    : piece_(scratch_,
                             static_cast<size_t>(
                                     numbers_internal::FastIntToBuffer(value, scratch_) -
                                     scratch_)) {}

            Arg(float value)  // NOLINT(google-explicit-constructor)
                    : piece_(scratch_, numbers_internal::SixDigitsToBuffer(value, scratch_)) {
            }

            Arg(double value)  // NOLINT(google-explicit-constructor)
                    : piece_(scratch_, numbers_internal::SixDigitsToBuffer(value, scratch_)) {
            }

            Arg(bool value)  // NOLINT(google-explicit-constructor)
                    : piece_(value ? "true" : "false") {}

            template<typename T, typename = typename std::enable_if<
                    HasTurboStringify<T>::value>::type>
            Arg(  // NOLINT(google-explicit-constructor)
                    const T &v, strings_internal::StringifySink &&sink = {})
                    : piece_(strings_internal::ExtractStringification(sink, v)) {}

            Arg(Hex hex);  // NOLINT(google-explicit-constructor)
            Arg(Dec dec);  // NOLINT(google-explicit-constructor)

            // vector<bool>::reference and const_reference require special help to convert
            // to `Arg` because it requires two user defined conversions.
            template<typename T,
                    turbo::enable_if_t<
                            std::is_class<T>::value &&
                            (std::is_same<T, std::vector<bool>::reference>::value ||
                             std::is_same<T, std::vector<bool>::const_reference>::value)> * =
                    nullptr>
            Arg(T value)  // NOLINT(google-explicit-constructor)
                    : Arg(static_cast<bool>(value)) {}

            // `void*` values, with the exception of `char*`, are printed as
            // "0x<hex value>". However, in the case of `nullptr`, "NULL" is printed.
            Arg(  // NOLINT(google-explicit-constructor)
                    turbo::Nullable<const void *> value);

            // Normal enums are already handled by the integer formatters.
            // This overload matches only scoped enums.
            template<typename T,
                    typename = typename std::enable_if<
                            std::is_enum<T>{} && !std::is_convertible<T, int>{} &&
                            !HasTurboStringify<T>::value>::type>
            Arg(T value)  // NOLINT(google-explicit-constructor)
                    : Arg(static_cast<typename std::underlying_type<T>::type>(value)) {}

            Arg(const Arg &) = delete;

            Arg &operator=(const Arg &) = delete;

            turbo::string_view piece() const { return piece_; }

        private:
            turbo::string_view piece_;
            char scratch_[numbers_internal::kFastToBufferSize];
        };

        // Internal helper function. Don't call this from outside this implementation.
        // This interface may change without notice.
        void SubstituteAndAppendArray(
                turbo::Nonnull<std::string *> output, turbo::string_view format,
                turbo::Nullable<const turbo::string_view *> args_array, size_t num_args);

#if defined(TURBO_BAD_CALL_IF)
        constexpr int CalculateOneBit(turbo::Nonnull<const char*> format) {
          // Returns:
          // * 2^N for '$N' when N is in [0-9]
          // * 0 for correct '$' escaping: '$$'.
          // * -1 otherwise.
          return (*format < '0' || *format > '9') ? (*format == '$' ? 0 : -1)
                                                  : (1 << (*format - '0'));
        }

        constexpr const char* SkipNumber(turbo::Nonnull<const char*> format) {
          return !*format ? format : (format + 1);
        }

        constexpr int PlaceholderBitmask(turbo::Nonnull<const char*> format) {
          return !*format
                     ? 0
                     : *format != '$' ? PlaceholderBitmask(format + 1)
                                      : (CalculateOneBit(format + 1) |
                                         PlaceholderBitmask(SkipNumber(format + 1)));
        }
#endif  // TURBO_BAD_CALL_IF

    }  // namespace substitute_internal

    //
    // PUBLIC API
    //

    // substitute_and_append()
    //
    // Substitutes variables into a given format string and appends to a given
    // output string. See file comments above for usage.
    //
    // The declarations of `substitute_and_append()` below consist of overloads
    // for passing 0 to 10 arguments, respectively.
    //
    // NOTE: A zero-argument `substitute_and_append()` may be used within variadic
    // templates to allow a variable number of arguments.
    //
    // Example:
    //  template <typename... Args>
    //  void VarMsg(std::string* boilerplate, turbo::string_view format,
    //      const Args&... args) {
    //    turbo::substitute_and_append(boilerplate, format, args...);
    //  }
    //
    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format) {
        substitute_internal::SubstituteAndAppendArray(output, format, nullptr, 0);
    }

    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format,
                                    const substitute_internal::Arg &a0) {
        const turbo::string_view args[] = {a0.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format,
                                    const substitute_internal::Arg &a0,
                                    const substitute_internal::Arg &a1) {
        const turbo::string_view args[] = {a0.piece(), a1.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format,
                                    const substitute_internal::Arg &a0,
                                    const substitute_internal::Arg &a1,
                                    const substitute_internal::Arg &a2) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format,
                                    const substitute_internal::Arg &a0,
                                    const substitute_internal::Arg &a1,
                                    const substitute_internal::Arg &a2,
                                    const substitute_internal::Arg &a3) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(turbo::Nonnull<std::string *> output,
                                    turbo::string_view format,
                                    const substitute_internal::Arg &a0,
                                    const substitute_internal::Arg &a1,
                                    const substitute_internal::Arg &a2,
                                    const substitute_internal::Arg &a3,
                                    const substitute_internal::Arg &a4) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece(), a4.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(
            turbo::Nonnull<std::string *> output, turbo::string_view format,
            const substitute_internal::Arg &a0, const substitute_internal::Arg &a1,
            const substitute_internal::Arg &a2, const substitute_internal::Arg &a3,
            const substitute_internal::Arg &a4, const substitute_internal::Arg &a5) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece(), a4.piece(), a5.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(
            turbo::Nonnull<std::string *> output, turbo::string_view format,
            const substitute_internal::Arg &a0, const substitute_internal::Arg &a1,
            const substitute_internal::Arg &a2, const substitute_internal::Arg &a3,
            const substitute_internal::Arg &a4, const substitute_internal::Arg &a5,
            const substitute_internal::Arg &a6) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece(), a4.piece(), a5.piece(),
                                           a6.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(
            turbo::Nonnull<std::string *> output, turbo::string_view format,
            const substitute_internal::Arg &a0, const substitute_internal::Arg &a1,
            const substitute_internal::Arg &a2, const substitute_internal::Arg &a3,
            const substitute_internal::Arg &a4, const substitute_internal::Arg &a5,
            const substitute_internal::Arg &a6, const substitute_internal::Arg &a7) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece(), a4.piece(), a5.piece(),
                                           a6.piece(), a7.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(
            turbo::Nonnull<std::string *> output, turbo::string_view format,
            const substitute_internal::Arg &a0, const substitute_internal::Arg &a1,
            const substitute_internal::Arg &a2, const substitute_internal::Arg &a3,
            const substitute_internal::Arg &a4, const substitute_internal::Arg &a5,
            const substitute_internal::Arg &a6, const substitute_internal::Arg &a7,
            const substitute_internal::Arg &a8) {
        const turbo::string_view args[] = {a0.piece(), a1.piece(), a2.piece(),
                                           a3.piece(), a4.piece(), a5.piece(),
                                           a6.piece(), a7.piece(), a8.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

    inline void substitute_and_append(
            turbo::Nonnull<std::string *> output, turbo::string_view format,
            const substitute_internal::Arg &a0, const substitute_internal::Arg &a1,
            const substitute_internal::Arg &a2, const substitute_internal::Arg &a3,
            const substitute_internal::Arg &a4, const substitute_internal::Arg &a5,
            const substitute_internal::Arg &a6, const substitute_internal::Arg &a7,
            const substitute_internal::Arg &a8, const substitute_internal::Arg &a9) {
        const turbo::string_view args[] = {
                a0.piece(), a1.piece(), a2.piece(), a3.piece(), a4.piece(),
                a5.piece(), a6.piece(), a7.piece(), a8.piece(), a9.piece()};
        substitute_internal::SubstituteAndAppendArray(output, format, args,
                                                      TURBO_ARRAYSIZE(args));
    }

#if defined(TURBO_BAD_CALL_IF)
    // This body of functions catches cases where the number of placeholders
    // doesn't match the number of data arguments.
    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 0,
            "There were no substitution arguments "
            "but this format string either has a $[0-9] in it or contains "
            "an unescaped $ character (use $$ instead)");

    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format,
                             const substitute_internal::Arg& a0)
        TURBO_BAD_CALL_IF(substitute_internal::PlaceholderBitmask(format) != 1,
                         "There was 1 substitution argument given, but "
                         "this format string is missing its $0, contains "
                         "one of $1-$9, or contains an unescaped $ character (use "
                         "$$ instead)");

    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format,
                             const substitute_internal::Arg& a0,
                             const substitute_internal::Arg& a1)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 3,
            "There were 2 substitution arguments given, but this format string is "
            "missing its $0/$1, contains one of $2-$9, or contains an "
            "unescaped $ character (use $$ instead)");

    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format,
                             const substitute_internal::Arg& a0,
                             const substitute_internal::Arg& a1,
                             const substitute_internal::Arg& a2)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 7,
            "There were 3 substitution arguments given, but "
            "this format string is missing its $0/$1/$2, contains one of "
            "$3-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format,
                             const substitute_internal::Arg& a0,
                             const substitute_internal::Arg& a1,
                             const substitute_internal::Arg& a2,
                             const substitute_internal::Arg& a3)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 15,
            "There were 4 substitution arguments given, but "
            "this format string is missing its $0-$3, contains one of "
            "$4-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(turbo::Nonnull<std::string*> output,
                             turbo::Nonnull<const char*> format,
                             const substitute_internal::Arg& a0,
                             const substitute_internal::Arg& a1,
                             const substitute_internal::Arg& a2,
                             const substitute_internal::Arg& a3,
                             const substitute_internal::Arg& a4)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 31,
            "There were 5 substitution arguments given, but "
            "this format string is missing its $0-$4, contains one of "
            "$5-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(
        turbo::Nonnull<std::string*> output, turbo::Nonnull<const char*> format,
        const substitute_internal::Arg& a0, const substitute_internal::Arg& a1,
        const substitute_internal::Arg& a2, const substitute_internal::Arg& a3,
        const substitute_internal::Arg& a4, const substitute_internal::Arg& a5)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 63,
            "There were 6 substitution arguments given, but "
            "this format string is missing its $0-$5, contains one of "
            "$6-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(
        turbo::Nonnull<std::string*> output, turbo::Nonnull<const char*> format,
        const substitute_internal::Arg& a0, const substitute_internal::Arg& a1,
        const substitute_internal::Arg& a2, const substitute_internal::Arg& a3,
        const substitute_internal::Arg& a4, const substitute_internal::Arg& a5,
        const substitute_internal::Arg& a6)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 127,
            "There were 7 substitution arguments given, but "
            "this format string is missing its $0-$6, contains one of "
            "$7-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(
        turbo::Nonnull<std::string*> output, turbo::Nonnull<const char*> format,
        const substitute_internal::Arg& a0, const substitute_internal::Arg& a1,
        const substitute_internal::Arg& a2, const substitute_internal::Arg& a3,
        const substitute_internal::Arg& a4, const substitute_internal::Arg& a5,
        const substitute_internal::Arg& a6, const substitute_internal::Arg& a7)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 255,
            "There were 8 substitution arguments given, but "
            "this format string is missing its $0-$7, contains one of "
            "$8-$9, or contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(
        turbo::Nonnull<std::string*> output, turbo::Nonnull<const char*> format,
        const substitute_internal::Arg& a0, const substitute_internal::Arg& a1,
        const substitute_internal::Arg& a2, const substitute_internal::Arg& a3,
        const substitute_internal::Arg& a4, const substitute_internal::Arg& a5,
        const substitute_internal::Arg& a6, const substitute_internal::Arg& a7,
        const substitute_internal::Arg& a8)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 511,
            "There were 9 substitution arguments given, but "
            "this format string is missing its $0-$8, contains a $9, or "
            "contains an unescaped $ character (use $$ instead)");

    void substitute_and_append(
        turbo::Nonnull<std::string*> output, turbo::Nonnull<const char*> format,
        const substitute_internal::Arg& a0, const substitute_internal::Arg& a1,
        const substitute_internal::Arg& a2, const substitute_internal::Arg& a3,
        const substitute_internal::Arg& a4, const substitute_internal::Arg& a5,
        const substitute_internal::Arg& a6, const substitute_internal::Arg& a7,
        const substitute_internal::Arg& a8, const substitute_internal::Arg& a9)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 1023,
            "There were 10 substitution arguments given, but this "
            "format string either doesn't contain all of $0 through $9 or "
            "contains an unescaped $ character (use $$ instead)");
#endif  // TURBO_BAD_CALL_IF

    // substitute()
    //
    // Substitutes variables into a given format string. See file comments above
    // for usage.
    //
    // The declarations of `substitute()` below consist of overloads for passing 0
    // to 10 arguments, respectively.
    //
    // NOTE: A zero-argument `substitute()` may be used within variadic templates to
    // allow a variable number of arguments.
    //
    // Example:
    //  template <typename... Args>
    //  void VarMsg(turbo::string_view format, const Args&... args) {
    //    std::string s = turbo::substitute(format, args...);

    TURBO_MUST_USE_RESULT inline std::string substitute(turbo::string_view format) {
        std::string result;
        substitute_and_append(&result, format);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0) {
        std::string result;
        substitute_and_append(&result, format, a0);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1) {
        std::string result;
        substitute_and_append(&result, format, a0, a1);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4,
            const substitute_internal::Arg &a5) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4, a5);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4,
            const substitute_internal::Arg &a5, const substitute_internal::Arg &a6) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4, a5, a6);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4,
            const substitute_internal::Arg &a5, const substitute_internal::Arg &a6,
            const substitute_internal::Arg &a7) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4, a5, a6, a7);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4,
            const substitute_internal::Arg &a5, const substitute_internal::Arg &a6,
            const substitute_internal::Arg &a7, const substitute_internal::Arg &a8) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
        return result;
    }

    TURBO_MUST_USE_RESULT inline std::string substitute(
            turbo::string_view format, const substitute_internal::Arg &a0,
            const substitute_internal::Arg &a1, const substitute_internal::Arg &a2,
            const substitute_internal::Arg &a3, const substitute_internal::Arg &a4,
            const substitute_internal::Arg &a5, const substitute_internal::Arg &a6,
            const substitute_internal::Arg &a7, const substitute_internal::Arg &a8,
            const substitute_internal::Arg &a9) {
        std::string result;
        substitute_and_append(&result, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
        return result;
    }

#if defined(TURBO_BAD_CALL_IF)
    // This body of functions catches cases where the number of placeholders
    // doesn't match the number of data arguments.
    std::string substitute(turbo::Nonnull<const char*> format)
        TURBO_BAD_CALL_IF(substitute_internal::PlaceholderBitmask(format) != 0,
                         "There were no substitution arguments "
                         "but this format string either has a $[0-9] in it or "
                         "contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 1,
            "There was 1 substitution argument given, but "
            "this format string is missing its $0, contains one of $1-$9, "
            "or contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0,
                           const substitute_internal::Arg& a1)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 3,
            "There were 2 substitution arguments given, but "
            "this format string is missing its $0/$1, contains one of "
            "$2-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0,
                           const substitute_internal::Arg& a1,
                           const substitute_internal::Arg& a2)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 7,
            "There were 3 substitution arguments given, but "
            "this format string is missing its $0/$1/$2, contains one of "
            "$3-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0,
                           const substitute_internal::Arg& a1,
                           const substitute_internal::Arg& a2,
                           const substitute_internal::Arg& a3)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 15,
            "There were 4 substitution arguments given, but "
            "this format string is missing its $0-$3, contains one of "
            "$4-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0,
                           const substitute_internal::Arg& a1,
                           const substitute_internal::Arg& a2,
                           const substitute_internal::Arg& a3,
                           const substitute_internal::Arg& a4)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 31,
            "There were 5 substitution arguments given, but "
            "this format string is missing its $0-$4, contains one of "
            "$5-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(turbo::Nonnull<const char*> format,
                           const substitute_internal::Arg& a0,
                           const substitute_internal::Arg& a1,
                           const substitute_internal::Arg& a2,
                           const substitute_internal::Arg& a3,
                           const substitute_internal::Arg& a4,
                           const substitute_internal::Arg& a5)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 63,
            "There were 6 substitution arguments given, but "
            "this format string is missing its $0-$5, contains one of "
            "$6-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(
        turbo::Nonnull<const char*> format, const substitute_internal::Arg& a0,
        const substitute_internal::Arg& a1, const substitute_internal::Arg& a2,
        const substitute_internal::Arg& a3, const substitute_internal::Arg& a4,
        const substitute_internal::Arg& a5, const substitute_internal::Arg& a6)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 127,
            "There were 7 substitution arguments given, but "
            "this format string is missing its $0-$6, contains one of "
            "$7-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(
        turbo::Nonnull<const char*> format, const substitute_internal::Arg& a0,
        const substitute_internal::Arg& a1, const substitute_internal::Arg& a2,
        const substitute_internal::Arg& a3, const substitute_internal::Arg& a4,
        const substitute_internal::Arg& a5, const substitute_internal::Arg& a6,
        const substitute_internal::Arg& a7)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 255,
            "There were 8 substitution arguments given, but "
            "this format string is missing its $0-$7, contains one of "
            "$8-$9, or contains an unescaped $ character (use $$ instead)");

    std::string substitute(
        turbo::Nonnull<const char*> format, const substitute_internal::Arg& a0,
        const substitute_internal::Arg& a1, const substitute_internal::Arg& a2,
        const substitute_internal::Arg& a3, const substitute_internal::Arg& a4,
        const substitute_internal::Arg& a5, const substitute_internal::Arg& a6,
        const substitute_internal::Arg& a7, const substitute_internal::Arg& a8)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 511,
            "There were 9 substitution arguments given, but "
            "this format string is missing its $0-$8, contains a $9, or "
            "contains an unescaped $ character (use $$ instead)");

    std::string substitute(
        turbo::Nonnull<const char*> format, const substitute_internal::Arg& a0,
        const substitute_internal::Arg& a1, const substitute_internal::Arg& a2,
        const substitute_internal::Arg& a3, const substitute_internal::Arg& a4,
        const substitute_internal::Arg& a5, const substitute_internal::Arg& a6,
        const substitute_internal::Arg& a7, const substitute_internal::Arg& a8,
        const substitute_internal::Arg& a9)
        TURBO_BAD_CALL_IF(
            substitute_internal::PlaceholderBitmask(format) != 1023,
            "There were 10 substitution arguments given, but this "
            "format string either doesn't contain all of $0 through $9 or "
            "contains an unescaped $ character (use $$ instead)");
#endif  // TURBO_BAD_CALL_IF
}  // namespace turbo

