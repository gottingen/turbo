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
// Created by jeff on 24-6-2.
//

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace turbo {

    template<typename OutputType, typename InputType>
    inline OutputType checked_cast(InputType &&value) {
        static_assert(std::is_class<typename std::remove_pointer<
                              typename std::remove_reference<InputType>::type>::type>::value,
                      "checked_cast input type must be a class");
        static_assert(std::is_class<typename std::remove_pointer<
                              typename std::remove_reference<OutputType>::type>::type>::value,
                      "checked_cast output type must be a class");
#ifdef NDEBUG
        return static_cast<OutputType>(value);
#else
        return dynamic_cast<OutputType>(value);
#endif
    }

    template<class T, class U>
    std::shared_ptr<T> checked_pointer_cast(std::shared_ptr<U> r) noexcept {
#ifdef NDEBUG
        return std::static_pointer_cast<T>(std::move(r));
#else
        return std::dynamic_pointer_cast<T>(std::move(r));
#endif
    }

    template<class T, class U>
    std::unique_ptr<T> checked_pointer_cast(std::unique_ptr<U> r) noexcept {
#ifdef NDEBUG
        return std::unique_ptr<T>(static_cast<T*>(r.release()));
#else
        return std::unique_ptr<T>(dynamic_cast<T *>(r.release()));
#endif
    }

}  // namespace tubo