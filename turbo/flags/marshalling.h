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
// File: marshalling.h
// -----------------------------------------------------------------------------
//
// This header file defines the API for extending Turbo flag support to
// custom types, and defines the set of overloads for fundamental types.
//
// Out of the box, the Turbo flags library supports the following types:
//
// * `bool`
// * `int16_t`
// * `uint16_t`
// * `int32_t`
// * `uint32_t`
// * `int64_t`
// * `uint64_t`
// * `float`
// * `double`
// * `std::string`
// * `std::vector<std::string>`
// * `std::optional<T>`
// * `turbo::LogSeverity` (provided natively for layering reasons)
//
// Note that support for integral types is implemented using overloads for
// variable-width fundamental types (`short`, `int`, `long`, etc.). However,
// you should prefer the fixed-width integral types (`int32_t`, `uint64_t`,
// etc.) we've noted above within flag definitions.
//
// In addition, several Turbo libraries provide their own custom support for
// Turbo flags. Documentation for these formats is provided in the type's
// `turbo_parse_flag()` definition.
//
// The Turbo time library provides the following support for civil time values:
//
// * `turbo::CivilSecond`
// * `turbo::CivilMinute`
// * `turbo::CivilHour`
// * `turbo::CivilDay`
// * `turbo::CivilMonth`
// * `turbo::CivilYear`
//
// and also provides support for the following absolute time values:
//
// * `turbo::Duration`
// * `turbo::Time`
//
// Additional support for Turbo types will be noted here as it is added.
//
// You can also provide your own custom flags by adding overloads for
// `turbo_parse_flag()` and `turbo_unparse_flag()` to your type definitions. (See
// below.)
//
// -----------------------------------------------------------------------------
// Optional Flags
// -----------------------------------------------------------------------------
//
// The Turbo flags library supports flags of type `std::optional<T>` where
// `T` is a type of one of the supported flags. We refer to this flag type as
// an "optional flag." An optional flag is either "valueless", holding no value
// of type `T` (indicating that the flag has not been set) or a value of type
// `T`. The valueless state in C++ code is represented by a value of
// `std::nullopt` for the optional flag.
//
// Using `std::nullopt` as an optional flag's default value allows you to check
// whether such a flag was ever specified on the command line:
//
//   if (turbo::get_flag(FLAGS_foo).has_value()) {
//     // flag was set on command line
//   } else {
//     // flag was not passed on command line
//   }
//
// Using an optional flag in this manner avoids common workarounds for
// indicating such an unset flag (such as using sentinel values to indicate this
// state).
//
// An optional flag also allows a developer to pass a flag in an "unset"
// valueless state on the command line, allowing the flag to later be set in
// binary logic. An optional flag's valueless state is indicated by the special
// notation of passing the value as an empty string through the syntax `--flag=`
// or `--flag ""`.
//
//   $ binary_with_optional --flag_in_unset_state=
//   $ binary_with_optional --flag_in_unset_state ""
//
// Note: as a result of the above syntax requirements, an optional flag cannot
// be set to a `T` of any value which unparses to the empty string.
//
// -----------------------------------------------------------------------------
// Adding Type Support for Turbo Flags
// -----------------------------------------------------------------------------
//
// To add support for your user-defined type, add overloads of `turbo_parse_flag()`
// and `turbo_unparse_flag()` as free (non-member) functions to your type. If `T`
// is a class type, these functions can be friend function definitions. These
// overloads must be added to the same namespace where the type is defined, so
// that they can be discovered by Argument-Dependent Lookup (ADL).
//
// Example:
//
//   namespace foo {
//
//   enum OutputMode { kPlainText, kHtml };
//
//   // turbo_parse_flag converts from a string to OutputMode.
//   // Must be in same namespace as OutputMode.
//
//   // Parses an OutputMode from the command line flag value `text`. Returns
//   // `true` and sets `*mode` on success; returns `false` and sets `*error`
//   // on failure.
//   bool turbo_parse_flag(std::string_view text,
//                      OutputMode* mode,
//                      std::string* error) {
//     if (text == "plaintext") {
//       *mode = kPlainText;
//       return true;
//     }
//     if (text == "html") {
//       *mode = kHtml;
//      return true;
//     }
//     *error = "unknown value for enumeration";
//     return false;
//  }
//
//  // turbo_unparse_flag converts from an OutputMode to a string.
//  // Must be in same namespace as OutputMode.
//
//  // Returns a textual flag value corresponding to the OutputMode `mode`.
//  std::string turbo_unparse_flag(OutputMode mode) {
//    switch (mode) {
//      case kPlainText: return "plaintext";
//      case kHtml: return "html";
//    }
//    return turbo::str_cat(mode);
//  }
//
// Notice that neither `turbo_parse_flag()` nor `turbo_unparse_flag()` are class
// members, but free functions. `turbo_parse_flag/turbo_unparse_flag()` overloads
// for a type should only be declared in the same file and namespace as said
// type. The proper `turbo_parse_flag/turbo_unparse_flag()` implementations for a
// given type will be discovered via Argument-Dependent Lookup (ADL).
//
// `turbo_parse_flag()` may need, in turn, to parse simpler constituent types
// using `turbo::parse_flag()`. For example, a custom struct `MyFlagType`
// consisting of a `std::pair<int, std::string>` would add an `turbo_parse_flag()`
// overload for its `MyFlagType` like so:
//
// Example:
//
//   namespace my_flag_type {
//
//   struct MyFlagType {
//     std::pair<int, std::string> my_flag_data;
//   };
//
//   bool turbo_parse_flag(std::string_view text, MyFlagType* flag,
//                      std::string* err);
//
//   std::string turbo_unparse_flag(const MyFlagType&);
//
//   // Within the implementation, `turbo_parse_flag()` will, in turn invoke
//   // `turbo::parse_flag()` on its constituent `int` and `std::string` types
//   // (which have built-in Turbo flag support).
//
//   bool turbo_parse_flag(std::string_view text, MyFlagType* flag,
//                      std::string* err) {
//     std::pair<std::string_view, std::string_view> tokens =
//         turbo::str_split(text, ',');
//     if (!turbo::parse_flag(tokens.first, &flag->my_flag_data.first, err))
//         return false;
//     if (!turbo::parse_flag(tokens.second, &flag->my_flag_data.second, err))
//         return false;
//     return true;
//   }
//
//   // Similarly, for unparsing, we can simply invoke `turbo::unparse_flag()` on
//   // the constituent types.
//   std::string turbo_unparse_flag(const MyFlagType& flag) {
//     return turbo::str_cat(turbo::unparse_flag(flag.my_flag_data.first),
//                         ",",
//                         turbo::unparse_flag(flag.my_flag_data.second));
//   }

#pragma once

#include <turbo/base/config.h>
#include <turbo/numeric/int128.h>
#include <optional>
#include <string>
#include <vector>

#include <turbo/strings/string_view.h>

namespace turbo {

    // Forward declaration to be used inside composable flag parse/unparse
    // implementations
    template<typename T>
    inline bool parse_flag(std::string_view input, T *dst, std::string *error);

    template<typename T>
    inline std::string unparse_flag(const T &v);

    namespace flags_internal {

        // Overloads of `turbo_parse_flag()` and `turbo_unparse_flag()` for fundamental types.
        bool turbo_parse_flag(std::string_view, bool *, std::string *);

        bool turbo_parse_flag(std::string_view, short *, std::string *);           // NOLINT
        bool turbo_parse_flag(std::string_view, unsigned short *, std::string *);  // NOLINT
        bool turbo_parse_flag(std::string_view, int *, std::string *);             // NOLINT
        bool turbo_parse_flag(std::string_view, unsigned int *, std::string *);    // NOLINT
        bool turbo_parse_flag(std::string_view, long *, std::string *);            // NOLINT
        bool turbo_parse_flag(std::string_view, unsigned long *, std::string *);   // NOLINT
        bool turbo_parse_flag(std::string_view, long long *, std::string *);       // NOLINT
        bool turbo_parse_flag(std::string_view, unsigned long long *,             // NOLINT
                              std::string *);

        bool turbo_parse_flag(std::string_view, turbo::int128 *, std::string *);    // NOLINT
        bool turbo_parse_flag(std::string_view, turbo::uint128 *, std::string *);   // NOLINT
        bool turbo_parse_flag(std::string_view, float *, std::string *);

        bool turbo_parse_flag(std::string_view, double *, std::string *);

        bool turbo_parse_flag(std::string_view, std::string *, std::string *);

        bool turbo_parse_flag(std::string_view, std::vector<std::string> *, std::string *);

        template <typename T>
        bool turbo_parse_flag(std::string_view text, std::optional<T>* f,
                           std::string* err) {
          if (text.empty()) {
            *f = std::nullopt;
            return true;
          }
          T value;
          if (!turbo::parse_flag(text, &value, err)) return false;

          *f = std::move(value);
          return true;
        }

        template<typename T>
        bool InvokeParseFlag(std::string_view input, T *dst, std::string *err) {
            // Comment on next line provides a good compiler error message if T
            // does not have turbo_parse_flag(std::string_view, T*, std::string*).
            return turbo_parse_flag(input, dst, err);  // Is T missing turbo_parse_flag?
        }

        // Strings and std:: containers do not have the same overload resolution
        // considerations as fundamental types. Naming these 'turbo_unparse_flag' means we
        // can avoid the need for additional specializations of Unparse (below).
        std::string turbo_unparse_flag(std::string_view v);

        std::string turbo_unparse_flag(const std::vector<std::string> &);


        template <typename T>
        std::string turbo_unparse_flag(const std::optional<T>& f) {
          return f.has_value() ? turbo::unparse_flag(*f) : "";
        }

        template<typename T>
        std::string Unparse(const T &v) {
            // Comment on next line provides a good compiler error message if T does not
            // have unparse_flag.
            return turbo_unparse_flag(v);  // Is T missing turbo_unparse_flag?
        }

        // Overloads for builtin types.
        std::string Unparse(bool v);

        std::string Unparse(short v);               // NOLINT
        std::string Unparse(unsigned short v);      // NOLINT
        std::string Unparse(int v);                 // NOLINT
        std::string Unparse(unsigned int v);        // NOLINT
        std::string Unparse(long v);                // NOLINT
        std::string Unparse(unsigned long v);       // NOLINT
        std::string Unparse(long long v);           // NOLINT
        std::string Unparse(unsigned long long v);  // NOLINT
        std::string Unparse(turbo::int128 v);

        std::string Unparse(turbo::uint128 v);

        std::string Unparse(float v);

        std::string Unparse(double v);

    }  // namespace flags_internal

    // parse_flag()
    //
    // Parses a string value into a flag value of type `T`. Do not add overloads of
    // this function for your type directly; instead, add an `turbo_parse_flag()`
    // free function as documented above.
    //
    // Some implementations of `turbo_parse_flag()` for types which consist of other,
    // constituent types which already have Turbo flag support, may need to call
    // `turbo::parse_flag()` on those consituent string values. (See above.)
    template<typename T>
    inline bool parse_flag(std::string_view input, T *dst, std::string *error) {
        return flags_internal::InvokeParseFlag(input, dst, error);
    }

    // unparse_flag()
    //
    // Unparses a flag value of type `T` into a string value. Do not add overloads
    // of this function for your type directly; instead, add an `turbo_unparse_flag()`
    // free function as documented above.
    //
    // Some implementations of `turbo_unparse_flag()` for types which consist of other,
    // constituent types which already have Turbo flag support, may want to call
    // `turbo::unparse_flag()` on those constituent types. (See above.)
    template<typename T>
    inline std::string unparse_flag(const T &v) {
        return flags_internal::Unparse(v);
    }

    // Overloads for `turbo::LogSeverity` can't (easily) appear alongside that type's
    // definition because it is layered below flags.  See proper documentation in
    // base/log_severity.h.
    enum class LogSeverity : int;

    bool turbo_parse_flag(std::string_view, turbo::LogSeverity *, std::string *);

    std::string turbo_unparse_flag(turbo::LogSeverity);

}  // namespace turbo
