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
// Created by jeff on 24-6-1.
//

#pragma once


#include <memory>
#include <type_traits>
#include <utility>

#include <turbo/base/macros.h>

namespace turbo {
    /// CRTP helper for declaring equality comparison. Defines operator== and operator!=
    template<typename T>
    class EqualityComparable {
    public:
        ~EqualityComparable() {
            static_assert(
                    std::is_same<decltype(std::declval<const T>().equals(std::declval<const T>())),
                            bool>::value,
                    "EqualityComparable depends on the method T::equals(const T&) const");
        }

        template<typename... Extra>
        bool equals(const std::shared_ptr<T> &other, Extra &&... extra) const {
            if (other == nullptr) {
                return false;
            }
            return cast().equals(*other, std::forward<Extra>(extra)...);
        }

        struct PtrsEqual {
            bool operator()(const std::shared_ptr<T> &l, const std::shared_ptr<T> &r) const {
                return l->equals(*r);
            }
        };

        friend bool operator==(T const &a, T const &b) { return a.equals(b); }

        friend bool operator!=(T const &a, T const &b) { return !(a == b); }

    private:
        const T &cast() const { return static_cast<const T &>(*this); }
    };

}  // namespace turbo

