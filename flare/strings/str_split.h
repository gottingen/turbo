
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: str_split.h
// -----------------------------------------------------------------------------
//
// This file contains functions for splitting strings. It defines the main
// ` string_split()` function, several delimiters for determining the boundaries on
// which to split the string, and predicates for filtering delimited results.
// ` string_split()` adapts the returned collection to the type specified by the
// caller.
//
// Example:
//
//   // Splits the given string on commas. Returns the results in a
//   // vector of strings.
//   std::vector<std::string> v = flare:: string_split("a,b,c", ',');
//   // Can also use ","
//   // v[0] == "a", v[1] == "b", v[2] == "c"
//
// See  string_split() below for more information.
#ifndef FLARE_STRINGS_STR_SPLIT_H_
#define FLARE_STRINGS_STR_SPLIT_H_

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <string_view>

#include "flare/log/logging.h"
#include "flare/strings/internal/str_split_internal.h"
#include "flare/strings/strip.h"
#include "flare/strings/trim.h"

namespace flare {


    //------------------------------------------------------------------------------
    // Delimiters
    //------------------------------------------------------------------------------
    //
    // ` string_split()` uses delimiters to define the boundaries between elements in the
    // provided input. Several `Delimiter` types are defined below. If a string
    // (`const char*`, `std::string`, or `std::string_view`) is passed in place of
    // an explicit `Delimiter` object, ` string_split()` treats it the same way as if it
    // were passed a `by_string` delimiter.
    //
    // A `Delimiter` is an object with a `find()` function that knows how to find
    // the first occurrence of itself in a given `std::string_view`.
    //
    // The following `Delimiter` types are available for use within ` string_split()`:
    //
    //   - `by_string` (default for string arguments)
    //   - `by_char` (default for a char argument)
    //   - `by_any_char`
    //   - `by_length`
    //   - `max_splits`
    //
    // A Delimiter's `find()` member function will be passed an input `text` that is
    // to be split and a position (`pos`) to begin searching for the next delimiter
    // in `text`. The returned std::string_view should refer to the next occurrence
    // (after `pos`) of the represented delimiter; this returned std::string_view
    // represents the next location where the input `text` should be broken.
    //
    // The returned std::string_view may be zero-length if the Delimiter does not
    // represent a part of the string (e.g., a fixed-length delimiter). If no
    // delimiter is found in the input `text`, a zero-length std::string_view
    // referring to `text.end()` should be returned (e.g.,
    // `text.substr(text.size())`). It is important that the returned
    // std::string_view always be within the bounds of the input `text` given as an
    // argument--it must not refer to a string that is physically located outside of
    // the given string.
    //
    // The following example is a simple Delimiter object that is created with a
    // single char and will look for that char in the text passed to the `find()`
    // function:
    //
    //   struct SimpleDelimiter {
    //     const char c_;
    //     explicit SimpleDelimiter(char c) : c_(c) {}
    //     std::string_view find(std::string_view text, size_t pos) {
    //       auto found = text.find(c_, pos);
    //       if (found == std::string_view::npos)
    //         return text.substr(text.size());
    //
    //       return text.substr(found, 1);
    //     }
    //   };

    // by_string
    //
    // A sub-string delimiter. If ` string_split()` is passed a string in place of a
    // `Delimiter` object, the string will be implicitly converted into a
    // `by_string` delimiter.
    //
    // Example:
    //
    //   // Because a string literal is converted to an `flare::by_string`,
    //   // the following two splits are equivalent.
    //
    //   std::vector<std::string> v1 = flare:: string_split("a, b, c", ", ");
    //
    //   using flare::by_string;
    //   std::vector<std::string> v2 = flare:: string_split("a, b, c",
    //                                                by_string(", "));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    class by_string {
    public:
        explicit by_string(std::string_view sp);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const std::string _delimiter;
    };

    // by_char
    //
    // A single character delimiter. `by_char` is functionally equivalent to a
    // 1-char string within a `by_string` delimiter, but slightly more efficient.
    //
    // Example:
    //
    //   // Because a char literal is converted to a flare::by_char,
    //   // the following two splits are equivalent.
    //   std::vector<std::string> v1 = flare:: string_split("a,b,c", ',');
    //   using flare::by_char;
    //   std::vector<std::string> v2 = flare:: string_split("a,b,c", by_char(','));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // `by_char` is also the default delimiter if a single character is given
    // as the delimiter to ` string_split()`. For example, the following calls are
    // equivalent:
    //
    //   std::vector<std::string> v = flare:: string_split("a-b", '-');
    //
    //   using flare::by_char;
    //   std::vector<std::string> v = flare:: string_split("a-b", by_char('-'));
    //
    class by_char {
    public:
        explicit by_char(char c) : c_(c) {}

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        char c_;
    };

    // by_any_char
    //
    // A delimiter that will match any of the given byte-sized characters within
    // its provided string.
    //
    // Note: this delimiter works with single-byte string data, but does not work
    // with variable-width encodings, such as UTF-8.
    //
    // Example:
    //
    //   using flare::by_any_char;
    //   std::vector<std::string> v = flare:: string_split("a,b=c", by_any_char(",="));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // If `by_any_char` is given the empty string, it behaves exactly like
    // `by_string` and matches each individual character in the input string.
    //
    class by_any_char {
    public:
        explicit by_any_char(std::string_view sp);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const std::string _delimiters;
    };

    //  by_length
    //
    // A delimiter for splitting into equal-length strings. The length argument to
    // the constructor must be greater than 0.
    //
    // Note: this delimiter works with single-byte string data, but does not work
    // with variable-width encodings, such as UTF-8.
    //
    // Example:
    //
    //   using flare:: by_length;
    //   std::vector<std::string> v = flare:: string_split("123456789",  by_length(3));

    //   // v[0] == "123", v[1] == "456", v[2] == "789"
    //
    // Note that the string does not have to be a multiple of the fixed split
    // length. In such a case, the last substring will be shorter.
    //
    //   using flare:: by_length;
    //   std::vector<std::string> v = flare:: string_split("12345",  by_length(2));
    //
    //   // v[0] == "12", v[1] == "34", v[2] == "5"
    class by_length {
    public:
        explicit by_length(ptrdiff_t length);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const ptrdiff_t _length;
    };

    namespace strings_internal {

        // A traits-like metafunction for selecting the default Delimiter object type
        // for a particular Delimiter type. The base case simply exposes type Delimiter
        // itself as the delimiter's Type. However, there are specializations for
        // string-like objects that map them to the by_string delimiter object.
        // This allows functions like flare:: string_split() and flare:: max_splits() to accept
        // string-like objects (e.g., ',') as delimiter arguments but they will be
        // treated as if a by_string delimiter was given.
        template<typename Delimiter>
        struct select_delimiter {
            using type = Delimiter;
        };

        template<>
        struct select_delimiter<char> {
            using type = by_char;
        };
        template<>
        struct select_delimiter<char *> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<const char *> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<std::string_view> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<std::string> {
            using type = by_string;
        };

        // Wraps another delimiter and sets a max number of matches for that delimiter.
        template<typename Delimiter>
        class max_splits_impl {
        public:
            max_splits_impl(Delimiter delimiter, int limit)
                    : _delimiter(delimiter), _limit(limit), _count(0) {}

            std::string_view find(std::string_view text, size_t pos) {
                if (_count++ == _limit) {
                    return std::string_view(text.data() + text.size(),
                                            0);  // No more matches.
                }
                return _delimiter.find(text, pos);
            }

        private:
            Delimiter _delimiter;
            const int _limit;
            int _count;
        };

    }  // namespace strings_internal

    //  max_splits()
    //
    // A delimiter that limits the number of matches which can occur to the passed
    // `limit`. The last element in the returned collection will contain all
    // remaining unsplit pieces, which may contain instances of the delimiter.
    // The collection will contain at most `limit` + 1 elements.
    // Example:
    //
    //   using flare:: max_splits;
    //   std::vector<std::string> v = flare:: string_split("a,b,c",  max_splits(',', 1));
    //
    //   // v[0] == "a", v[1] == "b,c"
    template<typename Delimiter>
    FLARE_FORCE_INLINE strings_internal::max_splits_impl<
            typename strings_internal::select_delimiter<Delimiter>::type>
    max_splits(Delimiter delimiter, int limit) {
        typedef
        typename strings_internal::select_delimiter<Delimiter>::type DelimiterType;
        return strings_internal::max_splits_impl<DelimiterType>(
                DelimiterType(delimiter), limit);
    }

    //------------------------------------------------------------------------------
    // Predicates
    //------------------------------------------------------------------------------
    //
    // Predicates filter the results of a ` string_split()` by determining whether or not
    // a resultant element is included in the result set. A predicate may be passed
    // as an optional third argument to the ` string_split()` function.
    //
    // Predicates are unary functions (or functors) that take a single
    // `std::string_view` argument and return a bool indicating whether the
    // argument should be included (`true`) or excluded (`false`).
    //
    // Predicates are useful when filtering out empty substrings. By default, empty
    // substrings may be returned by ` string_split()`, which is similar to the way split
    // functions work in other programming languages.

    //  allow_empty()
    //
    // Always returns `true`, indicating that all strings--including empty
    // strings--should be included in the split output. This predicate is not
    // strictly needed because this is the default behavior of ` string_split()`;
    // however, it might be useful at some call sites to make the intent explicit.
    //
    // Example:
    //
    //  std::vector<std::string> v = flare:: string_split(" a , ,,b,", ',',  allow_empty());
    //
    //  // v[0] == " a ", v[1] == " ", v[2] == "", v[3] = "b", v[4] == ""
    struct allow_empty {
        bool operator()(std::string_view) const { return true; }
    };

    //  skip_empty()
    //
    // Returns `false` if the given `std::string_view` is empty, indicating that
    // ` string_split()` should omit the empty string.
    //
    // Example:
    //
    //   std::vector<std::string> v = flare:: string_split(",a,,b,", ',',  skip_empty());
    //
    //   // v[0] == "a", v[1] == "b"
    //
    // Note: ` skip_empty()` does not consider a string containing only whitespace
    // to be empty. To skip such whitespace as well, use the ` skip_whitespace()`
    // predicate.
    struct skip_empty {
        bool operator()(std::string_view sp) const { return !sp.empty(); }
    };

    //  skip_whitespace()
    //
    // Returns `false` if the given `std::string_view` is empty *or* contains only
    // whitespace, indicating that ` string_split()` should omit the string.
    //
    // Example:
    //
    //   std::vector<std::string> v = flare:: string_split(" a , ,,b,",
    //                                               ',',  skip_whitespace());
    //   // v[0] == " a ", v[1] == "b"
    //
    //   //  skip_empty() would return whitespace elements
    //   std::vector<std::string> v = flare:: string_split(" a , ,,b,", ',',  skip_empty());
    //   // v[0] == " a ", v[1] == " ", v[2] == "b"
    struct skip_whitespace {
        bool operator()(std::string_view sp) const {
            sp = flare::trim_all(sp);
            return !sp.empty();
        }
    };

    //------------------------------------------------------------------------------
    //                                   string_split()
    //------------------------------------------------------------------------------

    //  string_split()
    //
    // Splits a given string based on the provided `Delimiter` object, returning the
    // elements within the type specified by the caller. Optionally, you may pass a
    // `Predicate` to ` string_split()` indicating whether to include or exclude the
    // resulting element within the final result set. (See the overviews for
    // Delimiters and Predicates above.)
    //
    // Example:
    //
    //   std::vector<std::string> v = flare:: string_split("a,b,c,d", ',');
    //   // v[0] == "a", v[1] == "b", v[2] == "c", v[3] == "d"
    //
    // You can also provide an explicit `Delimiter` object:
    //
    // Example:
    //
    //   using flare::by_any_char;
    //   std::vector<std::string> v = flare:: string_split("a,b=c", by_any_char(",="));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // See above for more information on delimiters.
    //
    // By default, empty strings are included in the result set. You can optionally
    // include a third `Predicate` argument to apply a test for whether the
    // resultant element should be included in the result set:
    //
    // Example:
    //
    //   std::vector<std::string> v = flare:: string_split(" a , ,,b,",
    //                                               ',',  skip_whitespace());
    //   // v[0] == " a ", v[1] == "b"
    //
    // See above for more information on predicates.
    //
    //------------------------------------------------------------------------------
    //  string_split() Return Types
    //------------------------------------------------------------------------------
    //
    // The ` string_split()` function adapts the returned collection to the collection
    // specified by the caller (e.g. `std::vector` above). The returned collections
    // may contain `std::string`, `std::string_view` (in which case the original
    // string being split must ensure that it outlives the collection), or any
    // object that can be explicitly created from an `std::string_view`. This
    // behavior works for:
    //
    // 1) All standard STL containers including `std::vector`, `std::list`,
    //    `std::deque`, `std::set`,`std::multiset`, 'std::map`, and `std::multimap`
    // 2) `std::pair` (which is not actually a container). See below.
    //
    // Example:
    //
    //   // The results are returned as `std::string_view` objects. Note that we
    //   // have to ensure that the input string outlives any results.
    //   std::vector<std::string_view> v = flare:: string_split("a,b,c", ',');
    //
    //   // Stores results in a std::set<std::string>, which also performs
    //   // de-duplication and orders the elements in ascending order.
    //   std::set<std::string> a = flare:: string_split("b,a,c,a,b", ',');
    //   // v[0] == "a", v[1] == "b", v[2] = "c"
    //
    //   // ` string_split()` can be used within a range-based for loop, in which case
    //   // each element will be of type `std::string_view`.
    //   std::vector<std::string> v;
    //   for (const auto sv : flare:: string_split("a,b,c", ',')) {
    //     if (sv != "b") v.emplace_back(sv);
    //   }
    //   // v[0] == "a", v[1] == "c"
    //
    //   // Stores results in a map. The map implementation assumes that the input
    //   // is provided as a series of key/value pairs. For example, the 0th element
    //   // resulting from the split will be stored as a key to the 1st element. If
    //   // an odd number of elements are resolved, the last element is paired with
    //   // a default-constructed value (e.g., empty string).
    //   std::map<std::string, std::string> m = flare:: string_split("a,b,c", ',');
    //   // m["a"] == "b", m["c"] == ""     // last component value equals ""
    //
    // Splitting to `std::pair` is an interesting case because it can hold only two
    // elements and is not a collection type. When splitting to a `std::pair` the
    // first two split strings become the `std::pair` `.first` and `.second`
    // members, respectively. The remaining split substrings are discarded. If there
    // are less than two split substrings, the empty string is used for the
    // corresponding
    // `std::pair` member.
    //
    // Example:
    //
    //   // Stores first two split strings as the members in a std::pair.
    //   std::pair<std::string, std::string> p = flare:: string_split("a,b,c", ',');
    //   // p.first == "a", p.second == "b"       // "c" is omitted.
    //
    // The ` string_split()` function can be used multiple times to perform more
    // complicated splitting logic, such as intelligently parsing key-value pairs.
    //
    // Example:
    //
    //   // The input string "a=b=c,d=e,f=,g" becomes
    //   // { "a" => "b=c", "d" => "e", "f" => "", "g" => "" }
    //   std::map<std::string, std::string> m;
    //   for (std::string_view sp : flare:: string_split("a=b=c,d=e,f=,g", ',')) {
    //     m.insert(flare:: string_split(sp, flare:: max_splits('=', 1)));
    //   }
    //   EXPECT_EQ("b=c", m.find("a")->second);
    //   EXPECT_EQ("e", m.find("d")->second);
    //   EXPECT_EQ("", m.find("f")->second);
    //   EXPECT_EQ("", m.find("g")->second);
    //
    // WARNING: Due to a legacy bug that is maintained for backward compatibility,
    // splitting the following empty string_views produces different results:
    //
    //   flare:: string_split(std::string_view(""), '-');  // {""}
    //   flare:: string_split(std::string_view(), '-');    // {}, but should be {""}
    //
    // Try not to depend on this distinction because the bug may one day be fixed.
    template<typename Delimiter>
    strings_internal::splitter<
            typename strings_internal::select_delimiter<Delimiter>::type, allow_empty>
    string_split(strings_internal::convertible_to_string_view text, Delimiter d) {
        using DelimiterType =
        typename strings_internal::select_delimiter<Delimiter>::type;
        return strings_internal::splitter<DelimiterType, allow_empty>(
                std::move(text), DelimiterType(d), allow_empty());
    }

    template<typename Delimiter, typename Predicate>
    strings_internal::splitter<
            typename strings_internal::select_delimiter<Delimiter>::type, Predicate>
    string_split(strings_internal::convertible_to_string_view text, Delimiter d,
                 Predicate p) {
        using DelimiterType =
        typename strings_internal::select_delimiter<Delimiter>::type;
        return strings_internal::splitter<DelimiterType, Predicate>(
                std::move(text), DelimiterType(d), std::move(p));
    }


}  // namespace flare

#endif  // FLARE_STRINGS_STR_SPLIT_H_
