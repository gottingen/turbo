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

#ifndef TURBO_CONTAINER_HASH_CONTAINER_DEFAULTS_H_
#define TURBO_CONTAINER_HASH_CONTAINER_DEFAULTS_H_

#include <turbo/base/config.h>
#include <turbo/container/internal/hash_function_defaults.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// DefaultHashContainerHash is a convenience alias for the functor that is used
// by default by Turbo hash-based (unordered) containers for hashing when
// `Hash` type argument is not explicitly specified.
//
// This type alias can be used by generic code that wants to provide more
// flexibility for defining underlying containers.
template <typename T>
using DefaultHashContainerHash = turbo::container_internal::hash_default_hash<T>;

// DefaultHashContainerEq is a convenience alias for the functor that is used by
// default by Turbo hash-based (unordered) containers for equality check when
// `Eq` type argument is not explicitly specified.
//
// This type alias can be used by generic code that wants to provide more
// flexibility for defining underlying containers.
template <typename T>
using DefaultHashContainerEq = turbo::container_internal::hash_default_eq<T>;

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_HASH_CONTAINER_DEFAULTS_H_
