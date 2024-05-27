//
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

#ifndef TURBO_BASE_INTERNAL_FAST_TYPE_ID_H_
#define TURBO_BASE_INTERNAL_FAST_TYPE_ID_H_

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

template <typename Type>
struct FastTypeTag {
  constexpr static char dummy_var = 0;
};

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
template <typename Type>
constexpr char FastTypeTag<Type>::dummy_var;
#endif

// FastTypeId<Type>() evaluates at compile/link-time to a unique pointer for the
// passed-in type. These are meant to be good match for keys into maps or
// straight up comparisons.
using FastTypeIdType = const void*;

template <typename Type>
constexpr inline FastTypeIdType FastTypeId() {
  return &FastTypeTag<Type>::dummy_var;
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_FAST_TYPE_ID_H_
