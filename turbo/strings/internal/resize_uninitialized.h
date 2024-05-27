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

#ifndef TURBO_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_
#define TURBO_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>

#include <turbo/base/port.h>
#include <turbo/meta/type_traits.h>  //  for void_t

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

// In this type trait, we look for a __resize_default_init member function, and
// we use it if available, otherwise, we use resize. We provide HasMember to
// indicate whether __resize_default_init is present.
template <typename string_type, typename = void>
struct ResizeUninitializedTraits {
  using HasMember = std::false_type;
  static void Resize(string_type* s, size_t new_size) { s->resize(new_size); }
};

// __resize_default_init is provided by libc++ >= 8.0
template <typename string_type>
struct ResizeUninitializedTraits<
    string_type, turbo::void_t<decltype(std::declval<string_type&>()
                                           .__resize_default_init(237))> > {
  using HasMember = std::true_type;
  static void Resize(string_type* s, size_t new_size) {
    s->__resize_default_init(new_size);
  }
};

// Returns true if the std::string implementation supports a resize where
// the new characters added to the std::string are left untouched.
//
// (A better name might be "STLStringSupportsUninitializedResize", alluding to
// the previous function.)
template <typename string_type>
inline constexpr bool STLStringSupportsNontrashingResize(string_type*) {
  return ResizeUninitializedTraits<string_type>::HasMember::value;
}

// Like str->resize(new_size), except any new characters added to "*str" as a
// result of resizing may be left uninitialized, rather than being filled with
// '0' bytes. Typically used when code is then going to overwrite the backing
// store of the std::string with known data.
template <typename string_type, typename = void>
inline void STLStringResizeUninitialized(string_type* s, size_t new_size) {
  ResizeUninitializedTraits<string_type>::Resize(s, new_size);
}

// Used to ensure exponential growth so that the amortized complexity of
// increasing the string size by a small amount is O(1), in contrast to
// O(str->size()) in the case of precise growth.
template <typename string_type>
void STLStringReserveAmortized(string_type* s, size_t new_size) {
  const size_t cap = s->capacity();
  if (new_size > cap) {
    // Make sure to always grow by at least a factor of 2x.
    s->reserve((std::max)(new_size, 2 * cap));
  }
}

// In this type trait, we look for an __append_default_init member function, and
// we use it if available, otherwise, we use append.
template <typename string_type, typename = void>
struct AppendUninitializedTraits {
  static void Append(string_type* s, size_t n) {
    s->append(n, typename string_type::value_type());
  }
};

template <typename string_type>
struct AppendUninitializedTraits<
    string_type, turbo::void_t<decltype(std::declval<string_type&>()
                                           .__append_default_init(237))> > {
  static void Append(string_type* s, size_t n) {
    s->__append_default_init(n);
  }
};

// Like STLStringResizeUninitialized(str, new_size), except guaranteed to use
// exponential growth so that the amortized complexity of increasing the string
// size by a small amount is O(1), in contrast to O(str->size()) in the case of
// precise growth.
template <typename string_type>
void STLStringResizeUninitializedAmortized(string_type* s, size_t new_size) {
  const size_t size = s->size();
  if (new_size > size) {
    AppendUninitializedTraits<string_type>::Append(s, new_size - size);
  } else {
    s->erase(new_size);
  }
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_RESIZE_UNINITIALIZED_H_
