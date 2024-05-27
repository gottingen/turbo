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

#ifndef TURBO_BASE_INTERNAL_UNALIGNED_ACCESS_H_
#define TURBO_BASE_INTERNAL_UNALIGNED_ACCESS_H_

#include <string.h>

#include <cstdint>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/nullability.h>

// unaligned APIs

// Portable handling of unaligned loads, stores, and copies.

// The unaligned API is C++ only.  The declarations use C++ features
// (namespaces, inline) which are absent or incompatible in C.
#if defined(__cplusplus)
namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

inline uint16_t UnalignedLoad16(turbo::Nonnull<const void *> p) {
  uint16_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint32_t UnalignedLoad32(turbo::Nonnull<const void *> p) {
  uint32_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline uint64_t UnalignedLoad64(turbo::Nonnull<const void *> p) {
  uint64_t t;
  memcpy(&t, p, sizeof t);
  return t;
}

inline void UnalignedStore16(turbo::Nonnull<void *> p, uint16_t v) {
  memcpy(p, &v, sizeof v);
}

inline void UnalignedStore32(turbo::Nonnull<void *> p, uint32_t v) {
  memcpy(p, &v, sizeof v);
}

inline void UnalignedStore64(turbo::Nonnull<void *> p, uint64_t v) {
  memcpy(p, &v, sizeof v);
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#define TURBO_INTERNAL_UNALIGNED_LOAD16(_p) \
  (turbo::base_internal::UnalignedLoad16(_p))
#define TURBO_INTERNAL_UNALIGNED_LOAD32(_p) \
  (turbo::base_internal::UnalignedLoad32(_p))
#define TURBO_INTERNAL_UNALIGNED_LOAD64(_p) \
  (turbo::base_internal::UnalignedLoad64(_p))

#define TURBO_INTERNAL_UNALIGNED_STORE16(_p, _val) \
  (turbo::base_internal::UnalignedStore16(_p, _val))
#define TURBO_INTERNAL_UNALIGNED_STORE32(_p, _val) \
  (turbo::base_internal::UnalignedStore32(_p, _val))
#define TURBO_INTERNAL_UNALIGNED_STORE64(_p, _val) \
  (turbo::base_internal::UnalignedStore64(_p, _val))

#endif  // defined(__cplusplus), end of unaligned API

#endif  // TURBO_BASE_INTERNAL_UNALIGNED_ACCESS_H_
