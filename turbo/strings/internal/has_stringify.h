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

#pragma once

#include <turbo/strings/has_stringify.h>

#include <type_traits>
#include <utility>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace strings_internal {

// This exists to fix a circular dependency problem with the GoogleTest release.
// GoogleTest referenced this internal file and this internal trait.  Since
// simultaneous releases are not possible since once release must reference
// another, we will temporarily add this back.
// https://github.com/google/googletest/blob/v1.14.x/googletest/include/gtest/gtest-printers.h#L119
//
// This file can be deleted after the next Turbo and GoogleTest release.
//
// https://github.com/google/googletest/pull/4368#issuecomment-1717699895
// https://github.com/google/googletest/pull/4368#issuecomment-1717699895
template <typename T, typename = void>
struct HasTurboStringify : std::false_type {};

template <typename T>
struct HasTurboStringify<
    T, std::enable_if_t<std::is_void<decltype(turbo_stringify(
           std::declval<strings_internal::UnimplementedSink&>(),
           std::declval<const T&>()))>::value>> : std::true_type {};

}  // namespace strings_internal

TURBO_NAMESPACE_END
}  // namespace turbo
