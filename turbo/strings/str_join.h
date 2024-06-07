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
// -----------------------------------------------------------------------------
// File: str_join.h
// -----------------------------------------------------------------------------
//
// This header file contains functions for joining a range of elements and
// returning the result as a std::string. str_join operations are specified by
// passing a range, a separator string to use between the elements joined, and
// an optional Formatter responsible for converting each argument in the range
// to a string. If omitted, a default `alpha_num_formatter()` is called on the
// elements to be joined, using the same formatting that `turbo::str_cat()` uses.
// This package defines a number of default formatters, and you can define your
// own implementations.
//
// Ranges are specified by passing a container with `std::begin()` and
// `std::end()` iterators, container-specific `begin()` and `end()` iterators, a
// brace-initialized `std::initializer_list`, or a `std::tuple` of heterogeneous
// objects. The separator string is specified as an `std::string_view`.
//
// Because the default formatter uses the `turbo::AlphaNum` class,
// `turbo::str_join()`, like `turbo::str_cat()`, will work out-of-the-box on
// collections of strings, ints, floats, doubles, etc.
//
// Example:
//
//   std::vector<std::string> v = {"foo", "bar", "baz"};
//   std::string s = turbo::str_join(v, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// See comments on the `turbo::str_join()` function for more examples.

#pragma once

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <turbo/base/macros.h>
#include <turbo/strings/internal/str_join_internal.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    // -----------------------------------------------------------------------------
    // Concept: Formatter
    // -----------------------------------------------------------------------------
    //
    // A Formatter is a function object that is responsible for formatting its
    // argument as a string and appending it to a given output std::string.
    // Formatters may be implemented as function objects, lambdas, or normal
    // functions. You may provide your own Formatter to enable `turbo::str_join()` to
    // work with arbitrary types.
    //
    // The following is an example of a custom Formatter that uses
    // `turbo::Duration::format` to join a list of `turbo::Duration`s.
    //
    //   std::vector<turbo::Duration> v = {turbo::Duration::seconds(1), turbo::Duration::milliseconds(10)};
    //   std::string s =
    //       turbo::str_join(v, ", ", [](std::string* out, turbo::Duration dur) {
    //         turbo::str_append(out, turbo::Duration::format(dur));
    //       });
    //   EXPECT_EQ(s, "1s, 10ms");
    //
    // The following standard formatters are provided within this file:
    //
    // - `alpha_num_formatter()` (the default)
    // - `stream_formatter()`
    // - `pair_formatter()`
    // - `dereference_formatter()`

    // alpha_num_formatter()
    //
    // Default formatter used if none is specified. Uses `turbo::AlphaNum` to convert
    // numeric arguments to strings.
    inline strings_internal::AlphaNumFormatterImpl alpha_num_formatter() {
        return strings_internal::AlphaNumFormatterImpl();
    }

    // stream_formatter()
    //
    // Formats its argument using the << operator.
    inline strings_internal::StreamFormatterImpl stream_formatter() {
        return strings_internal::StreamFormatterImpl();
    }

    // Function Template: pair_formatter(Formatter, std::string_view, Formatter)
    //
    // Formats a `std::pair` by putting a given separator between the pair's
    // `.first` and `.second` members. This formatter allows you to specify
    // custom Formatters for both the first and second member of each pair.
    template<typename FirstFormatter, typename SecondFormatter>
    inline strings_internal::PairFormatterImpl<FirstFormatter, SecondFormatter>
    pair_formatter(FirstFormatter f1, std::string_view sep, SecondFormatter f2) {
        return strings_internal::PairFormatterImpl<FirstFormatter, SecondFormatter>(
                std::move(f1), sep, std::move(f2));
    }

    // Function overload of pair_formatter() for using a default
    // `alpha_num_formatter()` for each Formatter in the pair.
    inline strings_internal::PairFormatterImpl<
            strings_internal::AlphaNumFormatterImpl,
            strings_internal::AlphaNumFormatterImpl>
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
    strings_internal::DereferenceFormatterImpl<Formatter> dereference_formatter(
            Formatter &&f) {
        return strings_internal::DereferenceFormatterImpl<Formatter>(
                std::forward<Formatter>(f));
    }

    // Function overload of `dereference_formatter()` for using a default
    // `alpha_num_formatter()`.
    inline strings_internal::DereferenceFormatterImpl<
            strings_internal::AlphaNumFormatterImpl>
    dereference_formatter() {
        return strings_internal::DereferenceFormatterImpl<
                strings_internal::AlphaNumFormatterImpl>(alpha_num_formatter());
    }

    // -----------------------------------------------------------------------------
    // str_join()
    // -----------------------------------------------------------------------------
    //
    // Joins a range of elements and returns the result as a std::string.
    // `turbo::str_join()` takes a range, a separator string to use between the
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
    //   std::string s = turbo::str_join(v, "-");
    //   EXPECT_EQ(s, "foo-bar-baz");
    //
    // Example 2:
    //   // Joins the values in the given `std::initializer_list<>` specified using
    //   // brace initialization. This pattern also works with an initializer_list
    //   // of ints or `std::string_view` -- any `AlphaNum`-compatible type.
    //   std::string s = turbo::str_join({"foo", "bar", "baz"}, "-");
    //   EXPECT_EQs, "foo-bar-baz");
    //
    // Example 3:
    //   // Joins a collection of ints. This pattern also works with floats,
    //   // doubles, int64s -- any `str_cat()`-compatible type.
    //   std::vector<int> v = {1, 2, 3, -4};
    //   std::string s = turbo::str_join(v, "-");
    //   EXPECT_EQ(s, "1-2-3--4");
    //
    // Example 4:
    //   // Joins a collection of pointer-to-int. By default, pointers are
    //   // dereferenced and the pointee is formatted using the default format for
    //   // that type; such dereferencing occurs for all levels of indirection, so
    //   // this pattern works just as well for `std::vector<int**>` as for
    //   // `std::vector<int*>`.
    //   int x = 1, y = 2, z = 3;
    //   std::vector<int*> v = {&x, &y, &z};
    //   std::string s = turbo::str_join(v, "-");
    //   EXPECT_EQ(s, "1-2-3");
    //
    // Example 5:
    //   // Dereferencing of `std::unique_ptr<>` is also supported:
    //   std::vector<std::unique_ptr<int>> v
    //   v.emplace_back(new int(1));
    //   v.emplace_back(new int(2));
    //   v.emplace_back(new int(3));
    //   std::string s = turbo::str_join(v, "-");
    //   EXPECT_EQ(s, "1-2-3");
    //
    // Example 6:
    //   // Joins a `std::map`, with each key-value pair separated by an equals
    //   // sign. This pattern would also work with, say, a
    //   // `std::vector<std::pair<>>`.
    //   std::map<std::string, int> m = {
    //       {"a", 1},
    //       {"b", 2},
    //       {"c", 3}};
    //   std::string s = turbo::str_join(m, ",", turbo::pair_formatter("="));
    //   EXPECT_EQ(s, "a=1,b=2,c=3");
    //
    // Example 7:
    //   // These examples show how `turbo::str_join()` handles a few common edge
    //   // cases:
    //   std::vector<std::string> v_empty;
    //   EXPECT_EQ(turbo::str_join(v_empty, "-"), "");
    //
    //   std::vector<std::string> v_one_item = {"foo"};
    //   EXPECT_EQ(turbo::str_join(v_one_item, "-"), "foo");
    //
    //   std::vector<std::string> v_empty_string = {""};
    //   EXPECT_EQ(turbo::str_join(v_empty_string, "-"), "");
    //
    //   std::vector<std::string> v_one_item_empty_string = {"a", ""};
    //   EXPECT_EQ(turbo::str_join(v_one_item_empty_string, "-"), "a-");
    //
    //   std::vector<std::string> v_two_empty_string = {"", ""};
    //   EXPECT_EQ(turbo::str_join(v_two_empty_string, "-"), "-");
    //
    // Example 8:
    //   // Joins a `std::tuple<T...>` of heterogeneous types, converting each to
    //   // a std::string using the `turbo::AlphaNum` class.
    //   std::string s = turbo::str_join(std::make_tuple(123, "abc", 0.456), "-");
    //   EXPECT_EQ(s, "123-abc-0.456");

    template<typename Iterator, typename Formatter>
    std::string str_join(Iterator start, Iterator end, std::string_view sep,
                        Formatter &&fmt) {
        return strings_internal::JoinAlgorithm(start, end, sep, fmt);
    }

    template<typename Range, typename Formatter>
    std::string str_join(const Range &range, std::string_view separator,
                        Formatter &&fmt) {
        return strings_internal::JoinRange(range, separator, fmt);
    }

    template<typename T, typename Formatter,
            typename = typename std::enable_if<
                    !std::is_convertible<T, std::string_view>::value>::type>
    std::string str_join(std::initializer_list<T> il, std::string_view separator,
                        Formatter &&fmt) {
        return strings_internal::JoinRange(il, separator, fmt);
    }

    template<typename Formatter>
    inline std::string str_join(std::initializer_list<std::string_view> il,
                               std::string_view separator, Formatter &&fmt) {
        return strings_internal::JoinRange(il, separator, fmt);
    }

    template<typename... T, typename Formatter>
    std::string str_join(const std::tuple<T...> &value, std::string_view separator,
                        Formatter &&fmt) {
        return strings_internal::JoinAlgorithm(value, separator, fmt);
    }

    template<typename Iterator>
    std::string str_join(Iterator start, Iterator end, std::string_view separator) {
        return strings_internal::JoinRange(start, end, separator);
    }

    template<typename Range>
    std::string str_join(const Range &range, std::string_view separator) {
        return strings_internal::JoinRange(range, separator);
    }

    template<typename T, typename = typename std::enable_if<!std::is_convertible<
            T, std::string_view>::value>::type>
    std::string str_join(std::initializer_list<T> il, std::string_view separator) {
        return strings_internal::JoinRange(il, separator);
    }

    inline std::string str_join(std::initializer_list<std::string_view> il,
                               std::string_view separator) {
        return strings_internal::JoinRange(il, separator);
    }

    template<typename... T>
    std::string str_join(const std::tuple<T...> &value,
                        std::string_view separator) {
        return strings_internal::JoinTuple(value, separator,
                                           std::index_sequence_for<T...>{});
    }

}  // namespace turbo
