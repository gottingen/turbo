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

#include <turbo/strings/match.h>

#include <algorithm>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/endian.h>
#include <turbo/base/optimization.h>
#include <turbo/numeric/bits.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/internal/memutil.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

bool equals_ignore_case(turbo::string_view piece1,
                      turbo::string_view piece2) noexcept {
  return (piece1.size() == piece2.size() &&
          0 == turbo::strings_internal::memcasecmp(piece1.data(), piece2.data(),
                                                  piece1.size()));
  // memcasecmp uses turbo::ascii_tolower().
}

bool str_contains_ignore_case(turbo::string_view haystack,
                           turbo::string_view needle) noexcept {
  while (haystack.size() >= needle.size()) {
    if (starts_with_ignore_case(haystack, needle)) return true;
    haystack.remove_prefix(1);
  }
  return false;
}

bool str_contains_ignore_case(turbo::string_view haystack,
                           char needle) noexcept {
  char upper_needle = turbo::ascii_toupper(static_cast<unsigned char>(needle));
  char lower_needle = turbo::ascii_tolower(static_cast<unsigned char>(needle));
  if (upper_needle == lower_needle) {
    return str_contains(haystack, needle);
  } else {
    const char both_cstr[3] = {lower_needle, upper_needle, '\0'};
    return haystack.find_first_of(both_cstr) != turbo::string_view::npos;
  }
}

bool starts_with_ignore_case(turbo::string_view text,
                          turbo::string_view prefix) noexcept {
  return (text.size() >= prefix.size()) &&
         equals_ignore_case(text.substr(0, prefix.size()), prefix);
}

bool ends_with_ignore_case(turbo::string_view text,
                        turbo::string_view suffix) noexcept {
  return (text.size() >= suffix.size()) &&
         equals_ignore_case(text.substr(text.size() - suffix.size()), suffix);
}

turbo::string_view find_longest_common_prefix(turbo::string_view a,
                                          turbo::string_view b) {
  const turbo::string_view::size_type limit = std::min(a.size(), b.size());
  const char* const pa = a.data();
  const char* const pb = b.data();
  turbo::string_view::size_type count = (unsigned) 0;

  if (TURBO_UNLIKELY(limit < 8)) {
    while (TURBO_LIKELY(count + 2 <= limit)) {
      uint16_t xor_bytes = turbo::little_endian::load16(pa + count) ^
                           turbo::little_endian::load16(pb + count);
      if (TURBO_UNLIKELY(xor_bytes != 0)) {
        if (TURBO_LIKELY((xor_bytes & 0xff) == 0)) ++count;
        return turbo::string_view(pa, count);
      }
      count += 2;
    }
    if (TURBO_LIKELY(count != limit)) {
      if (TURBO_LIKELY(pa[count] == pb[count])) ++count;
    }
    return turbo::string_view(pa, count);
  }

  do {
    uint64_t xor_bytes = turbo::little_endian::load64(pa + count) ^
                         turbo::little_endian::load64(pb + count);
    if (TURBO_UNLIKELY(xor_bytes != 0)) {
      count += static_cast<uint64_t>(turbo::countr_zero(xor_bytes) >> 3);
      return turbo::string_view(pa, count);
    }
    count += 8;
  } while (TURBO_LIKELY(count + 8 < limit));

  count = limit - 8;
  uint64_t xor_bytes = turbo::little_endian::load64(pa + count) ^
                       turbo::little_endian::load64(pb + count);
  if (TURBO_LIKELY(xor_bytes != 0)) {
    count += static_cast<uint64_t>(turbo::countr_zero(xor_bytes) >> 3);
    return turbo::string_view(pa, count);
  }
  return turbo::string_view(pa, limit);
}

turbo::string_view find_longest_common_suffix(turbo::string_view a,
                                          turbo::string_view b) {
  const turbo::string_view::size_type limit = std::min(a.size(), b.size());
  if (limit == 0) return turbo::string_view();

  const char* pa = a.data() + a.size() - 1;
  const char* pb = b.data() + b.size() - 1;
  turbo::string_view::size_type count = (unsigned) 0;
  while (count < limit && *pa == *pb) {
    --pa;
    --pb;
    ++count;
  }

  return turbo::string_view(++pa, count);
}

TURBO_NAMESPACE_END
}  // namespace turbo
