//
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
// -----------------------------------------------------------------------------
// type_traits.h
// -----------------------------------------------------------------------------
//
// This file contains C++11-compatible versions of standard <type_traits> API
// functions for determining the characteristics of types. Such traits can
// support type inference, classification, and transformation, as well as
// make it easier to write templates based on generic type behavior.
//
// See https://en.cppreference.com/w/cpp/header/type_traits
//
// WARNING: use of many of the constructs in this header will count as "complex
// template metaprogramming", so before proceeding, please carefully consider
// https://google.github.io/styleguide/cppguide.html#Template_metaprogramming
//
// WARNING: using template metaprogramming to detect or depend on API
// features is brittle and not guaranteed. Neither the standard library nor
// Turbo provides any guarantee that APIs are stable in the face of template
// metaprogramming. Use with caution.
#ifndef TURBO_META_TYPE_TRAITS_H_
#define TURBO_META_TYPE_TRAITS_H_

#include <cstddef>
#include <functional>
#include <type_traits>
#include <string>
#include <complex>
#include <chrono>
#include "turbo/platform/port.h"

// MSVC constructibility traits do not detect destructor properties and so our
// implementations should not use them as a source-of-truth.
#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#define TURBO_META_INTERNAL_STD_CONSTRUCTION_TRAITS_DONT_CHECK_DESTRUCTION 1
#endif

// Defines the default alignment. `__STDCPP_DEFAULT_NEW_ALIGNMENT__` is a C++17
// feature.
#if defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
#define TURBO_INTERNAL_DEFAULT_NEW_ALIGNMENT __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else  // defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
#define TURBO_INTERNAL_DEFAULT_NEW_ALIGNMENT alignof(std::max_align_t)
#endif  // defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    namespace type_traits_internal {
        template<typename... Ts>
        struct VoidTImpl {
            using type = void;
        };

        ////////////////////////////////
        // Library Fundamentals V2 TS //
        ////////////////////////////////

        // NOTE: The `is_detected` family of templates here differ from the library
        // fundamentals specification in that for library fundamentals, `Op<Args...>` is
        // evaluated as soon as the type `is_detected<Op, Args...>` undergoes
        // substitution, regardless of whether or not the `::value` is accessed. That
        // is inconsistent with all other standard traits and prevents lazy evaluation
        // in larger contexts (such as if the `is_detected` check is a trailing argument
        // of a `conjunction`. This implementation opts to instead be lazy in the same
        // way that the standard traits are (this "defect" of the detection idiom
        // specifications has been reported).

        template<class Enabler, template<class...> class Op, class... Args>
        struct is_detected_impl {
            using type = std::false_type;
        };

        template<template<class...> class Op, class... Args>
        struct is_detected_impl<typename VoidTImpl<Op<Args...>>::type, Op, Args...> {
            using type = std::true_type;
        };

        template<template<class...> class Op, class... Args>
        struct is_detected : is_detected_impl<void, Op, Args...>::type {
        };

        template<class Enabler, class To, template<class...> class Op, class... Args>
        struct is_detected_convertible_impl {
            using type = std::false_type;
        };

        template<class To, template<class...> class Op, class... Args>
        struct is_detected_convertible_impl<
                typename std::enable_if<std::is_convertible<Op<Args...>, To>::value>::type,
                To, Op, Args...> {
            using type = std::true_type;
        };

        template<class To, template<class...> class Op, class... Args>
        struct is_detected_convertible
                : is_detected_convertible_impl<void, To, Op, Args...>::type {
        };

    }  // namespace type_traits_internal

    // void_t()
    //
    // Ignores the type of any its arguments and returns `void`. In general, this
    // metafunction allows you to create a general case that maps to `void` while
    // allowing specializations that map to specific types.
    //
    // This metafunction is designed to be a drop-in replacement for the C++17
    // `std::void_t` metafunction.
    //
    // NOTE: `turbo::void_t` does not use the standard-specified implementation so
    // that it can remain compatible with gcc < 5.1. This can introduce slightly
    // different behavior, such as when ordering partial specializations.
    template<typename... Ts>
    using void_t = typename type_traits_internal::VoidTImpl<Ts...>::type;

    // negation
    //
    // Performs a compile-time logical NOT operation on the passed type (which
    // must have  `::value` members convertible to `bool`.
    //
    // This metafunction is designed to be a drop-in replacement for the C++17
    // `std::negation` metafunction.
    template<typename T>
    struct negation : std::integral_constant<bool, !T::value> {
    };

    // is_function()
    //
    // Determines whether the passed type `T` is a function type.
    //
    // This metafunction is designed to be a drop-in replacement for the C++11
    // `std::is_function()` metafunction for platforms that have incomplete C++11
    // support (such as libstdc++ 4.x).
    //
    // This metafunction works because appending `const` to a type does nothing to
    // function types and reference types (and forms a const-qualified type
    // otherwise).
    template<typename T>
    struct is_function
            : std::integral_constant<
                    bool, !(std::is_reference<T>::value ||
                            std::is_const<typename std::add_const<T>::type>::value)> {
    };


#if defined(__cpp_lib_remove_cvref) && __cpp_lib_remove_cvref >= 201711L
    template <typename T>
    using remove_cvref = std::remove_cvref<T>;

    template <typename T>
    using remove_cvref_t = typename std::remove_cvref<T>::type;
#else
    // remove_cvref()
    //
    // C++11 compatible implementation of std::remove_cvref which was added in
    // C++20.
    template<typename T>
    struct remove_cvref {
        using type =
                typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    };

    template<typename T>
    using remove_cvref_t = typename remove_cvref<T>::type;
#endif

    namespace type_traits_internal {
        // This trick to retrieve a default alignment is necessary for our
        // implementation of aligned_storage_t to be consistent with any
        // implementation of std::aligned_storage.
        template<size_t Len, typename T = std::aligned_storage<Len>>
        struct default_alignment_of_aligned_storage;

        template<size_t Len, size_t Align>
        struct default_alignment_of_aligned_storage<
                Len, std::aligned_storage<Len, Align>> {
            static constexpr size_t value = Align;
        };
    }  // namespace type_traits_internal

    // TODO(b/260219225): std::aligned_storage(_t) is deprecated in C++23.
    template<size_t Len, size_t Align = type_traits_internal::
    default_alignment_of_aligned_storage<Len>::value>
    using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

    namespace type_traits_internal {

#if (defined(__cpp_lib_is_invocable) && __cpp_lib_is_invocable >= 201703L) || \
    (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
// std::result_of is deprecated (C++17) or removed (C++20)
        template<typename>
        struct result_of;
        template<typename F, typename... Args>
        struct result_of<F(Args...)> : std::invoke_result<F, Args...> {
        };
#else
        template<typename F> using result_of = std::result_of<F>;
#endif

    }  // namespace type_traits_internal

    template<typename F>
    using result_of_t = typename type_traits_internal::result_of<F>::type;

    namespace type_traits_internal {
// In MSVC we can't probe std::hash or stdext::hash because it triggers a
// static_assert instead of failing substitution. Libc++ prior to 4.0
// also used a static_assert.
//
#if defined(_MSC_VER) || (defined(_LIBCPP_VERSION) && \
                          _LIBCPP_VERSION < 4000 && _LIBCPP_STD_VER > 11)
#define TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 0
#else
#define TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_ 1
#endif

#if !TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
        template <typename Key, typename = size_t>
        struct IsHashable : std::true_type {};
#else   // TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
        template<typename Key, typename = void>
        struct IsHashable : std::false_type {
        };

        template<typename Key>
        struct IsHashable<
                Key,
                std::enable_if_t<std::is_convertible<
                        decltype(std::declval<std::hash<Key> &>()(std::declval<Key const &>())),
                        std::size_t>::value>> : std::true_type {
        };
#endif  // !TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_

        struct AssertHashEnabledHelper {
        private:
            static void Sink(...) {}

            struct NAT {
            };

            template<class Key>
            static auto GetReturnType(int)
            -> decltype(std::declval<std::hash<Key>>()(std::declval<Key const &>()));

            template<class Key>
            static NAT GetReturnType(...);

            template<class Key>
            static std::nullptr_t DoIt() {
                static_assert(IsHashable<Key>::value,
                              "std::hash<Key> does not provide a call operator");
                static_assert(
                        std::is_default_constructible<std::hash<Key>>::value,
                        "std::hash<Key> must be default constructible when it is enabled");
                static_assert(
                        std::is_copy_constructible<std::hash<Key>>::value,
                        "std::hash<Key> must be copy constructible when it is enabled");
                static_assert(std::is_copy_assignable<std::hash<Key>>::value,
                              "std::hash<Key> must be copy assignable when it is enabled");
                // is_destructible is unchecked as it's implied by each of the
                // is_constructible checks.
                using ReturnType = decltype(GetReturnType<Key>(0));
                static_assert(std::is_same<ReturnType, NAT>::value ||
                              std::is_same<ReturnType, size_t>::value,
                              "std::hash<Key> must return size_t");
                return nullptr;
            }

            template<class... Ts>
            friend void AssertHashEnabled();
        };

        template<class... Ts>
        inline void AssertHashEnabled() {
            using Helper = AssertHashEnabledHelper;
            Helper::Sink(Helper::DoIt<Ts>()...);
        }

    }  // namespace type_traits_internal

    // An internal namespace that is required to implement the C++17 swap traits.
    // It is not further nested in type_traits_internal to avoid long symbol names.
    namespace swap_internal {

        // Necessary for the traits.
        using std::swap;

        // This declaration prevents global `swap` and `turbo::swap` overloads from being
        // considered unless ADL picks them up.
        void swap();

        template<class T>
        using IsSwappableImpl = decltype(swap(std::declval<T &>(), std::declval<T &>()));

        // NOTE: This dance with the default template parameter is for MSVC.
        template<class T,
                class IsNoexcept = std::integral_constant<
                        bool, noexcept(swap(std::declval<T &>(), std::declval<T &>()))>>
        using IsNothrowSwappableImpl = typename std::enable_if<IsNoexcept::value>::type;

        // IsSwappable
        //
        // Determines whether the standard swap idiom is a valid expression for
        // arguments of type `T`.
        template<class T>
        struct IsSwappable
                : turbo::type_traits_internal::is_detected<IsSwappableImpl, T> {
        };

        // IsNothrowSwappable
        //
        // Determines whether the standard swap idiom is a valid expression for
        // arguments of type `T` and is noexcept.
        template<class T>
        struct IsNothrowSwappable
                : turbo::type_traits_internal::is_detected<IsNothrowSwappableImpl, T> {
        };

        // Swap()
        //
        // Performs the swap idiom from a namespace where valid candidates may only be
        // found in `std` or via ADL.
        template<class T, std::enable_if_t<IsSwappable<T>::value, int> = 0>
        void Swap(T &lhs, T &rhs) noexcept(IsNothrowSwappable<T>::value) {
            swap(lhs, rhs);
        }

        // StdSwapIsUnconstrained
        //
        // Some standard library implementations are broken in that they do not
        // constrain `std::swap`. This will effectively tell us if we are dealing with
        // one of those implementations.
        using StdSwapIsUnconstrained = IsSwappable<void()>;

    }  // namespace swap_internal

    namespace type_traits_internal {

        // Make the swap-related traits/function accessible from this namespace.
        using swap_internal::IsNothrowSwappable;
        using swap_internal::IsSwappable;
        using swap_internal::Swap;
        using swap_internal::StdSwapIsUnconstrained;

    }  // namespace type_traits_internal

// turbo::is_trivially_relocatable<T>
// Detects whether a type is "trivially relocatable" -- meaning it can be
// relocated without invoking the constructor/destructor, using a form of move
// elision.
//
// Example:
//
// if constexpr (turbo::is_trivially_relocatable<T>::value) {
//   memcpy(new_location, old_location, sizeof(T));
// } else {
//   new(new_location) T(std::move(*old_location));
//   old_location->~T();
// }
//
// Upstream documentation:
//
// https://clang.llvm.org/docs/LanguageExtensions.html#:~:text=__is_trivially_relocatable
//
#if TURBO_HAVE_BUILTIN(__is_trivially_relocatable)
    template <class T>
    struct is_trivially_relocatable
        : std::integral_constant<bool, __is_trivially_relocatable(T)> {};
#else
    template<class T>
    struct is_trivially_relocatable : std::integral_constant<bool, false> {
    };
#endif

// turbo::is_constant_evaluated()
//
// Detects whether the function call occurs within a constant-evaluated context.
// Returns true if the evaluation of the call occurs within the evaluation of an
// expression or conversion that is manifestly constant-evaluated; otherwise
// returns false.
//
// This function is implemented in terms of `std::is_constant_evaluated` for
// c++20 and up. For older c++ versions, the function is implemented in terms
// of `__builtin_is_constant_evaluated` if available, otherwise the function
// will fail to compile.
//
// Applications can inspect `TURBO_HAVE_CONSTANT_EVALUATED` at compile time
// to check if this function is supported.
//
// Example:
//
// constexpr MyClass::MyClass(int param) {
// #ifdef TURBO_HAVE_CONSTANT_EVALUATED
//   if (!turbo::is_constant_evaluated()) {
//     TURBO_LOG(INFO) << "MyClass(" << param << ")";
//   }
// #endif  // TURBO_HAVE_CONSTANT_EVALUATED
// }
//
// Upstream documentation:
//
// http://en.cppreference.com/w/cpp/types/is_constant_evaluated
// http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#:~:text=__builtin_is_constant_evaluated
//
#if defined(TURBO_HAVE_CONSTANT_EVALUATED)
    constexpr bool is_constant_evaluated() noexcept {
#ifdef __cpp_lib_is_constant_evaluated
      return std::is_constant_evaluated();
#elif TURBO_HAVE_BUILTIN(__builtin_is_constant_evaluated)
      return __builtin_is_constant_evaluated();
#endif
    }
#endif  // TURBO_HAVE_CONSTANT_EVALUATED
    TURBO_NAMESPACE_END

#ifdef TURBO_COMPILER_HAVE_RTTI
#define TURBO_TYPE_INFO_OF(...) (&typeid(__VA_ARGS__))
#else
#define TURBO_TYPE_INFO_OF(...) \
  ((sizeof(__VA_ARGS__)), static_cast<std::type_info const*>(nullptr))
#endif

    //  type_info_of
    //
    //  Returns &typeid(T) if RTTI is available, nullptr otherwise.
    //
    //  This overload works on the static type of the template parameter.
    template<typename T>
    TURBO_FORCE_INLINE static std::type_info const *type_info_of() {
        return TURBO_TYPE_INFO_OF(T);
    }

    //  type_info_of
    //
    //  Returns &typeid(t) if RTTI is available, nullptr otherwise.
    //
    //  This overload works on the dynamic type of the non-template parameter.
    template<typename T>
    TURBO_FORCE_INLINE static std::type_info const *type_info_of(TURBO_MAYBE_UNUSED T const &t) {
        return TURBO_TYPE_INFO_OF(t);
    }

    template<typename T>
    struct is_string_type : public std::false_type {
    };
    template<>
    struct is_string_type<std::string> : public std::true_type {
    };

    /************************************
    * arithmetic type promotion traits *
    ************************************/

    /**
     * Traits class for the result type of mixed arithmetic expressions.
     * For example, <tt>promote_type<unsigned char, unsigned char>::type</tt> tells
     * the user that <tt>unsigned char + unsigned char => int</tt>.
     */
    template<class... T>
    struct promote_type;

    template<>
    struct promote_type<> {
        using type = void;
    };

    template<class T>
    struct promote_type<T> {
        using type = typename promote_type<T, T>::type;
    };

    template<class C, class D1, class D2>
    struct promote_type<std::chrono::time_point<C, D1>, std::chrono::time_point<C, D2>> {
        using type = std::chrono::time_point<C, typename promote_type<D1, D2>::type>;
    };

    template<class T0, class T1>
    struct promote_type<T0, T1> {
        using type = decltype(std::declval<std::decay_t<T0>>() + std::declval<std::decay_t<T1>>());
    };

    template<class T0, class... REST>
    struct promote_type<T0, REST...> {
        using type = decltype(std::declval<std::decay_t<T0>>() + std::declval<typename promote_type<REST...>::type>());
    };

    template<>
    struct promote_type<bool> {
        using type = bool;
    };

    template<class T>
    struct promote_type<bool, T> {
        using type = T;
    };

    template<class T>
    struct promote_type<bool, std::complex<T>> {
        using type = std::complex<T>;
    };

    template<class T1, class T2>
    struct promote_type<T1, std::complex<T2>> {
        using type = std::complex<typename promote_type<T1, T2>::type>;
    };

    template<class T1, class T2>
    struct promote_type<std::complex<T1>, T2>
            : promote_type<T2, std::complex<T1>> {
    };

    template<class T>
    struct promote_type<std::complex<T>, std::complex<T>> {
        using type = std::complex<T>;
    };

    template<class T1, class T2>
    struct promote_type<std::complex<T1>, std::complex<T2>> {
        using type = std::complex<typename promote_type<T1, T2>::type>;
    };

    template<class... REST>
    struct promote_type<bool, REST...> {
        using type = typename promote_type<bool, typename promote_type<REST...>::type>::type;
    };

    /**
     * Abbreviation of 'typename promote_type<T>::type'.
     */
    template<class... T>
    using promote_type_t = typename promote_type<T...>::type;

    /**
     * Traits class to find the biggest type of the same kind.
     *
     * For example, <tt>big_promote_type<unsigned char>::type</tt> is <tt>unsigned long long</tt>.
     * The default implementation only supports built-in types and <tt>std::complex</tt>. All
     * other types remain unchanged unless <tt>big_promote_type</tt> gets specialized for them.
     */
    template<class T>
    struct big_promote_type {
    private:

        using V = std::decay_t<T>;
        static constexpr bool is_arithmetic = std::is_arithmetic<V>::value;
        static constexpr bool is_signed = std::is_signed<V>::value;
        static constexpr bool is_integral = std::is_integral<V>::value;
        static constexpr bool is_long_double = std::is_same<V, long double>::value;

    public:

        using type = std::conditional_t<is_arithmetic,
                std::conditional_t<is_integral,
                        std::conditional_t<is_signed, long long, unsigned long long>,
                        std::conditional_t<is_long_double, long double, double>
                >,
                V
        >;
    };

    template<class T>
    struct big_promote_type<std::complex<T>> {
        using type = std::complex<typename big_promote_type<T>::type>;
    };

    /**
     * Abbreviation of 'typename big_promote_type<T>::type'.
     */
    template<class T>
    using big_promote_type_t = typename big_promote_type<T>::type;

    namespace traits_detail {
        using std::sqrt;

        template<class T>
        using real_promote_type_t = decltype(sqrt(std::declval<std::decay_t<T>>()));
    }

    /**
     * Result type of algebraic expressions.
     *
     * For example, <tt>real_promote_type<int>::type</tt> tells the
     * user that <tt>sqrt(int) => double</tt>.
     */
    template<class T>
    struct real_promote_type {
        using type = traits_detail::real_promote_type_t<T>;
    };

    /**
     * Abbreviation of 'typename real_promote_type<T>::type'.
     */
    template<class T>
    using real_promote_type_t = typename real_promote_type<T>::type;

    /**
     * Traits class to replace 'bool' with 'uint8_t' and keep everything else.
     *
     * This is useful for scientific computing, where a boolean mask array is
     * usually implemented as an array of bytes.
     */
    template<class T>
    struct bool_promote_type {
        using type = typename std::conditional<std::is_same<T, bool>::value, uint8_t, T>::type;
    };

    /**
     * Abbreviation for typename bool_promote_type<T>::type
     */
    template<class T>
    using bool_promote_type_t = typename bool_promote_type<T>::type;

    /************
     * apply_cv *
     ************/

    namespace detail {
        template<class T, class U, bool = std::is_const<std::remove_reference_t<T>>::value,
                bool = std::is_volatile<std::remove_reference_t<T>>::value>
        struct apply_cv_impl {
            using type = U;
        };

        template<class T, class U>
        struct apply_cv_impl<T, U, true, false> {
            using type = const U;
        };

        template<class T, class U>
        struct apply_cv_impl<T, U, false, true> {
            using type = volatile U;
        };

        template<class T, class U>
        struct apply_cv_impl<T, U, true, true> {
            using type = const volatile U;
        };

        template<class T, class U>
        struct apply_cv_impl<T &, U, false, false> {
            using type = U &;
        };

        template<class T, class U>
        struct apply_cv_impl<T &, U, true, false> {
            using type = const U &;
        };

        template<class T, class U>
        struct apply_cv_impl<T &, U, false, true> {
            using type = volatile U &;
        };

        template<class T, class U>
        struct apply_cv_impl<T &, U, true, true> {
            using type = const volatile U &;
        };
    }

    template<class T, class U>
    struct apply_cv {
        using type = typename detail::apply_cv_impl<T, U>::type;
    };

    template<class T, class U>
    using apply_cv_t = typename apply_cv<T, U>::type;



    /************
     * concepts *
     ************/

#if !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 5))

    template<class... C>
    constexpr bool turbo_require = std::conjunction<C...>::value;

    template<class... C>
    constexpr bool either = std::disjunction<C...>::value;

    template<class... C>
    constexpr bool disallow = std::negation<std::conjunction<C...>>::value;

    template<class... C>
    constexpr bool disallow_one = std::negation<std::disjunction<C...>>::value;

    template<class... C>
    using check_requires = std::enable_if_t<turbo_require<C...>, int>;

    template<class... C>
    using check_either = std::enable_if_t<either<C...>, int>;

    template<class... C>
    using check_disallow = std::enable_if_t<disallow<C...>, int>;

    template<class... C>
    using check_disallow_one = std::enable_if_t<disallow_one<C...>, int>;

#else

    template <class... C>
    using check_requires = std::enable_if_t<conjunction<C...>::value, int>;

    template <class... C>
    using check_either = std::enable_if_t<std::disjunction<C...>::value, int>;

    template <class... C>
    using check_disallow = std::enable_if_t<turbo::negation<std::conjunction<C...>>::value, int>;

    template <class... C>
    using check_disallow_one = std::enable_if_t<turbo::negation<std::disjunction<C...>>::value, int>;

#endif

#define TURBO_REQUIRES_IMPL(...) turbo::check_requires<__VA_ARGS__>
#define TURBO_REQUIRES(...) TURBO_REQUIRES_IMPL(__VA_ARGS__) = 0

#define TURBO_EITHER_IMPL(...) turbo::check_either<__VA_ARGS__>
#define TURBO_EITHER(...) TURBO_EITHER_IMPL(__VA_ARGS__) = 0

#define TURBO_DISALLOW_IMPL(...) turbo::check_disallow<__VA_ARGS__>
#define TURBO_DISALLOW(...) TURBO_DISALLOW_IMPL(__VA_ARGS__) = 0

#define TURBO_DISALLOW_ONE_IMPL(...) turbo::check_disallow_one<__VA_ARGS__>
#define TURBO_DISALLOW_ONE(...) TURBO_DISALLOW_ONE_IMPL(__VA_ARGS__) = 0

    // For backward compatibility
    template<class... C>
    using check_concept = check_requires<C...>;

    /**************
     * all_scalar *
     **************/

    template<class... Args>
    struct all_scalar : std::conjunction<std::is_scalar<Args>...> {
    };

    struct identity {
        template<class T>
        T &&operator()(T &&x) const {
            return std::forward<T>(x);
        }
    };

    /*************************
     * select implementation *
     *************************/

    template<class B, class T1, class T2, TURBO_REQUIRES(all_scalar < B, T1, T2 >)>
    inline std::common_type_t<T1, T2> select(const B &cond, const T1 &v1, const T2 &v2) noexcept {
        return cond ? v1 : v2;
    }
    // to avoid useless casts (see https://github.com/nlohmann/json/issues/2893#issuecomment-889152324)
    template<typename T, typename U, std::enable_if_t<!std::is_same<T, U>::value, int> = 0>
    constexpr T conditional_static_cast(U value) {
        return static_cast<T>(value);
    }

    template<typename T, typename U, std::enable_if_t<std::is_same<T, U>::value, int> = 0>
    constexpr T conditional_static_cast(U value) {
        return value;
    }
}  // namespace turbo


//////
// TURBO_DECLVAL(T)
//
// This macro works like std::declval<T>() but does the same thing in a way
// that does not require instantiating a function template.
//
// Use this macro instead of std::declval<T>() in places that are widely
// instantiated to reduce compile-time overhead of instantiating function
// templates.
//
// Note that, like std::declval<T>(), this macro can only be used in
// unevaluated contexts.
//
// There are some small differences between this macro and std::declval<T>().
// - This macro results in a value of type 'T' instead of 'T&&'.
// - This macro requires the type T to be a complete type at the
// point of use.
// If this is a problem then use TURBO_DECLVAL(T&&) instead, or if T might
// be 'void', then use TURBO_DECLVAL(std::add_rvalue_reference_t<T>).
//
#define TURBO_DECLVAL(...) static_cast<__VA_ARGS__ (*)() noexcept>(nullptr)()

#endif  // TURBO_META_TYPE_TRAITS_H_
