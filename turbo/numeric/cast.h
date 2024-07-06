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

#pragma once

#include <limits>
#include <type_traits>
#include <cassert>
#include <turbo/base/casts.h>

namespace turbo {

    inline float double_to_float(double value) {
        if (value < std::numeric_limits<float>::lowest())
            return -std::numeric_limits<float>::infinity();
        if (value > std::numeric_limits<float>::max())
            return std::numeric_limits<float>::infinity();

        return static_cast<float>(value);
    }

    inline float double_to_finite_float(double value) {
        if (value < std::numeric_limits<float>::lowest())
            return std::numeric_limits<float>::lowest();
        if (value > std::numeric_limits<float>::max())
            return std::numeric_limits<float>::max();

        return static_cast<float>(value);
    }

    template <typename To, typename From>
    inline To down_cast(From* f) {
        static_assert(
                (std::is_base_of<From, typename std::remove_pointer<To>::type>::value),
                "target type not derived from source type");

#if !defined(__GNUC__) || defined(__GXX_RTTI)

        assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
#endif

        return static_cast<To>(f);
    }

    template <typename To, typename From>
    inline To down_cast(From& f) {
        static_assert(std::is_lvalue_reference<To>::value,
                      "target type not a reference");
        static_assert(
                (std::is_base_of<From, typename std::remove_reference<To>::type>::value),
                "target type not derived from source type");

#if !defined(__GNUC__) || defined(__GXX_RTTI)

        assert(dynamic_cast<typename std::remove_reference<To>::type*>(&f) !=
               nullptr);
#endif

        return static_cast<To>(f);
    }

}  // namespace turbo
