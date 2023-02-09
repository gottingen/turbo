//
//  Copyright 2019 The Turbo Authors.
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
// `TurboParseFlag()` definition.
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
// `TurboParseFlag()` and `TurboUnparseFlag()` to your type definitions. (See
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
//   if (turbo::GetFlag(FLAGS_foo).has_value()) {
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
// To add support for your user-defined type, add overloads of `TurboParseFlag()`
// and `TurboUnparseFlag()` as free (non-member) functions to your type. If `T`
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
//   // TurboParseFlag converts from a string to OutputMode.
//   // Must be in same namespace as OutputMode.
//
//   // Parses an OutputMode from the command line flag value `text`. Returns
//   // `true` and sets `*mode` on success; returns `false` and sets `*error`
//   // on failure.
//   bool TurboParseFlag(turbo::string_view text,
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
//  // TurboUnparseFlag converts from an OutputMode to a string.
//  // Must be in same namespace as OutputMode.
//
//  // Returns a textual flag value corresponding to the OutputMode `mode`.
//  std::string TurboUnparseFlag(OutputMode mode) {
//    switch (mode) {
//      case kPlainText: return "plaintext";
//      case kHtml: return "html";
//    }
//    return turbo::StrCat(mode);
//  }
//
// Notice that neither `TurboParseFlag()` nor `TurboUnparseFlag()` are class
// members, but free functions. `TurboParseFlag/TurboUnparseFlag()` overloads
// for a type should only be declared in the same file and namespace as said
// type. The proper `TurboParseFlag/TurboUnparseFlag()` implementations for a
// given type will be discovered via Argument-Dependent Lookup (ADL).
//
// `TurboParseFlag()` may need, in turn, to parse simpler constituent types
// using `turbo::ParseFlag()`. For example, a custom struct `MyFlagType`
// consisting of a `std::pair<int, std::string>` would add an `TurboParseFlag()`
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
//   bool TurboParseFlag(turbo::string_view text, MyFlagType* flag,
//                      std::string* err);
//
//   std::string TurboUnparseFlag(const MyFlagType&);
//
//   // Within the implementation, `TurboParseFlag()` will, in turn invoke
//   // `turbo::ParseFlag()` on its constituent `int` and `std::string` types
//   // (which have built-in Turbo flag support).
//
//   bool TurboParseFlag(turbo::string_view text, MyFlagType* flag,
//                      std::string* err) {
//     std::pair<turbo::string_view, turbo::string_view> tokens =
//         turbo::StrSplit(text, ',');
//     if (!turbo::ParseFlag(tokens.first, &flag->my_flag_data.first, err))
//         return false;
//     if (!turbo::ParseFlag(tokens.second, &flag->my_flag_data.second, err))
//         return false;
//     return true;
//   }
//
//   // Similarly, for unparsing, we can simply invoke `turbo::UnparseFlag()` on
//   // the constituent types.
//   std::string TurboUnparseFlag(const MyFlagType& flag) {
//     return turbo::StrCat(turbo::UnparseFlag(flag.my_flag_data.first),
//                         ",",
//                         turbo::UnparseFlag(flag.my_flag_data.second));
//   }
#ifndef TURBO_FLAGS_MARSHALLING_H_
#define TURBO_FLAGS_MARSHALLING_H_

#include "turbo/platform/port.h"

#if defined(TURBO_HAVE_STD_OPTIONAL) && !defined(TURBO_USES_STD_OPTIONAL)
#include <optional>
#endif
#include <string>
#include <vector>

#include "turbo/strings/string_view.h"
#include "turbo/meta/optional.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Forward declaration to be used inside composable flag parse/unparse
// implementations
template <typename T>
inline bool ParseFlag(turbo::string_view input, T* dst, std::string* error);
template <typename T>
inline std::string UnparseFlag(const T& v);

namespace flags_internal {

// Overloads of `TurboParseFlag()` and `TurboUnparseFlag()` for fundamental types.
bool TurboParseFlag(turbo::string_view, bool*, std::string*);
bool TurboParseFlag(turbo::string_view, short*, std::string*);           // NOLINT
bool TurboParseFlag(turbo::string_view, unsigned short*, std::string*);  // NOLINT
bool TurboParseFlag(turbo::string_view, int*, std::string*);             // NOLINT
bool TurboParseFlag(turbo::string_view, unsigned int*, std::string*);    // NOLINT
bool TurboParseFlag(turbo::string_view, long*, std::string*);            // NOLINT
bool TurboParseFlag(turbo::string_view, unsigned long*, std::string*);   // NOLINT
bool TurboParseFlag(turbo::string_view, long long*, std::string*);       // NOLINT
bool TurboParseFlag(turbo::string_view, unsigned long long*,             // NOLINT
                   std::string*);
bool TurboParseFlag(turbo::string_view, float*, std::string*);
bool TurboParseFlag(turbo::string_view, double*, std::string*);
bool TurboParseFlag(turbo::string_view, std::string*, std::string*);
bool TurboParseFlag(turbo::string_view, std::vector<std::string>*, std::string*);

template <typename T>
bool TurboParseFlag(turbo::string_view text, turbo::optional<T>* f,
                   std::string* err) {
  if (text.empty()) {
    *f = turbo::nullopt;
    return true;
  }
  T value;
  if (!turbo::ParseFlag(text, &value, err)) return false;

  *f = std::move(value);
  return true;
}

#if defined(TURBO_HAVE_STD_OPTIONAL) && !defined(TURBO_USES_STD_OPTIONAL)
template <typename T>
bool TurboParseFlag(turbo::string_view text, std::optional<T>* f,
                   std::string* err) {
  if (text.empty()) {
    *f = std::nullopt;
    return true;
  }
  T value;
  if (!turbo::ParseFlag(text, &value, err)) return false;

  *f = std::move(value);
  return true;
}
#endif

template <typename T>
bool InvokeParseFlag(turbo::string_view input, T* dst, std::string* err) {
  // Comment on next line provides a good compiler error message if T
  // does not have TurboParseFlag(turbo::string_view, T*, std::string*).
  return TurboParseFlag(input, dst, err);  // Is T missing TurboParseFlag?
}

// Strings and std:: containers do not have the same overload resolution
// considerations as fundamental types. Naming these 'TurboUnparseFlag' means we
// can avoid the need for additional specializations of Unparse (below).
std::string TurboUnparseFlag(turbo::string_view v);
std::string TurboUnparseFlag(const std::vector<std::string>&);

template <typename T>
std::string TurboUnparseFlag(const turbo::optional<T>& f) {
  return f.has_value() ? turbo::UnparseFlag(*f) : "";
}

#if defined(TURBO_HAVE_STD_OPTIONAL) && !defined(TURBO_USES_STD_OPTIONAL)
template <typename T>
std::string TurboUnparseFlag(const std::optional<T>& f) {
  return f.has_value() ? turbo::UnparseFlag(*f) : "";
}
#endif

template <typename T>
std::string Unparse(const T& v) {
  // Comment on next line provides a good compiler error message if T does not
  // have UnparseFlag.
  return TurboUnparseFlag(v);  // Is T missing TurboUnparseFlag?
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
std::string Unparse(float v);
std::string Unparse(double v);

}  // namespace flags_internal

// ParseFlag()
//
// Parses a string value into a flag value of type `T`. Do not add overloads of
// this function for your type directly; instead, add an `TurboParseFlag()`
// free function as documented above.
//
// Some implementations of `TurboParseFlag()` for types which consist of other,
// constituent types which already have Turbo flag support, may need to call
// `turbo::ParseFlag()` on those consituent string values. (See above.)
template <typename T>
inline bool ParseFlag(turbo::string_view input, T* dst, std::string* error) {
  return flags_internal::InvokeParseFlag(input, dst, error);
}

// UnparseFlag()
//
// Unparses a flag value of type `T` into a string value. Do not add overloads
// of this function for your type directly; instead, add an `TurboUnparseFlag()`
// free function as documented above.
//
// Some implementations of `TurboUnparseFlag()` for types which consist of other,
// constituent types which already have Turbo flag support, may want to call
// `turbo::UnparseFlag()` on those constituent types. (See above.)
template <typename T>
inline std::string UnparseFlag(const T& v) {
  return flags_internal::Unparse(v);
}

// Overloads for `turbo::LogSeverity` can't (easily) appear alongside that type's
// definition because it is layered below flags.  See proper documentation in
// base/log_severity.h.
enum class LogSeverity : int;
bool TurboParseFlag(turbo::string_view, turbo::LogSeverity*, std::string*);
std::string TurboUnparseFlag(turbo::LogSeverity);

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_MARSHALLING_H_
