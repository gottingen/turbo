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

#ifndef TURBO_STRINGS_HAS_OSTREAM_OPERATOR_H_
#define TURBO_STRINGS_HAS_OSTREAM_OPERATOR_H_

#include <ostream>
#include <type_traits>
#include <utility>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Detects if type `T` supports streaming to `std::ostream`s with `operator<<`.

template <typename T, typename = void>
struct HasOstreamOperator : std::false_type {};

template <typename T>
struct HasOstreamOperator<
    T, std::enable_if_t<std::is_same<
           std::ostream&, decltype(std::declval<std::ostream&>()
                                   << std::declval<const T&>())>::value>>
    : std::true_type {};

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_HAS_OSTREAM_OPERATOR_H_
