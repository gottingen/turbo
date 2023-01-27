//
// Copyright 2017 The Turbo Authors.
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
// File: string_view.h
// -----------------------------------------------------------------------------
//
// This file contains the definition of the `std::string_view` class. A
// `string_view` points to a contiguous span of characters, often part or all of
// another `std::string`, double-quoted string literal, character array, or even
// another `string_view`.
//
// This `std::string_view` abstraction is designed to be a drop-in
// replacement for the C++17 `std::string_view` abstraction.
#ifndef TURBO_STRINGS_STRING_VIEW_H_
#define TURBO_STRINGS_STRING_VIEW_H_


#include "turbo/platform/config.h"
#include <string_view>  // IWYU pragma: export

namespace turbo {
TURBO_NAMESPACE_BEGIN

// ClippedSubstr()
//
// Like `s.substr(pos, n)`, but clips `pos` to an upper bound of `s.size()`.
// Provided because std::string_view::substr throws if `pos > size()`
inline std::string_view ClippedSubstr(std::string_view s, size_t pos,
                                 size_t n = std::string_view::npos) {
  pos = (std::min)(pos, static_cast<size_t>(s.size()));
  return s.substr(pos, n);
}

// NullSafeStringView()
//
// Creates an `std::string_view` from a pointer `p` even if it's null-valued.
// This function should be used where an `std::string_view` can be created from
// a possibly-null pointer.
constexpr std::string_view NullSafeStringView(const char* p) {
  return p ? std::string_view(p) : std::string_view();
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_STRING_VIEW_H_
