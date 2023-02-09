//
// Copyright 2020 The Turbo Authors.
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
// File: strip.h
// -----------------------------------------------------------------------------
//
// This file contains various functions for stripping substrings from a string.
#ifndef TURBO_STRINGS_STRIP_H_
#define TURBO_STRINGS_STRIP_H_

#include <cstddef>
#include <string>

#include "turbo/platform/port.h"
#include "turbo/strings/ascii.h"
#include "turbo/strings/match.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

// ConsumePrefix()
//
// Strips the `expected` prefix, if found, from the start of `str`.
// If the operation succeeded, `true` is returned.  If not, `false`
// is returned and `str` is not modified.
//
// Example:
//
//   turbo::string_view input("abc");
//   EXPECT_TRUE(turbo::ConsumePrefix(&input, "a"));
//   EXPECT_EQ(input, "bc");
inline bool ConsumePrefix(turbo::string_view* str, turbo::string_view expected) {
  if (!turbo::StartsWith(*str, expected)) return false;
  str->remove_prefix(expected.size());
  return true;
}
// ConsumeSuffix()
//
// Strips the `expected` suffix, if found, from the end of `str`.
// If the operation succeeded, `true` is returned.  If not, `false`
// is returned and `str` is not modified.
//
// Example:
//
//   turbo::string_view input("abcdef");
//   EXPECT_TRUE(turbo::ConsumeSuffix(&input, "def"));
//   EXPECT_EQ(input, "abc");
inline bool ConsumeSuffix(turbo::string_view* str, turbo::string_view expected) {
  if (!turbo::EndsWith(*str, expected)) return false;
  str->remove_suffix(expected.size());
  return true;
}

// StripPrefix()
//
// Returns a view into the input string `str` with the given `prefix` removed,
// but leaving the original string intact. If the prefix does not match at the
// start of the string, returns the original string instead.
TURBO_MUST_USE_RESULT inline turbo::string_view StripPrefix(
    turbo::string_view str, turbo::string_view prefix) {
  if (turbo::StartsWith(str, prefix)) str.remove_prefix(prefix.size());
  return str;
}

// StripSuffix()
//
// Returns a view into the input string `str` with the given `suffix` removed,
// but leaving the original string intact. If the suffix does not match at the
// end of the string, returns the original string instead.
TURBO_MUST_USE_RESULT inline turbo::string_view StripSuffix(
    turbo::string_view str, turbo::string_view suffix) {
  if (turbo::EndsWith(str, suffix)) str.remove_suffix(suffix.size());
  return str;
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_STRIP_H_
