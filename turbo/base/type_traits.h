
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_TYPE_TRAITS_H_
#define TURBO_BASE_TYPE_TRAITS_H_

#include <cstddef>  // For size_t.
#include <type_traits>

namespace turbo {

    template<typename T>
    struct remove_volatile;
    template<typename T>
    struct remove_reference;
    template<typename T>
    struct remove_const_reference;
    template<typename T>
    struct add_volatile;
    template<typename T>
    struct add_cv;
    template<typename T>
    struct add_reference;
    template<typename T>
    struct add_const_reference;
    template<typename T>
    struct remove_pointer;
    template<typename T>
    struct add_cr_non_integral;


    template<typename T>
    struct remove_cvref {
        typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
    };


    template<typename T>
    struct remove_volatile {
        typedef T type;
    };
    template<typename T>
    struct remove_volatile<T volatile> {
        typedef T type;
    };
    template<typename T>
    struct remove_cv {
        typedef typename std::remove_const<typename remove_volatile<T>::type>::type type;
    };

    // Specified by TR1 [4.7.2] Reference modifications.
    template<typename T>
    struct remove_reference {
        typedef T type;
    };
    template<typename T>
    struct remove_reference<T &> {
        typedef T type;
    };

    template<typename T>
    struct add_reference {
        typedef T &type;
    };
    template<typename T>
    struct add_reference<T &> {
        typedef T &type;
    };
    // Specializations for void which can't be referenced.
    template<>
    struct add_reference<void> {
        typedef void type;
    };
    template<>
    struct add_reference<void const> {
        typedef void const type;
    };
    template<>
    struct add_reference<void volatile> {
        typedef void volatile type;
    };
    template<>
    struct add_reference<void const volatile> {
        typedef void const volatile type;
    };

    // Shortcut for adding/removing const&
    template<typename T>
    struct add_const_reference {
        typedef typename add_reference<typename std::add_const<T>::type>::type type;
    };
    template<typename T>
    struct remove_const_reference {
        typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
    };

    // Add const& for non-integral types.
    // add_cr_non_integral<int>::type      -> int
    // add_cr_non_integral<FooClass>::type -> const FooClass&
    template<typename T>
    struct add_cr_non_integral {
        typedef typename std::conditional<std::is_integral<T>::value, T,
                typename add_reference<typename std::add_const<T>::type>::type>::type type;
    };


    // has_mapped_type<T>::value is true iff there exists a type T::mapped_type.
    template<typename T, typename = void>
    struct has_mapped_type : std::false_type {
    };
    template<typename T>
    struct has_mapped_type<T, std::void_t<typename T::mapped_type>>
            : std::true_type {
    };

    // has_value_type<T>::value is true iff there exists a type T::value_type.
    template<typename T, typename = void>
    struct has_value_type : std::false_type {
    };
    template<typename T>
    struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {
    };

    // has_const_iterator<T>::value is true iff there exists a type T::const_iterator.
    template<typename T, typename = void>
    struct has_const_iterator : std::false_type {
    };
    template<typename T>
    struct has_const_iterator<T, std::void_t<typename T::const_iterator>>
            : std::true_type {
    };


}  // namespace turbo


#endif  // TURBO_BASE_TYPE_TRAITS_H_
