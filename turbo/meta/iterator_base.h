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
#ifndef TURBO_META_XITERATOR_BASE_H_
#define TURBO_META_XITERATOR_BASE_H_

#include <cstddef>
#include <iterator>

namespace turbo {
    /**************************************
     * class bidirectional_iterator_base *
     **************************************/

    template<class I, class T, class D = std::ptrdiff_t, class P = T *, class R = T &>
    class bidirectional_iterator_base {
    public:

        using derived_type = I;
        using value_type = T;
        using reference = R;
        using pointer = P;
        using difference_type = D;
        using iterator_category = std::bidirectional_iterator_tag;

        inline friend derived_type operator++(derived_type &d, int) {
            derived_type tmp(d);
            ++d;
            return tmp;
        }

        inline friend derived_type operator--(derived_type &d, int) {
            derived_type tmp(d);
            --d;
            return tmp;

        }

        inline friend bool operator!=(const derived_type &lhs, const derived_type &rhs) {
            return !(lhs == rhs);
        }
    };

    template<class T>
    using bidirectional_iterator_base2 = bidirectional_iterator_base<typename T::iterator_type,
            typename T::value_type,
            typename T::difference_type,
            typename T::pointer,
            typename T::reference>;

    template<class I, class T>
    using bidirectional_iterator_base3 = bidirectional_iterator_base<I,
            typename T::value_type,
            typename T::difference_type,
            typename T::pointer,
            typename T::reference>;

    /********************************
     * random_access_iterator_base *
     ********************************/

    template<class I, class T, class D = std::ptrdiff_t, class P = T *, class R = T &>
    class random_access_iterator_base : public bidirectional_iterator_base<I, T, D, P, R> {
    public:

        using derived_type = I;
        using value_type = T;
        using reference = R;
        using pointer = P;
        using difference_type = D;
        using iterator_category = std::random_access_iterator_tag;

        inline reference operator[](difference_type n) const {
            return *(*static_cast<const derived_type *>(this) + n);
        }

        inline friend derived_type operator+(const derived_type &it, difference_type n) {
            derived_type tmp(it);
            return tmp += n;
        }

        inline friend derived_type operator+(difference_type n, const derived_type &it) {
            derived_type tmp(it);
            return tmp += n;
        }

        inline friend derived_type operator-(const derived_type &it, difference_type n) {
            derived_type tmp(it);
            return tmp -= n;
        }

        inline friend bool operator<=(const derived_type &lhs, const derived_type &rhs) {
            return !(rhs < lhs);
        }

        inline friend bool operator>=(const derived_type &lhs, const derived_type &rhs) {
            return !(lhs < rhs);
        }

        inline friend bool operator>(const derived_type &lhs, const derived_type &rhs) {
            return rhs < lhs;
        }

    };

    template<class T>
    using random_access_iterator_base2 = random_access_iterator_base<typename T::iterator_type,
            typename T::value_type,
            typename T::difference_type,
            typename T::pointer,
            typename T::reference>;

    template<class I, class T>
    using random_access_iterator_base3 = random_access_iterator_base<I,
            typename T::value_type,
            typename T::difference_type,
            typename T::pointer,
            typename T::reference>;

    /*******************************
     * random_access_iterator_ext *
     *******************************/

    // Extension for random access iterators defining operator[] and operator+ overloads
    // accepting size_t arguments.
    template<class I, class R>
    class random_access_iterator_ext {
    public:

        using derived_type = I;
        using reference = R;
        using size_type = std::size_t;

        inline reference operator[](size_type n) const {
            return *(*static_cast<const derived_type *>(this) + n);
        }

        inline friend derived_type operator+(const derived_type &it, size_type n) {
            derived_type tmp(it);
            return tmp += n;
        }

        inline friend derived_type operator+(size_type n, const derived_type &it) {
            derived_type tmp(it);
            return tmp += n;
        }

        inline friend derived_type operator-(const derived_type &it, size_type n) {
            derived_type tmp(it);
            return tmp -= n;
        }
    };

    /*****************
     * key_iterator *
     *****************/

    template<class M>
    class key_iterator : public bidirectional_iterator_base<key_iterator<M>, const typename M::key_type> {
    public:

        using self_type = key_iterator;
        using base_type = bidirectional_iterator_base<self_type, const typename M::key_type>;
        using value_type = typename base_type::value_type;
        using reference = typename base_type::reference;
        using pointer = typename base_type::pointer;
        using difference_type = typename base_type::difference_type;
        using iterator_category = typename base_type::iterator_category;
        using subiterator = typename M::const_iterator;

        inline key_iterator(subiterator it) noexcept
                : m_it(it) {
        }

        inline self_type &operator++() {
            ++m_it;
            return *this;
        }

        inline self_type &operator--() {
            --m_it;
            return *this;
        }

        inline reference operator*() const {
            return m_it->first;
        }

        inline pointer operator->() const {
            return &(m_it->first);
        }

        inline bool operator==(const self_type &rhs) const {
            return m_it == rhs.m_it;
        }

    private:

        subiterator m_it;
    };

    /*******************
     * value_iterator *
     *******************/

    namespace detail {
        template<class M>
        struct value_iterator_types {
            using subiterator = typename M::iterator;
            using value_type = typename M::mapped_type;
            using reference = value_type &;
            using pointer = value_type *;
            using difference_type = typename subiterator::difference_type;
        };

        template<class M>
        struct value_iterator_types<const M> {
            using subiterator = typename M::const_iterator;
            using value_type = typename M::mapped_type;
            using reference = const value_type &;
            using pointer = const value_type *;
            using difference_type = typename subiterator::difference_type;
        };
    }

    template<class M>
    class value_iterator : bidirectional_iterator_base3<value_iterator<M>,
            detail::value_iterator_types<M>> {
    public:

        using self_type = value_iterator<M>;
        using base_type = bidirectional_iterator_base3<self_type, detail::value_iterator_types<M>>;
        using value_type = typename base_type::value_type;
        using reference = typename base_type::reference;
        using pointer = typename base_type::pointer;
        using difference_type = typename base_type::difference_type;
        using subiterator = typename detail::value_iterator_types<M>::subiterator;

        inline value_iterator(subiterator it) noexcept
                : m_it(it) {
        }

        inline self_type &operator++() {
            ++m_it;
            return *this;
        }

        inline self_type &operator--() {
            --m_it;
            return *this;
        }

        inline reference operator*() const {
            return m_it->second;
        }

        inline pointer operator->() const {
            return &(m_it->second);
        }

        inline bool operator==(const self_type &rhs) const {
            return m_it == rhs.m_it;
        }

    private:

        subiterator m_it;
    };

    /**********************
     * stepping_iterator *
     **********************/

    template<class It>
    class stepping_iterator : public random_access_iterator_base3<stepping_iterator<It>,
            std::iterator_traits<It>> {
    public:

        using self_type = stepping_iterator;
        using base_type = random_access_iterator_base3<self_type, std::iterator_traits<It>>;
        using value_type = typename base_type::value_type;
        using reference = typename base_type::reference;
        using pointer = typename base_type::pointer;
        using difference_type = typename base_type::difference_type;
        using iterator_category = typename base_type::iterator_category;
        using subiterator = It;

        stepping_iterator() = default;

        inline stepping_iterator(subiterator it, difference_type step) noexcept
                : m_it(it), m_step(step) {
        }

        inline self_type &operator++() {
            std::advance(m_it, m_step);
            return *this;
        }

        inline self_type &operator--() {
            std::advance(m_it, -m_step);
            return *this;
        }

        inline self_type &operator+=(difference_type n) {
            std::advance(m_it, n * m_step);
            return *this;
        }

        inline self_type &operator-=(difference_type n) {
            std::advance(m_it, -n * m_step);
            return *this;
        }

        inline difference_type operator-(const self_type &rhs) const {
            return std::distance(rhs.m_it, m_it) / m_step;
        }

        inline reference operator*() const {
            return *m_it;
        }

        inline pointer operator->() const {
            return m_it;
        }

        inline bool equal(const self_type &rhs) const {
            return m_it == rhs.m_it && m_step == rhs.m_step;
        }

        inline bool less_than(const self_type &rhs) const {
            return m_it < rhs.m_it && m_step == rhs.m_step;
        }

    private:

        subiterator m_it;
        difference_type m_step;
    };

    template<class It>
    inline bool operator==(const stepping_iterator<It> &lhs, const stepping_iterator<It> &rhs) {
        return lhs.equal(rhs);
    }

    template<class It>
    inline bool operator<(const stepping_iterator<It> &lhs, const stepping_iterator<It> &rhs) {
        return lhs.less_than(rhs);
    }

    template<class It>
    inline stepping_iterator<It>
    make_stepping_iterator(It it, typename std::iterator_traits<It>::difference_type step) {
        return stepping_iterator<It>(it, step);
    }

    /***********************
     * common_iterator_tag *
     ***********************/

    template<class... Its>
    struct common_iterator_tag : std::common_type<typename std::iterator_traits<Its>::iterator_category...> {
    };

    template<class... Its>
    using common_iterator_tag_t = typename common_iterator_tag<Its...>::type;
}

#endif  // TURBO_META_XITERATOR_BASE_H_

