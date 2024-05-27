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

// The file provides the IsStrictlyBaseOfAndConvertibleToSTLContainer type
// trait metafunction to assist in working with the _GLIBCXX_DEBUG debug
// wrappers of STL containers.
//
// DO NOT INCLUDE THIS FILE DIRECTLY. Use this file by including
// turbo/strings/str_split.h.
//
// IWYU pragma: private, include "turbo/strings/str_split.h"

#ifndef TURBO_STRINGS_INTERNAL_STL_TYPE_TRAITS_H_
#define TURBO_STRINGS_INTERNAL_STL_TYPE_TRAITS_H_

#include <array>
#include <bitset>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <turbo/meta/type_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

template <typename C, template <typename...> class T>
struct IsSpecializationImpl : std::false_type {};
template <template <typename...> class T, typename... Args>
struct IsSpecializationImpl<T<Args...>, T> : std::true_type {};
template <typename C, template <typename...> class T>
using IsSpecialization = IsSpecializationImpl<turbo::decay_t<C>, T>;

template <typename C>
struct IsArrayImpl : std::false_type {};
template <template <typename, size_t> class A, typename T, size_t N>
struct IsArrayImpl<A<T, N>> : std::is_same<A<T, N>, std::array<T, N>> {};
template <typename C>
using IsArray = IsArrayImpl<turbo::decay_t<C>>;

template <typename C>
struct IsBitsetImpl : std::false_type {};
template <template <size_t> class B, size_t N>
struct IsBitsetImpl<B<N>> : std::is_same<B<N>, std::bitset<N>> {};
template <typename C>
using IsBitset = IsBitsetImpl<turbo::decay_t<C>>;

template <typename C>
struct IsSTLContainer
    : turbo::disjunction<
          IsArray<C>, IsBitset<C>, IsSpecialization<C, std::deque>,
          IsSpecialization<C, std::forward_list>,
          IsSpecialization<C, std::list>, IsSpecialization<C, std::map>,
          IsSpecialization<C, std::multimap>, IsSpecialization<C, std::set>,
          IsSpecialization<C, std::multiset>,
          IsSpecialization<C, std::unordered_map>,
          IsSpecialization<C, std::unordered_multimap>,
          IsSpecialization<C, std::unordered_set>,
          IsSpecialization<C, std::unordered_multiset>,
          IsSpecialization<C, std::vector>> {};

template <typename C, template <typename...> class T, typename = void>
struct IsBaseOfSpecializationImpl : std::false_type {};
// IsBaseOfSpecializationImpl needs multiple partial specializations to SFINAE
// on the existence of container dependent types and plug them into the STL
// template.
template <typename C, template <typename, typename> class T>
struct IsBaseOfSpecializationImpl<
    C, T, turbo::void_t<typename C::value_type, typename C::allocator_type>>
    : std::is_base_of<C,
                      T<typename C::value_type, typename C::allocator_type>> {};
template <typename C, template <typename, typename, typename> class T>
struct IsBaseOfSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::key_compare,
                 typename C::allocator_type>>
    : std::is_base_of<C, T<typename C::key_type, typename C::key_compare,
                           typename C::allocator_type>> {};
template <typename C, template <typename, typename, typename, typename> class T>
struct IsBaseOfSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::mapped_type,
                 typename C::key_compare, typename C::allocator_type>>
    : std::is_base_of<C,
                      T<typename C::key_type, typename C::mapped_type,
                        typename C::key_compare, typename C::allocator_type>> {
};
template <typename C, template <typename, typename, typename, typename> class T>
struct IsBaseOfSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::hasher,
                 typename C::key_equal, typename C::allocator_type>>
    : std::is_base_of<C, T<typename C::key_type, typename C::hasher,
                           typename C::key_equal, typename C::allocator_type>> {
};
template <typename C,
          template <typename, typename, typename, typename, typename> class T>
struct IsBaseOfSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::mapped_type,
                 typename C::hasher, typename C::key_equal,
                 typename C::allocator_type>>
    : std::is_base_of<C, T<typename C::key_type, typename C::mapped_type,
                           typename C::hasher, typename C::key_equal,
                           typename C::allocator_type>> {};
template <typename C, template <typename...> class T>
using IsBaseOfSpecialization = IsBaseOfSpecializationImpl<turbo::decay_t<C>, T>;

template <typename C>
struct IsBaseOfArrayImpl : std::false_type {};
template <template <typename, size_t> class A, typename T, size_t N>
struct IsBaseOfArrayImpl<A<T, N>> : std::is_base_of<A<T, N>, std::array<T, N>> {
};
template <typename C>
using IsBaseOfArray = IsBaseOfArrayImpl<turbo::decay_t<C>>;

template <typename C>
struct IsBaseOfBitsetImpl : std::false_type {};
template <template <size_t> class B, size_t N>
struct IsBaseOfBitsetImpl<B<N>> : std::is_base_of<B<N>, std::bitset<N>> {};
template <typename C>
using IsBaseOfBitset = IsBaseOfBitsetImpl<turbo::decay_t<C>>;

template <typename C>
struct IsBaseOfSTLContainer
    : turbo::disjunction<IsBaseOfArray<C>, IsBaseOfBitset<C>,
                        IsBaseOfSpecialization<C, std::deque>,
                        IsBaseOfSpecialization<C, std::forward_list>,
                        IsBaseOfSpecialization<C, std::list>,
                        IsBaseOfSpecialization<C, std::map>,
                        IsBaseOfSpecialization<C, std::multimap>,
                        IsBaseOfSpecialization<C, std::set>,
                        IsBaseOfSpecialization<C, std::multiset>,
                        IsBaseOfSpecialization<C, std::unordered_map>,
                        IsBaseOfSpecialization<C, std::unordered_multimap>,
                        IsBaseOfSpecialization<C, std::unordered_set>,
                        IsBaseOfSpecialization<C, std::unordered_multiset>,
                        IsBaseOfSpecialization<C, std::vector>> {};

template <typename C, template <typename...> class T, typename = void>
struct IsConvertibleToSpecializationImpl : std::false_type {};
// IsConvertibleToSpecializationImpl needs multiple partial specializations to
// SFINAE on the existence of container dependent types and plug them into the
// STL template.
template <typename C, template <typename, typename> class T>
struct IsConvertibleToSpecializationImpl<
    C, T, turbo::void_t<typename C::value_type, typename C::allocator_type>>
    : std::is_convertible<
          C, T<typename C::value_type, typename C::allocator_type>> {};
template <typename C, template <typename, typename, typename> class T>
struct IsConvertibleToSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::key_compare,
                 typename C::allocator_type>>
    : std::is_convertible<C, T<typename C::key_type, typename C::key_compare,
                               typename C::allocator_type>> {};
template <typename C, template <typename, typename, typename, typename> class T>
struct IsConvertibleToSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::mapped_type,
                 typename C::key_compare, typename C::allocator_type>>
    : std::is_convertible<
          C, T<typename C::key_type, typename C::mapped_type,
               typename C::key_compare, typename C::allocator_type>> {};
template <typename C, template <typename, typename, typename, typename> class T>
struct IsConvertibleToSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::hasher,
                 typename C::key_equal, typename C::allocator_type>>
    : std::is_convertible<
          C, T<typename C::key_type, typename C::hasher, typename C::key_equal,
               typename C::allocator_type>> {};
template <typename C,
          template <typename, typename, typename, typename, typename> class T>
struct IsConvertibleToSpecializationImpl<
    C, T,
    turbo::void_t<typename C::key_type, typename C::mapped_type,
                 typename C::hasher, typename C::key_equal,
                 typename C::allocator_type>>
    : std::is_convertible<C, T<typename C::key_type, typename C::mapped_type,
                               typename C::hasher, typename C::key_equal,
                               typename C::allocator_type>> {};
template <typename C, template <typename...> class T>
using IsConvertibleToSpecialization =
    IsConvertibleToSpecializationImpl<turbo::decay_t<C>, T>;

template <typename C>
struct IsConvertibleToArrayImpl : std::false_type {};
template <template <typename, size_t> class A, typename T, size_t N>
struct IsConvertibleToArrayImpl<A<T, N>>
    : std::is_convertible<A<T, N>, std::array<T, N>> {};
template <typename C>
using IsConvertibleToArray = IsConvertibleToArrayImpl<turbo::decay_t<C>>;

template <typename C>
struct IsConvertibleToBitsetImpl : std::false_type {};
template <template <size_t> class B, size_t N>
struct IsConvertibleToBitsetImpl<B<N>>
    : std::is_convertible<B<N>, std::bitset<N>> {};
template <typename C>
using IsConvertibleToBitset = IsConvertibleToBitsetImpl<turbo::decay_t<C>>;

template <typename C>
struct IsConvertibleToSTLContainer
    : turbo::disjunction<
          IsConvertibleToArray<C>, IsConvertibleToBitset<C>,
          IsConvertibleToSpecialization<C, std::deque>,
          IsConvertibleToSpecialization<C, std::forward_list>,
          IsConvertibleToSpecialization<C, std::list>,
          IsConvertibleToSpecialization<C, std::map>,
          IsConvertibleToSpecialization<C, std::multimap>,
          IsConvertibleToSpecialization<C, std::set>,
          IsConvertibleToSpecialization<C, std::multiset>,
          IsConvertibleToSpecialization<C, std::unordered_map>,
          IsConvertibleToSpecialization<C, std::unordered_multimap>,
          IsConvertibleToSpecialization<C, std::unordered_set>,
          IsConvertibleToSpecialization<C, std::unordered_multiset>,
          IsConvertibleToSpecialization<C, std::vector>> {};

template <typename C>
struct IsStrictlyBaseOfAndConvertibleToSTLContainer
    : turbo::conjunction<turbo::negation<IsSTLContainer<C>>,
                        IsBaseOfSTLContainer<C>,
                        IsConvertibleToSTLContainer<C>> {};

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // TURBO_STRINGS_INTERNAL_STL_TYPE_TRAITS_H_
