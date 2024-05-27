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

#include <turbo/strings/string_view.h>

#ifndef TURBO_USES_STD_STRING_VIEW

#include <algorithm>
#include <climits>
#include <cstring>
#include <ostream>

#include <turbo/base/nullability.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

// This is significantly faster for case-sensitive matches with very
// few possible matches.
turbo::Nullable<const char*> memmatch(turbo::Nullable<const char*> phaystack,
                                     size_t haylen,
                                     turbo::Nullable<const char*> pneedle,
                                     size_t neelen) {
  if (0 == neelen) {
    return phaystack;  // even if haylen is 0
  }
  if (haylen < neelen) return nullptr;

  const char* match;
  const char* hayend = phaystack + haylen - neelen + 1;
  // A static cast is used here as memchr returns a const void *, and pointer
  // arithmetic is not allowed on pointers to void.
  while (
      (match = static_cast<const char*>(memchr(
           phaystack, pneedle[0], static_cast<size_t>(hayend - phaystack))))) {
    if (memcmp(match, pneedle, neelen) == 0)
      return match;
    else
      phaystack = match + 1;
  }
  return nullptr;
}

void WritePadding(std::ostream& o, size_t pad) {
  char fill_buf[32];
  memset(fill_buf, o.fill(), sizeof(fill_buf));
  while (pad) {
    size_t n = std::min(pad, sizeof(fill_buf));
    o.write(fill_buf, static_cast<std::streamsize>(n));
    pad -= n;
  }
}

class LookupTable {
 public:
  // For each character in wanted, sets the index corresponding
  // to the ASCII code of that character. This is used by
  // the find_.*_of methods below to tell whether or not a character is in
  // the lookup table in constant time.
  explicit LookupTable(string_view wanted) {
    for (char c : wanted) {
      table_[Index(c)] = true;
    }
  }
  bool operator[](char c) const { return table_[Index(c)]; }

 private:
  static unsigned char Index(char c) { return static_cast<unsigned char>(c); }
  bool table_[UCHAR_MAX + 1] = {};
};

}  // namespace

std::ostream& operator<<(std::ostream& o, string_view piece) {
  std::ostream::sentry sentry(o);
  if (sentry) {
    size_t lpad = 0;
    size_t rpad = 0;
    if (static_cast<size_t>(o.width()) > piece.size()) {
      size_t pad = static_cast<size_t>(o.width()) - piece.size();
      if ((o.flags() & o.adjustfield) == o.left) {
        rpad = pad;
      } else {
        lpad = pad;
      }
    }
    if (lpad) WritePadding(o, lpad);
    o.write(piece.data(), static_cast<std::streamsize>(piece.size()));
    if (rpad) WritePadding(o, rpad);
    o.width(0);
  }
  return o;
}

string_view::size_type string_view::find(string_view s,
                                         size_type pos) const noexcept {
  if (empty() || pos > length_) {
    if (empty() && pos == 0 && s.empty()) return 0;
    return npos;
  }
  const char* result = memmatch(ptr_ + pos, length_ - pos, s.ptr_, s.length_);
  return result ? static_cast<size_type>(result - ptr_) : npos;
}

string_view::size_type string_view::find(char c, size_type pos) const noexcept {
  if (empty() || pos >= length_) {
    return npos;
  }
  const char* result =
      static_cast<const char*>(memchr(ptr_ + pos, c, length_ - pos));
  return result != nullptr ? static_cast<size_type>(result - ptr_) : npos;
}

string_view::size_type string_view::rfind(string_view s,
                                          size_type pos) const noexcept {
  if (length_ < s.length_) return npos;
  if (s.empty()) return std::min(length_, pos);
  const char* last = ptr_ + std::min(length_ - s.length_, pos) + s.length_;
  const char* result = std::find_end(ptr_, last, s.ptr_, s.ptr_ + s.length_);
  return result != last ? static_cast<size_type>(result - ptr_) : npos;
}

// Search range is [0..pos] inclusive.  If pos == npos, search everything.
string_view::size_type string_view::rfind(char c,
                                          size_type pos) const noexcept {
  // Note: memrchr() is not available on Windows.
  if (empty()) return npos;
  for (size_type i = std::min(pos, length_ - 1);; --i) {
    if (ptr_[i] == c) {
      return i;
    }
    if (i == 0) break;
  }
  return npos;
}

string_view::size_type string_view::find_first_of(
    string_view s, size_type pos) const noexcept {
  if (empty() || s.empty()) {
    return npos;
  }
  // Avoid the cost of LookupTable() for a single-character search.
  if (s.length_ == 1) return find_first_of(s.ptr_[0], pos);
  LookupTable tbl(s);
  for (size_type i = pos; i < length_; ++i) {
    if (tbl[ptr_[i]]) {
      return i;
    }
  }
  return npos;
}

string_view::size_type string_view::find_first_not_of(
    string_view s, size_type pos) const noexcept {
  if (empty()) return npos;
  // Avoid the cost of LookupTable() for a single-character search.
  if (s.length_ == 1) return find_first_not_of(s.ptr_[0], pos);
  LookupTable tbl(s);
  for (size_type i = pos; i < length_; ++i) {
    if (!tbl[ptr_[i]]) {
      return i;
    }
  }
  return npos;
}

string_view::size_type string_view::find_first_not_of(
    char c, size_type pos) const noexcept {
  if (empty()) return npos;
  for (; pos < length_; ++pos) {
    if (ptr_[pos] != c) {
      return pos;
    }
  }
  return npos;
}

string_view::size_type string_view::find_last_of(string_view s,
                                                 size_type pos) const noexcept {
  if (empty() || s.empty()) return npos;
  // Avoid the cost of LookupTable() for a single-character search.
  if (s.length_ == 1) return find_last_of(s.ptr_[0], pos);
  LookupTable tbl(s);
  for (size_type i = std::min(pos, length_ - 1);; --i) {
    if (tbl[ptr_[i]]) {
      return i;
    }
    if (i == 0) break;
  }
  return npos;
}

string_view::size_type string_view::find_last_not_of(
    string_view s, size_type pos) const noexcept {
  if (empty()) return npos;
  size_type i = std::min(pos, length_ - 1);
  if (s.empty()) return i;
  // Avoid the cost of LookupTable() for a single-character search.
  if (s.length_ == 1) return find_last_not_of(s.ptr_[0], pos);
  LookupTable tbl(s);
  for (;; --i) {
    if (!tbl[ptr_[i]]) {
      return i;
    }
    if (i == 0) break;
  }
  return npos;
}

string_view::size_type string_view::find_last_not_of(
    char c, size_type pos) const noexcept {
  if (empty()) return npos;
  size_type i = std::min(pos, length_ - 1);
  for (;; --i) {
    if (ptr_[i] != c) {
      return i;
    }
    if (i == 0) break;
  }
  return npos;
}

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr string_view::size_type string_view::npos;
constexpr string_view::size_type string_view::kMaxSize;
#endif

TURBO_NAMESPACE_END
}  // namespace turbo

#else

// https://github.com/abseil/abseil-cpp/issues/1465
// CMake builds on Apple platforms error when libraries are empty.
// Our CMake configuration can avoid this error on header-only libraries,
// but since this library is conditionally empty, including a single
// variable is an easy workaround.
#ifdef __APPLE__
namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {
extern const char kAvoidEmptyStringViewLibraryWarning;
const char kAvoidEmptyStringViewLibraryWarning = 0;
}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // __APPLE__

#endif  // TURBO_USES_STD_STRING_VIEW
