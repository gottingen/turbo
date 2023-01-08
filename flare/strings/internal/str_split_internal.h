
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

// This file declares INTERNAL parts of the Split API that are inline/templated
// or otherwise need to be available at compile time. The main abstractions
// defined in here are
//
//   - convertible_to_string_view
//   - split_iterator<>
//   - Splitter<>
//
// DO NOT INCLUDE THIS FILE DIRECTLY. Use this file by including
// flare/strings/str_split.h.
//

#ifndef FLARE_STRINGS_INTERNAL_STR_SPLIT_INTERNAL_H_
#define FLARE_STRINGS_INTERNAL_STR_SPLIT_INTERNAL_H_

#include <array>
#include <initializer_list>
#include <iterator>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>
#include <string_view>
#include "flare/base/profile.h"
#include "flare/base/type_traits.h"


namespace flare::strings_internal {

    // This class is implicitly constructible from everything that std::string_view
    // is implicitly constructible from. If it's constructed from a temporary
    // string, the data is moved into a data member so its lifetime matches that of
    // the convertible_to_string_view instance.
    class convertible_to_string_view {
    public:
        convertible_to_string_view(const char *s)  // NOLINT(runtime/explicit)
                : value_(s) {}

        convertible_to_string_view(char *s) : value_(s) {}  // NOLINT(runtime/explicit)
        convertible_to_string_view(std::string_view s)     // NOLINT(runtime/explicit)
                : value_(s) {}

        convertible_to_string_view(const std::string &s)  // NOLINT(runtime/explicit)
                : value_(s) {}

        // Matches rvalue strings and moves their data to a member.
        convertible_to_string_view(std::string &&s)  // NOLINT(runtime/explicit)
                : copy_(std::move(s)), value_(copy_) {}

        convertible_to_string_view(const convertible_to_string_view &other)
                : copy_(other.copy_),
                  value_(other.is_self_referential() ? copy_ : other.value_) {}

        convertible_to_string_view(convertible_to_string_view &&other) {
            steal_members(std::move(other));
        }

        convertible_to_string_view &operator=(convertible_to_string_view other) {
            steal_members(std::move(other));
            return *this;
        }

        std::string_view value() const { return value_; }

    private:

        // Returns true if ctsp's value refers to its internal copy_ member.
        bool is_self_referential() const { return value_.data() == copy_.data(); }

        void steal_members(convertible_to_string_view &&other) {
            if (other.is_self_referential()) {
                copy_ = std::move(other.copy_);
                value_ = copy_;
                other.value_ = other.copy_;
            } else {
                value_ = other.value_;
            }
        }

        // Holds the data moved from temporary std::string arguments. Declared first
        // so that 'value' can refer to 'copy_'.
        std::string copy_;
        std::string_view value_;
    };

    // An iterator that enumerates the parts of a string from a Splitter. The text
    // to be split, the Delimiter, and the Predicate are all taken from the given
    // Splitter object. Iterators may only be compared if they refer to the same
    // Splitter instance.
    //
    // This class is NOT part of the public splitting API.
    template<typename Splitter>
    class split_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        enum State {
            kInitState, kLastState, kEndState
        };

        split_iterator(State state, const Splitter *splitter)
                : pos_(0),
                  state_(state),
                  splitter_(splitter),
                  delimiter_(splitter->delimiter()),
                  predicate_(splitter->predicate()) {
            // Hack to maintain backward compatibility. This one block makes it so an
            // empty std::string_view whose .data() happens to be nullptr behaves
            // *differently* from an otherwise empty std::string_view whose .data() is
            // not nullptr. This is an undesirable difference in general, but this
            // behavior is maintained to avoid breaking existing code that happens to
            // depend on this old behavior/bug. Perhaps it will be fixed one day. The
            // difference in behavior is as follows:
            //   Split(std::string_view(""), '-');  // {""}
            //   Split(std::string_view(), '-');    // {}
            if (splitter_->text().data() == nullptr) {
                state_ = kEndState;
                pos_ = splitter_->text().size();
                return;
            }

            if (state_ == kEndState) {
                pos_ = splitter_->text().size();
            } else {
                ++(*this);
            }
        }

        bool at_end() const { return state_ == kEndState; }

        reference operator*() const { return curr_; }

        pointer operator->() const { return &curr_; }

        split_iterator &operator++() {
            do {
                if (state_ == kLastState) {
                    state_ = kEndState;
                    return *this;
                }
                const std::string_view text = splitter_->text();
                const std::string_view d = delimiter_.find(text, pos_);
                if (d.data() == text.data() + text.size()) state_ = kLastState;
                curr_ = text.substr(pos_, d.data() - (text.data() + pos_));
                pos_ += curr_.size() + d.size();
            } while (!predicate_(curr_));
            return *this;
        }

        split_iterator operator++(int) {
            split_iterator old(*this);
            ++(*this);
            return old;
        }

        friend bool operator==(const split_iterator &a, const split_iterator &b) {
            return a.state_ == b.state_ && a.pos_ == b.pos_;
        }

        friend bool operator!=(const split_iterator &a, const split_iterator &b) {
            return !(a == b);
        }

    private:
        size_t pos_;
        State state_;
        std::string_view curr_;
        const Splitter *splitter_;
        typename Splitter::DelimiterType delimiter_;
        typename Splitter::PredicateType predicate_;
    };

    // A splitter_is_convertible_to<C>::type alias exists iff the specified condition
    // is true for type 'C'.
    //
    // Restricts conversion to container-like types (by testing for the presence of
    // a const_iterator member type) and also to disable conversion to an
    // std::initializer_list (which also has a const_iterator). Otherwise, code
    // compiled in C++11 will get an error due to ambiguous conversion paths (in
    // C++11 std::vector<T>::operator= is overloaded to take either a std::vector<T>
    // or an std::initializer_list<T>).

    template<typename C, bool has_value_type, bool has_mapped_type>
    struct splitter_is_convertible_to_impl : std::false_type {
    };

    template<typename C>
    struct splitter_is_convertible_to_impl<C, true, false>
            : std::is_constructible<typename C::value_type, std::string_view> {
    };

    template<typename C>
    struct splitter_is_convertible_to_impl<C, true, true>
            : std::conjunction<
                    std::is_constructible<typename C::key_type, std::string_view>,
                    std::is_constructible<typename C::mapped_type, std::string_view>> {
    };

    // is_initializer_list<T>::value is true iff T is an std::initializer_list. More
    // details below in Splitter<> where this is used.
    std::false_type is_initializer_list_dispatch(...);  // default: No
    template<typename T>
    std::true_type is_initializer_list_dispatch(std::initializer_list<T> *);

    template<typename T>
    struct is_initializer_list
            : decltype(is_initializer_list_dispatch(static_cast<T *>(nullptr))) {
    };

    template<typename C>
    struct splitter_is_convertible_to
            : splitter_is_convertible_to_impl<C,
                    !is_initializer_list<typename std::remove_reference<C>::type>::value &&
                    flare::has_value_type<C>::value && flare::has_const_iterator<C>::value,
                    flare::has_mapped_type<C>::value> {
    };

    // This class implements the range that is returned by flare:: string_split(). This
    // class has templated conversion operators that allow it to be implicitly
    // converted to a variety of types that the caller may have specified on the
    // left-hand side of an assignment.
    //
    // The main interface for interacting with this class is through its implicit
    // conversion operators. However, this class may also be used like a container
    // in that it has .begin() and .end() member functions. It may also be used
    // within a range-for loop.
    //
    // Output containers can be collections of any type that is constructible from
    // an std::string_view.
    //
    // An Predicate functor may be supplied. This predicate will be used to filter
    // the split strings: only strings for which the predicate returns true will be
    // kept. A Predicate object is any unary functor that takes an std::string_view
    // and returns bool.
    template<typename Delimiter, typename Predicate>
    class splitter {
    public:
        using DelimiterType = Delimiter;
        using PredicateType = Predicate;
        using const_iterator = strings_internal::split_iterator<splitter>;
        using value_type = typename std::iterator_traits<const_iterator>::value_type;

        splitter(convertible_to_string_view input_text, Delimiter d, Predicate p)
                : text_(std::move(input_text)),
                  delimiter_(std::move(d)),
                  predicate_(std::move(p)) {}

        std::string_view text() const { return text_.value(); }

        const Delimiter &delimiter() const { return delimiter_; }

        const Predicate &predicate() const { return predicate_; }

        // Range functions that iterate the split substrings as std::string_view
        // objects. These methods enable a Splitter to be used in a range-based for
        // loop.
        const_iterator begin() const { return {const_iterator::kInitState, this}; }

        const_iterator end() const { return {const_iterator::kEndState, this}; }

        // An implicit conversion operator that is restricted to only those containers
        // that the splitter is convertible to.
        template<typename Container,
                typename = typename std::enable_if<
                        splitter_is_convertible_to<Container>::value>::type>
        operator Container() const {  // NOLINT(runtime/explicit)
            return convert_to_container<Container, typename Container::value_type,
                    has_mapped_type<Container>::value>()(*this);
        }

        // Returns a pair with its .first and .second members set to the first two
        // strings returned by the begin() iterator. Either/both of .first and .second
        // will be constructed with empty strings if the iterator doesn't have a
        // corresponding value.
        template<typename First, typename Second>
        operator std::pair<First, Second>() const {  // NOLINT(runtime/explicit)
            std::string_view first, second;
            auto it = begin();
            if (it != end()) {
                first = *it;
                if (++it != end()) {
                    second = *it;
                }
            }
            return {First(first), Second(second)};
        }

    private:

        // convert_to_container is a functor converting a Splitter to the requested
        // Container of ValueType. It is specialized below to optimize splitting to
        // certain combinations of Container and ValueType.
        //
        // This base template handles the generic case of storing the split results in
        // the requested non-map-like container and converting the split substrings to
        // the requested type.
        template<typename Container, typename ValueType, bool is_map = false>
        struct convert_to_container {
            Container operator()(const splitter &splitter) const {
                Container c;
                auto it = std::inserter(c, c.end());
                for (const auto &sp : splitter) {
                    *it++ = ValueType(sp);
                }
                return c;
            }
        };

        // Partial specialization for a std::vector<std::string_view>.
        //
        // Optimized for the common case of splitting to a
        // std::vector<std::string_view>. In this case we first split the results to
        // a small array of std::string_view on the stack, to reduce reallocations.
        template<typename A>
        struct convert_to_container<std::vector<std::string_view, A>,
                std::string_view, false> {
            std::vector<std::string_view, A> operator()(
                    const splitter &splitter) const {
                struct raw_view {
                    const char *data;
                    size_t size;

                    operator std::string_view() const {  // NOLINT(runtime/explicit)
                        return {data, size};
                    }
                };
                std::vector<std::string_view, A> v;
                std::array<raw_view, 16> ar;
                for (auto it = splitter.begin(); !it.at_end();) {
                    size_t index = 0;
                    do {
                        ar[index].data = it->data();
                        ar[index].size = it->size();
                        ++it;
                    } while (++index != ar.size() && !it.at_end());
                    v.insert(v.end(), ar.begin(), ar.begin() + index);
                }
                return v;
            }
        };

        // Partial specialization for a std::vector<std::string>.
        //
        // Optimized for the common case of splitting to a std::vector<std::string>.
        // In this case we first split the results to a std::vector<std::string_view>
        // so the returned std::vector<std::string> can have space reserved to avoid
        // std::string moves.
        template<typename A>
        struct convert_to_container<std::vector<std::string, A>, std::string, false> {
            std::vector<std::string, A> operator()(const splitter &splitter) const {
                const std::vector<std::string_view> v = splitter;
                return std::vector<std::string, A>(v.begin(), v.end());
            }
        };

        // Partial specialization for containers of pairs (e.g., maps).
        //
        // The algorithm is to insert a new pair into the map for each even-numbered
        // item, with the even-numbered item as the key with a default-constructed
        // value. Each odd-numbered item will then be assigned to the last pair's
        // value.
        template<typename Container, typename First, typename Second>
        struct convert_to_container<Container, std::pair<const First, Second>, true> {
            Container operator()(const splitter &splitter) const {
                Container m;
                typename Container::iterator it;
                bool insert = true;
                for (const auto &sp : splitter) {
                    if (insert) {
                        it = inserter<Container>::Insert(&m, First(sp), Second());
                    } else {
                        it->second = Second(sp);
                    }
                    insert = !insert;
                }
                return m;
            }

            // Inserts the key and value into the given map, returning an iterator to
            // the inserted item. Specialized for std::map and std::multimap to use
            // emplace() and adapt emplace()'s return value.
            template<typename Map>
            struct inserter {
                using M = Map;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->insert(std::make_pair(std::forward<Args>(args)...)).first;
                }
            };

            template<typename... Ts>
            struct inserter<std::map<Ts...>> {
                using M = std::map<Ts...>;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->emplace(std::make_pair(std::forward<Args>(args)...)).first;
                }
            };

            template<typename... Ts>
            struct inserter<std::multimap<Ts...>> {
                using M = std::multimap<Ts...>;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->emplace(std::make_pair(std::forward<Args>(args)...));
                }
            };
        };

        convertible_to_string_view text_;
        Delimiter delimiter_;
        Predicate predicate_;
    };

}  // namespace flare::strings_internal

#endif  // FLARE_STRINGS_INTERNAL_STR_SPLIT_INTERNAL_H_
