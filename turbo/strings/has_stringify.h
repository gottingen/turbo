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

#include <type_traits>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    namespace strings_internal {

        // This is an empty class not intended to be used. It exists so that
        // `HasTurboStringify` can reference a universal class rather than needing to be
        // copied for each new sink.
        class UnimplementedSink {
        public:
            void Append(size_t count, char ch);

            void Append(std::string_view v);

            // Support `turbo::format(&sink, format, args...)`.
            friend void TurboFormatFlush(UnimplementedSink *sink, std::string_view v);
        };

    }  // namespace strings_internal

    // `HasTurboStringify<T>` detects if type `T` supports the `turbo_stringify()`
    //
    // Note that there are types that can be `str_cat`-ed that do not use the
    // `turbo_stringify` customization point (for example, `int`).

    template<typename T, typename = void>
    struct HasTurboStringify : std::false_type {
    };

    template<typename T>
    struct HasTurboStringify<
            T, std::enable_if_t<std::is_void<decltype(turbo_stringify(
                    std::declval<strings_internal::UnimplementedSink &>(),
                    std::declval<const T &>()))>::value>> : std::true_type {
    };

}  // namespace turbo
