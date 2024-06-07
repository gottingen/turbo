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
// This header file contains C++14 versions of standard <utility> header
// abstractions available within C++17, and are designed to be drop-in
// replacement for code compliant with C++14 and C++17.
//
// The following abstractions are defined:
//
//   * apply<Functor, Tuple>         == std::apply<Functor, Tuple>
//   * exchange<T>                   == std::exchange<T>
//   * make_from_tuple<T>            == std::make_from_tuple<T>
//
// This header file also provides the tag types `in_place_t`, `in_place_type_t`,
// and `in_place_index_t`, as well as the constant `in_place`, and
// `constexpr` `std::move()` and `std::forward()` implementations in C++11.
//
// References:
//
//  https://en.cppreference.com/w/cpp/utility/apply
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3658.html

#ifndef TURBO_UTILITY_UTILITY_H_
#define TURBO_UTILITY_UTILITY_H_

#include <cstddef>
#include <cstdlib>
#include <tuple>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/base/internal/inline_variable.h>
#include <turbo/base/internal/invoke.h>
#include <turbo/meta/type_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Historical note: Turbo once provided implementations of these
// abstractions for platforms that had not yet provided them. Those
// platforms are no longer supported. New code should simply use the
// the ones from std directly.
using std::exchange;
using std::forward;
using std::index_sequence;
using std::index_sequence_for;
using std::integer_sequence;
using std::make_index_sequence;
using std::make_integer_sequence;
using std::move;

namespace utility_internal {

template <typename T>
struct InPlaceTypeTag {
  explicit InPlaceTypeTag() = delete;
  InPlaceTypeTag(const InPlaceTypeTag&) = delete;
  InPlaceTypeTag& operator=(const InPlaceTypeTag&) = delete;
};

template <size_t I>
struct InPlaceIndexTag {
  explicit InPlaceIndexTag() = delete;
  InPlaceIndexTag(const InPlaceIndexTag&) = delete;
  InPlaceIndexTag& operator=(const InPlaceIndexTag&) = delete;
};

}  // namespace utility_internal

// Tag types

#ifdef TURBO_USES_STD_OPTIONAL

using std::in_place_t;
using std::in_place;

#else  // TURBO_USES_STD_OPTIONAL

// in_place_t
//
// Tag type used to specify in-place construction, such as with
// `turbo::optional`, designed to be a drop-in replacement for C++17's
// `std::in_place_t`.
struct in_place_t {};

TURBO_INTERNAL_INLINE_CONSTEXPR(in_place_t, in_place, {});

#endif  // TURBO_USES_STD_OPTIONAL

#if defined(TURBO_USES_STD_ANY) || defined(TURBO_USES_STD_VARIANT)
using std::in_place_type;
using std::in_place_type_t;
#else

// in_place_type_t
//
// Tag type used for in-place construction when the type to construct needs to
// be specified, such as with `turbo::any`, designed to be a drop-in replacement
// for C++17's `std::in_place_type_t`.
template <typename T>
using in_place_type_t = void (*)(utility_internal::InPlaceTypeTag<T>);

template <typename T>
void in_place_type(utility_internal::InPlaceTypeTag<T>) {}
#endif  // TURBO_USES_STD_ANY || TURBO_USES_STD_VARIANT

#ifdef TURBO_USES_STD_VARIANT
using std::in_place_index;
using std::in_place_index_t;
#else

// in_place_index_t
//
// Tag type used for in-place construction when the type to construct needs to
// be specified, such as with `turbo::any`, designed to be a drop-in replacement
// for C++17's `std::in_place_index_t`.
template <size_t I>
using in_place_index_t = void (*)(utility_internal::InPlaceIndexTag<I>);

template <size_t I>
void in_place_index(utility_internal::InPlaceIndexTag<I>) {}
#endif  // TURBO_USES_STD_VARIANT

namespace utility_internal {
// Helper method for expanding tuple into a called method.
template <typename Functor, typename Tuple, std::size_t... Indexes>
auto apply_helper(Functor&& functor, Tuple&& t, index_sequence<Indexes...>)
    -> decltype(turbo::base_internal::invoke(
        turbo::forward<Functor>(functor),
        std::get<Indexes>(turbo::forward<Tuple>(t))...)) {
  return turbo::base_internal::invoke(
      turbo::forward<Functor>(functor),
      std::get<Indexes>(turbo::forward<Tuple>(t))...);
}

}  // namespace utility_internal

// apply
//
// Invokes a Callable using elements of a tuple as its arguments.
// Each element of the tuple corresponds to an argument of the call (in order).
// Both the Callable argument and the tuple argument are perfect-forwarded.
// For member-function Callables, the first tuple element acts as the `this`
// pointer. `turbo::apply` is designed to be a drop-in replacement for C++17's
// `std::apply`. Unlike C++17's `std::apply`, this is not currently `constexpr`.
//
// Example:
//
//   class Foo {
//    public:
//     void Bar(int);
//   };
//   void user_function1(int, std::string);
//   void user_function2(std::unique_ptr<Foo>);
//   auto user_lambda = [](int, int) {};
//
//   int main()
//   {
//       std::tuple<int, std::string> tuple1(42, "bar");
//       // Invokes the first user function on int, std::string.
//       turbo::apply(&user_function1, tuple1);
//
//       std::tuple<std::unique_ptr<Foo>> tuple2(turbo::make_unique<Foo>());
//       // Invokes the user function that takes ownership of the unique
//       // pointer.
//       turbo::apply(&user_function2, std::move(tuple2));
//
//       auto foo = turbo::make_unique<Foo>();
//       std::tuple<Foo*, int> tuple3(foo.get(), 42);
//       // Invokes the method Bar on foo with one argument, 42.
//       turbo::apply(&Foo::Bar, tuple3);
//
//       std::tuple<int, int> tuple4(8, 9);
//       // Invokes a lambda.
//       turbo::apply(user_lambda, tuple4);
//   }
template <typename Functor, typename Tuple>
auto apply(Functor&& functor, Tuple&& t)
    -> decltype(utility_internal::apply_helper(
        turbo::forward<Functor>(functor), turbo::forward<Tuple>(t),
        turbo::make_index_sequence<std::tuple_size<
            typename std::remove_reference<Tuple>::type>::value>{})) {
  return utility_internal::apply_helper(
      turbo::forward<Functor>(functor), turbo::forward<Tuple>(t),
      turbo::make_index_sequence<std::tuple_size<
          typename std::remove_reference<Tuple>::type>::value>{});
}

namespace utility_internal {
template <typename T, typename Tuple, size_t... I>
T make_from_tuple_impl(Tuple&& tup, turbo::index_sequence<I...>) {
  return T(std::get<I>(std::forward<Tuple>(tup))...);
}
}  // namespace utility_internal

// make_from_tuple
//
// Given the template parameter type `T` and a tuple of arguments
// `std::tuple(arg0, arg1, ..., argN)` constructs an object of type `T` as if by
// calling `T(arg0, arg1, ..., argN)`.
//
// Example:
//
//   std::tuple<const char*, size_t> args("hello world", 5);
//   auto s = turbo::make_from_tuple<std::string>(args);
//   assert(s == "hello");
//
template <typename T, typename Tuple>
constexpr T make_from_tuple(Tuple&& tup) {
  return utility_internal::make_from_tuple_impl<T>(
      std::forward<Tuple>(tup),
      turbo::make_index_sequence<
          std::tuple_size<turbo::decay_t<Tuple>>::value>{});
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_UTILITY_UTILITY_H_
