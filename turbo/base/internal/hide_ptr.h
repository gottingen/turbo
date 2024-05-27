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

#ifndef TURBO_BASE_INTERNAL_HIDE_PTR_H_
#define TURBO_BASE_INTERNAL_HIDE_PTR_H_

#include <cstdint>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// Arbitrary value with high bits set. Xor'ing with it is unlikely
// to map one valid pointer to another valid pointer.
constexpr uintptr_t HideMask() {
  return (uintptr_t{0xF03A5F7BU} << (sizeof(uintptr_t) - 4) * 8) | 0xF03A5F7BU;
}

// Hide a pointer from the leak checker. For internal use only.
// Differs from turbo::IgnoreLeak(ptr) in that turbo::IgnoreLeak(ptr) causes ptr
// and all objects reachable from ptr to be ignored by the leak checker.
template <class T>
inline uintptr_t HidePtr(T* ptr) {
  return reinterpret_cast<uintptr_t>(ptr) ^ HideMask();
}

// Return a pointer that has been hidden from the leak checker.
// For internal use only.
template <class T>
inline T* UnhidePtr(uintptr_t hidden) {
  return reinterpret_cast<T*>(hidden ^ HideMask());
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_HIDE_PTR_H_
