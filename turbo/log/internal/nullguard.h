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
// File: log/internal/nullguard.h
// -----------------------------------------------------------------------------
//
// NullGuard exists such that NullGuard<T>::Guard(v) returns v, unless passed a
// nullptr_t, or a null char* or const char*, in which case it returns "(null)".
// This allows streaming NullGuard<T>::Guard(v) to an output stream without
// hitting undefined behavior for null values.

#ifndef TURBO_LOG_INTERNAL_NULLGUARD_H_
#define TURBO_LOG_INTERNAL_NULLGUARD_H_

#include <array>
#include <cstddef>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

TURBO_CONST_INIT TURBO_DLL extern const std::array<char, 7> kCharNull;
TURBO_CONST_INIT TURBO_DLL extern const std::array<signed char, 7>
    kSignedCharNull;
TURBO_CONST_INIT TURBO_DLL extern const std::array<unsigned char, 7>
    kUnsignedCharNull;

template <typename T>
struct NullGuard final {
  static const T& Guard(const T& v) { return v; }
};
template <>
struct NullGuard<char*> final {
  static const char* Guard(const char* v) { return v ? v : kCharNull.data(); }
};
template <>
struct NullGuard<const char*> final {
  static const char* Guard(const char* v) { return v ? v : kCharNull.data(); }
};
template <>
struct NullGuard<signed char*> final {
  static const signed char* Guard(const signed char* v) {
    return v ? v : kSignedCharNull.data();
  }
};
template <>
struct NullGuard<const signed char*> final {
  static const signed char* Guard(const signed char* v) {
    return v ? v : kSignedCharNull.data();
  }
};
template <>
struct NullGuard<unsigned char*> final {
  static const unsigned char* Guard(const unsigned char* v) {
    return v ? v : kUnsignedCharNull.data();
  }
};
template <>
struct NullGuard<const unsigned char*> final {
  static const unsigned char* Guard(const unsigned char* v) {
    return v ? v : kUnsignedCharNull.data();
  }
};
template <>
struct NullGuard<std::nullptr_t> final {
  static const char* Guard(const std::nullptr_t&) { return kCharNull.data(); }
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_NULLGUARD_H_
