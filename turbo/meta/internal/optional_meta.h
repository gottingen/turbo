// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef TURBO_META_INTERNAL_OPTIONAL_META_H_
#define TURBO_META_INTERNAL_OPTIONAL_META_H_

#include <type_traits>

#include "turbo/meta/internal/optional_internal.h"
#include "turbo/meta/type_traits.h"

namespace turbo {
    template<class T, class B = bool>
    class xmasked_value;

    namespace detail {
        template<class E>
        struct is_xmasked_value_impl : std::false_type {
        };

        template<class T, class B>
        struct is_xmasked_value_impl<xmasked_value<T, B>> : std::true_type {
        };
    }

    template<class E>
    using is_xmasked_value = detail::is_xmasked_value_impl<E>;

    template<class E, class R>
    using disable_xmasked_value = std::enable_if_t<!is_xmasked_value<E>::value, R>;

    template<class CT, class CB = bool>
    class optional_ref;

    namespace detail {
        template<class E>
        struct is_xoptional_impl : std::false_type {
        };

        template<class CT, class CB>
        struct is_xoptional_impl<optional_ref<CT, CB>> : std::true_type {
        };

        template<class CT, class CTO, class CBO>
        using converts_from_optional_ref = disjunction<
                std::is_constructible<CT, const optional_ref<CTO, CBO> &>,
                std::is_constructible<CT, optional_ref<CTO, CBO> &>,
                std::is_constructible<CT, const optional_ref<CTO, CBO> &&>,
                std::is_constructible<CT, optional_ref<CTO, CBO> &&>,
                std::is_convertible<const optional_ref<CTO, CBO> &, CT>,
                std::is_convertible<optional_ref<CTO, CBO> &, CT>,
                std::is_convertible<const optional_ref<CTO, CBO> &&, CT>,
                std::is_convertible<optional_ref<CTO, CBO> &&, CT>
        >;

        template<class CT, class CTO, class CBO>
        using assigns_from_xoptional = disjunction<
                std::is_assignable<std::add_lvalue_reference_t<CT>, const optional_ref<CTO, CBO> &>,
                std::is_assignable<std::add_lvalue_reference_t<CT>, optional_ref<CTO, CBO> &>,
                std::is_assignable<std::add_lvalue_reference_t<CT>, const optional_ref<CTO, CBO> &&>,
                std::is_assignable<std::add_lvalue_reference_t<CT>, optional_ref<CTO, CBO> &&>
        >;

        template<class... Args>
        struct common_optional_impl;

        template<class T>
        struct common_optional_impl<T> {
            using type = std::conditional_t<is_xoptional_impl<T>::value, T, optional_ref<T>>;
        };

        template<class T>
        struct identity {
            using type = T;
        };

        template<class T>
        struct get_value_type {
            using type = typename T::value_type;
        };

        template<class T1, class T2>
        struct common_optional_impl<T1, T2> {
            using decay_t1 = std::decay_t<T1>;
            using decay_t2 = std::decay_t<T2>;
            using type1 = mpl::eval_if_t<std::is_fundamental<decay_t1>, identity<decay_t1>, get_value_type<decay_t1>>;
            using type2 = mpl::eval_if_t<std::is_fundamental<decay_t2>, identity<decay_t2>, get_value_type<decay_t2>>;
            using type = optional_ref<std::common_type_t<type1, type2>>;
        };

        template<class T1, class T2, class B2>
        struct common_optional_impl<T1, optional_ref<T2, B2>>
                : common_optional_impl<T1, T2> {
        };

        template<class T1, class B1, class T2>
        struct common_optional_impl<optional_ref<T1, B1>, T2>
                : common_optional_impl<T1, T2> {
        };

        template<class T1, class B1, class T2, class B2>
        struct common_optional_impl<optional_ref<T1, B1>, optional_ref<T2, B2>>
                : common_optional_impl<T1, T2> {
        };

        template<class T1, class T2, class... Args>
        struct common_optional_impl<T1, T2, Args...> {
            using type = typename common_optional_impl<
                    typename common_optional_impl<T1, T2>::type,
                    Args...
            >::type;
        };
    }

    template<class E>
    using is_xoptional = detail::is_xoptional_impl<E>;

    template<class E, class R = void>
    using disable_xoptional = std::enable_if_t<!is_xoptional<E>::value, R>;

    template<class... Args>
    struct at_least_one_xoptional : std::disjunction<is_xoptional<Args>...> {
    };

    template<class... Args>
    struct common_optional : detail::common_optional_impl<Args...> {
    };

    template<class... Args>
    using common_optional_t = typename common_optional<Args...>::type;

    template<class E>
    struct is_not_xoptional_nor_xmasked_value : negation<disjunction<is_xoptional<E>, is_xmasked_value<E>>> {
    };
}

#endif  // TURBO_META_INTERNAL_OPTIONAL_META_H_
