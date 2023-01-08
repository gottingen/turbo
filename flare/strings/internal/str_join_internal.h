
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//

// This file declares INTERNAL parts of the Join API that are inlined/templated
// or otherwise need to be available at compile time. The main abstractions
// defined in this file are:
//
//   - A handful of default Formatters
//   - join_algorithm() overloads
//   - join_range() overloads
//   - JoinTuple()
//
// DO NOT INCLUDE THIS FILE DIRECTLY. Use this file by including
// flare/strings/str_join.h
//

#ifndef FLARE_STRINGS_INTERNAL_STR_JOIN_INTERNAL_H_
#define FLARE_STRINGS_INTERNAL_STR_JOIN_INTERNAL_H_

#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "flare/base/uninitialized.h"
#include "flare/strings/internal/ostringstream.h"
#include "flare/strings/str_cat.h"

namespace flare::strings_internal {

    //
    // Formatter objects
    //
    // The following are implementation classes for standard Formatter objects. The
    // factory functions that users will call to create and use these formatters are
    // defined and documented in strings/join.h.
    //

    // The default formatter. Converts alpha-numeric types to strings.
    struct alpha_num_formatter_impl {
        // This template is needed in order to support passing in a dereferenced
        // vector<bool>::iterator
        template<typename T>
        void operator()(std::string *out, const T &t) const {
            string_append(out, alpha_num(t));
        }

        void operator()(std::string *out, const alpha_num &t) const {
            string_append(out, t);
        }
    };

    // A type that's used to overload the join_algorithm() function (defined below)
    // for ranges that do not require additional formatting (e.g., a range of
    // strings).

    struct no_formatter : public alpha_num_formatter_impl {
    };

    // Formats types to strings using the << operator.
    class stream_formatter_impl {
    public:
        // The method isn't const because it mutates state. Making it const will
        // render stream_formatter_impl thread-hostile.
        template<typename T>
        void operator()(std::string *out, const T &t) {
            // The stream is created lazily to avoid paying the relatively high cost
            // of its construction when joining an empty range.
            if (strm_) {
                strm_->clear();  // clear the bad, fail and eof bits in case they were set
                strm_->str(out);
            } else {
                strm_.reset(new strings_internal::string_output_stream(out));
            }
            *strm_ << t;
        }

    private:
        std::unique_ptr<strings_internal::string_output_stream> strm_;
    };

    // Formats a std::pair<>. The 'first' member is formatted using f1_ and the
    // 'second' member is formatted using f2_. sep_ is the separator.
    template<typename F1, typename F2>
    class pair_formatter_impl {
    public:
        pair_formatter_impl(F1 f1, std::string_view sep, F2 f2)
                : f1_(std::move(f1)), sep_(sep), f2_(std::move(f2)) {}

        template<typename T>
        void operator()(std::string *out, const T &p) {
            f1_(out, p.first);
            out->append(sep_);
            f2_(out, p.second);
        }

        template<typename T>
        void operator()(std::string *out, const T &p) const {
            f1_(out, p.first);
            out->append(sep_);
            f2_(out, p.second);
        }

    private:
        F1 f1_;
        std::string sep_;
        F2 f2_;
    };

    // Wraps another formatter and dereferences the argument to operator() then
    // passes the dereferenced argument to the wrapped formatter. This can be
    // useful, for example, to join a std::vector<int*>.
    template<typename Formatter>
    class dereference_formatter_impl {
    public:
        dereference_formatter_impl() : f_() {}

        explicit dereference_formatter_impl(Formatter &&f)
                : f_(std::forward<Formatter>(f)) {}

        template<typename T>
        void operator()(std::string *out, const T &t) {
            f_(out, *t);
        }

        template<typename T>
        void operator()(std::string *out, const T &t) const {
            f_(out, *t);
        }

    private:
        Formatter f_;
    };

    // default_formatter<T> is a traits class that selects a default Formatter to use
    // for the given type T. The ::Type member names the Formatter to use. This is
    // used by the strings::Join() functions that do NOT take a Formatter argument,
    // in which case a default Formatter must be chosen.
    //
    // alpha_num_formatter_impl is the default in the base template, followed by
    // specializations for other types.
    template<typename ValueType>
    struct default_formatter {
        typedef alpha_num_formatter_impl Type;
    };
    template<>
    struct default_formatter<const char *> {
        typedef alpha_num_formatter_impl Type;
    };
    template<>
    struct default_formatter<char *> {
        typedef alpha_num_formatter_impl Type;
    };
    template<>
    struct default_formatter<std::string> {
        typedef no_formatter Type;
    };
    template<>
    struct default_formatter<std::string_view> {
        typedef no_formatter Type;
    };
    template<typename ValueType>
    struct default_formatter<ValueType *> {
        typedef dereference_formatter_impl<typename default_formatter<ValueType>::Type>
                Type;
    };

    template<typename ValueType>
    struct default_formatter<std::unique_ptr<ValueType>>
            : public default_formatter<ValueType *> {
    };

    //
    // join_algorithm() functions
    //

    // The main joining algorithm. This simply joins the elements in the given
    // iterator range, each separated by the given separator, into an output string,
    // and formats each element using the provided Formatter object.
    template<typename Iterator, typename Formatter>
    std::string join_algorithm(Iterator start, Iterator end, std::string_view s,
                               Formatter &&f) {
        std::string result;
        std::string_view sep("");
        for (Iterator it = start; it != end; ++it) {
            result.append(sep.data(), sep.size());
            f(&result, *it);
            sep = s;
        }
        return result;
    }

    // A joining algorithm that's optimized for a forward iterator range of
    // string-like objects that do not need any additional formatting. This is to
    // optimize the common case of joining, say, a std::vector<string> or a
    // std::vector<std::string_view>.
    //
    // This is an overload of the previous join_algorithm() function. Here the
    // Formatter argument is of type no_formatter. Since no_formatter is an internal
    // type, this overload is only invoked when strings::Join() is called with a
    // range of string-like objects (e.g., std::string, std::string_view), and an
    // explicit Formatter argument was NOT specified.
    //
    // The optimization is that the needed space will be reserved in the output
    // string to avoid the need to resize while appending. To do this, the iterator
    // range will be traversed twice: once to calculate the total needed size, and
    // then again to copy the elements and delimiters to the output string.
    template<typename Iterator,
            typename = typename std::enable_if<std::is_convertible<
                    typename std::iterator_traits<Iterator>::iterator_category,
                    std::forward_iterator_tag>::value>::type>
    std::string join_algorithm(Iterator start, Iterator end, std::string_view s,
                               no_formatter) {
        std::string result;
        if (start != end) {
            // Sums size
            size_t result_size = start->size();
            for (Iterator it = start; ++it != end;) {
                result_size += s.size();
                result_size += it->size();
            }

            if (result_size > 0) {
                flare::base::string_resize_uninitialized(&result, result_size);

                // Joins strings
                char *result_buf = &*result.begin();
                memcpy(result_buf, start->data(), start->size());
                result_buf += start->size();
                for (Iterator it = start; ++it != end;) {
                    memcpy(result_buf, s.data(), s.size());
                    result_buf += s.size();
                    memcpy(result_buf, it->data(), it->size());
                    result_buf += it->size();
                }
            }
        }

        return result;
    }

    // join_tuple_loop implements a loop over the elements of a std::tuple, which
    // are heterogeneous. The primary template matches the tuple interior case. It
    // continues the iteration after appending a separator (for nonzero indices)
    // and formatting an element of the tuple. The specialization for the I=N case
    // matches the end-of-tuple, and terminates the iteration.
    template<size_t I, size_t N>
    struct join_tuple_loop {
        template<typename Tup, typename Formatter>
        void operator()(std::string *out, const Tup &tup, std::string_view sep,
                        Formatter &&fmt) {
            if (I > 0) out->append(sep.data(), sep.size());
            fmt(out, std::get<I>(tup));
            join_tuple_loop<I + 1, N>()(out, tup, sep, fmt);
        }
    };

    template<size_t N>
    struct join_tuple_loop<N, N> {
        template<typename Tup, typename Formatter>
        void operator()(std::string *, const Tup &, std::string_view, Formatter &&) {}
    };

    template<typename... T, typename Formatter>
    std::string join_algorithm(const std::tuple<T...> &tup, std::string_view sep,
                               Formatter &&fmt) {
        std::string result;
        join_tuple_loop<0, sizeof...(T)>()(&result, tup, sep, fmt);
        return result;
    }

    template<typename Iterator>
    std::string join_range(Iterator first, Iterator last,
                           std::string_view separator) {
        // No formatter was explicitly given, so a default must be chosen.
        typedef typename std::iterator_traits<Iterator>::value_type ValueType;
        typedef typename default_formatter<ValueType>::Type Formatter;
        return join_algorithm(first, last, separator, Formatter());
    }

    template<typename Range, typename Formatter>
    std::string join_range(const Range &range, std::string_view separator,
                           Formatter &&fmt) {
        using std::begin;
        using std::end;
        return join_algorithm(begin(range), end(range), separator, fmt);
    }

    template<typename Range>
    std::string join_range(const Range &range, std::string_view separator) {
        using std::begin;
        using std::end;
        return join_range(begin(range), end(range), separator);
    }


}  // namespace flare::strings_internal

#endif  // FLARE_STRINGS_INTERNAL_STR_JOIN_INTERNAL_H_
