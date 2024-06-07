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

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>

#include <turbo/algorithm/algorithm.h>
#include <turbo/base/internal/throw_delegate.h>
#include <turbo/meta/type_traits.h>

namespace turbo {

    template<typename T>
    class span;

    namespace span_internal {
        // Wrappers for access to container data pointers.
        template<typename C>
        constexpr auto GetDataImpl(C &c, char) noexcept  // NOLINT(runtime/references)
        -> decltype(c.data()) {
            return c.data();
        }

        // Before C++17, std::string::data returns a const char* in all cases.
        inline char *GetDataImpl(std::string &s,  // NOLINT(runtime/references)
                                 int) noexcept {
            return &s[0];
        }

        template<typename C>
        constexpr auto GetData(C &c) noexcept  // NOLINT(runtime/references)
        -> decltype(GetDataImpl(c, 0)) {
            return GetDataImpl(c, 0);
        }

        // Detection idioms for size() and data().
        template<typename C>
        using HasSize =
                std::is_integral<turbo::decay_t<decltype(std::declval<C &>().size())>>;

        // We want to enable conversion from vector<T*> to span<const T* const> but
        // disable conversion from vector<Derived> to span<Base>. Here we use
        // the fact that U** is convertible to Q* const* if and only if Q is the same
        // type or a more cv-qualified version of U.  We also decay the result type of
        // data() to avoid problems with classes which have a member function data()
        // which returns a reference.
        template<typename T, typename C>
        using HasData =
                std::is_convertible<turbo::decay_t<decltype(GetData(std::declval<C &>()))> *,
                        T *const *>;

        // Extracts value type from a Container
        template<typename C>
        struct ElementType {
            using type = typename turbo::remove_reference_t<C>::value_type;
        };

        template<typename T, size_t N>
        struct ElementType<T (&)[N]> {
            using type = T;
        };

        template<typename C>
        using ElementT = typename ElementType<C>::type;

        template<typename T>
        using EnableIfMutable =
                typename std::enable_if<!std::is_const<T>::value, int>::type;

        template<template<typename> class SpanT, typename T>
        bool EqualImpl(SpanT<T> a, SpanT<T> b) {
            static_assert(std::is_const<T>::value, "");
            return std::equal(a.begin(), a.end(), b.begin(), b.end());
        }

        template<template<typename> class SpanT, typename T>
        bool LessThanImpl(SpanT<T> a, SpanT<T> b) {
            // We can't use value_type since that is remove_cv_t<T>, so we go the long way
            // around.
            static_assert(std::is_const<T>::value, "");
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
        }

        template<typename From, typename To>
        using EnableIfConvertibleTo =
                typename std::enable_if<std::is_convertible<From, To>::value>::type;

        // IsView is true for types where the return type of .data() is the same for
        // mutable and const instances. This isn't foolproof, but it's only used to
        // enable a compiler warning.
        template<typename T, typename = void, typename = void>
        struct IsView {
            static constexpr bool value = false;
        };

        template<typename T>
        struct IsView<
                T, turbo::void_t<decltype(span_internal::GetData(std::declval<const T &>()))>,
                turbo::void_t<decltype(span_internal::GetData(std::declval<T &>()))>> {
        private:
            using Container = std::remove_const_t<T>;
            using ConstData =
                    decltype(span_internal::GetData(std::declval<const Container &>()));
            using MutData = decltype(span_internal::GetData(std::declval<Container &>()));
        public:
            static constexpr bool value = std::is_same<ConstData, MutData>::value;
        };

        // These enablers result in 'int' so they can be used as typenames or defaults
        // in template parameters lists.
        template<typename T>
        using EnableIfIsView = std::enable_if_t<IsView<T>::value, int>;

        template<typename T>
        using EnableIfNotIsView = std::enable_if_t<!IsView<T>::value, int>;

    }  // namespace span_internal
}  // namespace turbo
