
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: str_join.h
// -----------------------------------------------------------------------------
//
// This header file contains functions for joining a range of elements and
// returning the result as a std::string. string_join operations are specified by
// passing a range, a separator string to use between the elements joined, and
// an optional Formatter responsible for converting each argument in the range
// to a string. If omitted, a default `alpha_num_formatter()` is called on the
// elements to be joined, using the same formatting that `flare::string_cat()` uses.
// This package defines a number of default formatters, and you can define your
// own implementations.
//
// Ranges are specified by passing a container with `std::begin()` and
// `std::end()` iterators, container-specific `begin()` and `end()` iterators, a
// brace-initialized `std::initializer_list`, or a `std::tuple` of heterogeneous
// objects. The separator string is specified as an `std::string_view`.
//
// Because the default formatter uses the `flare::alpha_num` class,
// `flare::string_join()`, like `flare::string_cat()`, will work out-of-the-box on
// collections of strings, ints, floats, doubles, etc.
//
// Example:
//
//   std::vector<std::string> v = {"foo", "bar", "baz"};
//   std::string s = flare::string_join(v, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// See comments on the `flare::string_join()` function for more examples.

#ifndef FLARE_STRINGS_STR_JOIN_H_
#define FLARE_STRINGS_STR_JOIN_H_

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "flare/base/profile.h"
#include "flare/strings/internal/str_join_internal.h"
#include <string_view>

namespace flare {


// -----------------------------------------------------------------------------
// Concept: Formatter
// -----------------------------------------------------------------------------
//
// A Formatter is a function object that is responsible for formatting its
// argument as a string and appending it to a given output std::string.
// Formatters may be implemented as function objects, lambdas, or normal
// functions. You may provide your own Formatter to enable `flare::string_join()` to
// work with arbitrary types.
//
// The following is an example of a custom Formatter that simply uses
// `std::to_string()` to format an integer as a std::string.
//
//   struct MyFormatter {
//     void operator()(std::string* out, int i) const {
//       out->append(std::to_string(i));
//     }
//   };
//
// You would use the above formatter by passing an instance of it as the final
// argument to `flare::string_join()`:
//
//   std::vector<int> v = {1, 2, 3, 4};
//   std::string s = flare::string_join(v, "-", MyFormatter());
//   EXPECT_EQ("1-2-3-4", s);
//
// The following standard formatters are provided within this file:
//
// - `alpha_num_formatter()` (the default)
// - `stream_formatter()`
// - `pair_formatter()`
// - `dereference_formatter()`

// alpha_num_formatter()
//
// Default formatter used if none is specified. Uses `flare::alpha_num` to convert
// numeric arguments to strings.
FLARE_FORCE_INLINE strings_internal::alpha_num_formatter_impl alpha_num_formatter() {
    return strings_internal::alpha_num_formatter_impl();
}

// stream_formatter()
//
// Formats its argument using the << operator.
FLARE_FORCE_INLINE strings_internal::stream_formatter_impl stream_formatter() {
    return strings_internal::stream_formatter_impl();
}

// Function Template: pair_formatter(Formatter, std::string_view, Formatter)
//
// Formats a `std::pair` by putting a given separator between the pair's
// `.first` and `.second` members. This formatter allows you to specify
// custom Formatters for both the first and second member of each pair.
template<typename FirstFormatter, typename SecondFormatter>
FLARE_FORCE_INLINE strings_internal::pair_formatter_impl<FirstFormatter, SecondFormatter>
pair_formatter(FirstFormatter f1, std::string_view sep, SecondFormatter f2) {
    return strings_internal::pair_formatter_impl<FirstFormatter, SecondFormatter>(
            std::move(f1), sep, std::move(f2));
}

// Function overload of pair_formatter() for using a default
// `alpha_num_formatter()` for each Formatter in the pair.
FLARE_FORCE_INLINE strings_internal::pair_formatter_impl<
        strings_internal::alpha_num_formatter_impl,
        strings_internal::alpha_num_formatter_impl>
pair_formatter(std::string_view sep) {
    return pair_formatter(alpha_num_formatter(), sep, alpha_num_formatter());
}

// Function Template: dereference_formatter(Formatter)
//
// Formats its argument by dereferencing it and then applying the given
// formatter. This formatter is useful for formatting a container of
// pointer-to-T. This pattern often shows up when joining repeated fields in
// protocol buffers.
template<typename Formatter>
strings_internal::dereference_formatter_impl<Formatter> dereference_formatter(
        Formatter &&f) {
    return strings_internal::dereference_formatter_impl<Formatter>(
            std::forward<Formatter>(f));
}

// Function overload of `DererefenceFormatter()` for using a default
// `alpha_num_formatter()`.
FLARE_FORCE_INLINE strings_internal::dereference_formatter_impl<
        strings_internal::alpha_num_formatter_impl>
dereference_formatter() {
    return strings_internal::dereference_formatter_impl<
            strings_internal::alpha_num_formatter_impl>(alpha_num_formatter());
}

// -----------------------------------------------------------------------------
// string_join()
// -----------------------------------------------------------------------------
//
// Joins a range of elements and returns the result as a std::string.
// `flare::string_join()` takes a range, a separator string to use between the
// elements joined, and an optional Formatter responsible for converting each
// argument in the range to a string.
//
// If omitted, the default `alpha_num_formatter()` is called on the elements to be
// joined.
//
// Example 1:
//   // Joins a collection of strings. This pattern also works with a collection
//   // of `std::string_view` or even `const char*`.
//   std::vector<std::string> v = {"foo", "bar", "baz"};
//   std::string s = flare::string_join(v, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// Example 2:
//   // Joins the values in the given `std::initializer_list<>` specified using
//   // brace initialization. This pattern also works with an initializer_list
//   // of ints or `std::string_view` -- any `alpha_num`-compatible type.
//   std::string s = flare::string_join({"foo", "bar", "baz"}, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// Example 3:
//   // Joins a collection of ints. This pattern also works with floats,
//   // doubles, int64s -- any `string_cat()`-compatible type.
//   std::vector<int> v = {1, 2, 3, -4};
//   std::string s = flare::string_join(v, "-");
//   EXPECT_EQ("1-2-3--4", s);
//
// Example 4:
//   // Joins a collection of pointer-to-int. By default, pointers are
//   // dereferenced and the pointee is formatted using the default format for
//   // that type; such dereferencing occurs for all levels of indirection, so
//   // this pattern works just as well for `std::vector<int**>` as for
//   // `std::vector<int*>`.
//   int x = 1, y = 2, z = 3;
//   std::vector<int*> v = {&x, &y, &z};
//   std::string s = flare::string_join(v, "-");
//   EXPECT_EQ("1-2-3", s);
//
// Example 5:
//   // Dereferencing of `std::unique_ptr<>` is also supported:
//   std::vector<std::unique_ptr<int>> v
//   v.emplace_back(new int(1));
//   v.emplace_back(new int(2));
//   v.emplace_back(new int(3));
//   std::string s = flare::string_join(v, "-");
//   EXPECT_EQ("1-2-3", s);
//
// Example 6:
//   // Joins a `std::map`, with each key-value pair separated by an equals
//   // sign. This pattern would also work with, say, a
//   // `std::vector<std::pair<>>`.
//   std::map<std::string, int> m = {
//       std::make_pair("a", 1),
//       std::make_pair("b", 2),
//       std::make_pair("c", 3)};
//   std::string s = flare::string_join(m, ",", flare::pair_formatter("="));
//   EXPECT_EQ("a=1,b=2,c=3", s);
//
// Example 7:
//   // These examples show how `flare::string_join()` handles a few common edge
//   // cases:
//   std::vector<std::string> v_empty;
//   EXPECT_EQ("", flare::string_join(v_empty, "-"));
//
//   std::vector<std::string> v_one_item = {"foo"};
//   EXPECT_EQ("foo", flare::string_join(v_one_item, "-"));
//
//   std::vector<std::string> v_empty_string = {""};
//   EXPECT_EQ("", flare::string_join(v_empty_string, "-"));
//
//   std::vector<std::string> v_one_item_empty_string = {"a", ""};
//   EXPECT_EQ("a-", flare::string_join(v_one_item_empty_string, "-"));
//
//   std::vector<std::string> v_two_empty_string = {"", ""};
//   EXPECT_EQ("-", flare::string_join(v_two_empty_string, "-"));
//
// Example 8:
//   // Joins a `std::tuple<T...>` of heterogeneous types, converting each to
//   // a std::string using the `flare::alpha_num` class.
//   std::string s = flare::string_join(std::make_tuple(123, "abc", 0.456), "-");
//   EXPECT_EQ("123-abc-0.456", s);

template<typename Iterator, typename Formatter>
std::string string_join(Iterator start, Iterator end, std::string_view sep,
                        Formatter &&fmt) {
    return strings_internal::join_algorithm(start, end, sep, fmt);
}

template<typename Range, typename Formatter>
std::string string_join(const Range &range, std::string_view separator,
                        Formatter &&fmt) {
    return strings_internal::join_range(range, separator, fmt);
}

template<typename T, typename Formatter>
std::string string_join(std::initializer_list<T> il, std::string_view separator,
                        Formatter &&fmt) {
    return strings_internal::join_range(il, separator, fmt);
}

template<typename... T, typename Formatter>
std::string string_join(const std::tuple<T...> &value, std::string_view separator,
                        Formatter &&fmt) {
    return strings_internal::join_algorithm(value, separator, fmt);
}

template<typename Iterator>
std::string string_join(Iterator start, Iterator end, std::string_view separator) {
    return strings_internal::join_range(start, end, separator);
}

template<typename Range>
std::string string_join(const Range &range, std::string_view separator) {
    return strings_internal::join_range(range, separator);
}

template<typename T>
std::string string_join(std::initializer_list<T> il,
                        std::string_view separator) {
    return strings_internal::join_range(il, separator);
}

template<typename... T>
std::string string_join(const std::tuple<T...> &value,
                        std::string_view separator) {
    return strings_internal::join_algorithm(value, separator, alpha_num_formatter());
}


}  // namespace flare

#endif  // FLARE_STRINGS_STR_JOIN_H_
