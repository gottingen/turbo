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
#ifndef TURBO_META_OPTIONAL_SEQUENCE_H_
#define TURBO_META_OPTIONAL_SEQUENCE_H_

#include <array>
#include <bitset>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "turbo/meta/iterator_base.h"
#include "turbo/meta/optional_ref.h"
#include "turbo/meta/sequence.h"
#include "turbo/container/dynamic_bitset.h"

namespace turbo {
    /**************************************
     * Optimized 1-D optional_ref containers *
     **************************************/

    template<class ITV, class ITB>
    class optional_iterator;

    template<class BC, class FC>
    class optional_sequence {
    public:

        // Internal typedefs

        using base_container_type = BC;
        using base_value_type = typename base_container_type::value_type;
        using base_reference = typename base_container_type::reference;
        using base_const_reference = typename base_container_type::const_reference;

        using flag_container_type = FC;
        using flag_type = typename flag_container_type::value_type;
        using flag_reference = typename flag_container_type::reference;
        using flag_const_reference = typename flag_container_type::const_reference;

        // Container typedefs
        using value_type = optional_ref<base_value_type, flag_type>;
        using reference = optional_ref<base_reference, flag_reference>;
        using const_reference = optional_ref<base_const_reference, flag_const_reference>;
        using pointer = closure_pointer<reference>;
        using const_pointer = closure_pointer<const_reference>;

        // Other typedefs
        using size_type = typename base_container_type::size_type;
        using difference_type = typename base_container_type::difference_type;
        using iterator = optional_iterator<typename base_container_type::iterator,
                typename flag_container_type::iterator>;
        using const_iterator = optional_iterator<typename base_container_type::const_iterator,
                typename flag_container_type::const_iterator>;

        using reverse_iterator = optional_iterator<typename base_container_type::reverse_iterator,
                typename flag_container_type::reverse_iterator>;
        using const_reverse_iterator = optional_iterator<typename base_container_type::const_reverse_iterator,
                typename flag_container_type::const_reverse_iterator>;

        bool empty() const noexcept;

        size_type size() const noexcept;

        size_type max_size() const noexcept;

        reference at(size_type i);

        const_reference at(size_type i) const;

        reference operator[](size_type i);

        const_reference operator[](size_type i) const;

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        iterator begin() noexcept;

        iterator end() noexcept;

        const_iterator begin() const noexcept;

        const_iterator end() const noexcept;

        const_iterator cbegin() const noexcept;

        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;

        reverse_iterator rend() noexcept;

        const_reverse_iterator rbegin() const noexcept;

        const_reverse_iterator rend() const noexcept;

        const_reverse_iterator crbegin() const noexcept;

        const_reverse_iterator crend() const noexcept;

        base_container_type value() && noexcept;

        base_container_type &value() & noexcept;

        const base_container_type &value() const & noexcept;

        flag_container_type has_value() && noexcept;

        flag_container_type &has_value() & noexcept;

        const flag_container_type &has_value() const & noexcept;

    protected:

        optional_sequence() = default;

        optional_sequence(size_type s, const base_value_type &v);

        template<class CTO, class CBO>
        optional_sequence(size_type s, const optional_ref<CTO, CBO> &v);

        ~optional_sequence() = default;

        optional_sequence(const optional_sequence &) = default;

        optional_sequence &operator=(const optional_sequence &) = default;

        optional_sequence(optional_sequence &&) = default;

        optional_sequence &operator=(optional_sequence &&) = default;

        base_container_type m_values;
        flag_container_type m_flags;
    };

    template<class BC, class FC>
    bool operator==(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    template<class BC, class FC>
    bool operator!=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    template<class BC, class FC>
    bool operator<(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    template<class BC, class FC>
    bool operator<=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    template<class BC, class FC>
    bool operator>(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    template<class BC, class FC>
    bool operator>=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs);

    /********************************
     * optional_array declarations *
     ********************************/

    // There is no value_type in std::bitset ...
    template<class T, std::size_t I, class BC = turbo::dynamic_bitset<std::size_t>>
    class optional_array : public optional_sequence<std::array<T, I>, BC> {
    public:

        using self_type = optional_array;
        using base_container_type = std::array<T, I>;
        using flag_container_type = BC;
        using base_type = optional_sequence<base_container_type, flag_container_type>;
        using base_value_type = typename base_type::base_value_type;
        using size_type = typename base_type::size_type;

        optional_array() = default;

        optional_array(size_type s, const base_value_type &v);

        template<class CTO, class CBO>
        optional_array(size_type s, const optional_ref<CTO, CBO> &v);
    };

    /********************
     * optional_vector *
     ********************/

    template<class T, class A = std::allocator<T>, class BC = turbo::dynamic_bitset<std::size_t>>
    class optional_vector : public optional_sequence<std::vector<T, A>, BC> {
    public:

        using self_type = optional_vector;
        using base_container_type = std::vector<T, A>;
        using flag_container_type = BC;
        using base_type = optional_sequence<base_container_type, flag_container_type>;
        using base_value_type = typename base_type::base_value_type;
        using allocator_type = A;

        using value_type = typename base_type::value_type;
        using size_type = typename base_type::size_type;
        using difference_type = typename base_type::difference_type;
        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;
        using pointer = typename base_type::pointer;
        using const_pointer = typename base_type::const_pointer;

        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;
        using reverse_iterator = typename base_type::reverse_iterator;
        using const_reverse_iterator = typename base_type::const_reverse_iterator;

        optional_vector() = default;

        optional_vector(size_type, const base_value_type &);

        template<class CTO, class CBO>
        optional_vector(size_type, const optional_ref<CTO, CBO> &);

        void resize(size_type);

        void resize(size_type, const base_value_type &);

        template<class CTO, class CBO>
        void resize(size_type, const optional_ref<CTO, CBO> &);
    };

    /**********************************
     * optional_iterator declaration *
     **********************************/

    template<class ITV, class ITB>
    struct optional_iterator_traits {
        using iterator_type = optional_iterator<ITV, ITB>;
        using value_type = optional_ref<typename ITV::value_type, typename ITB::value_type>;
        using reference = optional_ref<typename ITV::reference, typename ITB::reference>;
        using pointer = closure_pointer<reference>;
        using difference_type = typename ITV::difference_type;
    };

    template<class ITV, class ITB>
    class optional_iterator : public random_access_iterator_base2<optional_iterator_traits<ITV, ITB>> {
    public:

        using self_type = optional_iterator<ITV, ITB>;
        using base_type = random_access_iterator_base2<optional_iterator_traits<ITV, ITB>>;

        using value_type = typename base_type::value_type;
        using reference = typename base_type::reference;
        using pointer = typename base_type::pointer;
        using difference_type = typename base_type::difference_type;

        optional_iterator() = default;

        optional_iterator(ITV itv, ITB itb);

        self_type &operator++();

        self_type &operator--();

        self_type &operator+=(difference_type n);

        self_type &operator-=(difference_type n);

        difference_type operator-(const self_type &rhs) const;

        reference operator*() const;

        pointer operator->() const;

        bool operator==(const self_type &rhs) const;

        bool operator<(const self_type &rhs) const;

    private:

        ITV m_itv;
        ITB m_itb;
    };

    /*************************************
     * optional_sequence implementation *
     *************************************/

    template<class BC, class FC>
    inline optional_sequence<BC, FC>::optional_sequence(size_type s, const base_value_type &v)
            : m_values(make_sequence<base_container_type>(s, v)),
              m_flags(make_sequence<flag_container_type>(s, true)) {
    }

    template<class BC, class FC>
    template<class CTO, class CBO>
    inline optional_sequence<BC, FC>::optional_sequence(size_type s, const optional_ref<CTO, CBO> &v)
            : m_values(make_sequence<base_container_type>(s, v.value())),
              m_flags(make_sequence<flag_container_type>(s, v.has_value())) {
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::empty() const noexcept -> bool {
        return m_values.empty();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::size() const noexcept -> size_type {
        return m_values.size();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::max_size() const noexcept -> size_type {
        return m_values.max_size();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::at(size_type i) -> reference {
        return reference(m_values.at(i), m_flags.at(i));
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::at(size_type i) const -> const_reference {
        return const_reference(m_values.at(i), m_flags.at(i));
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::operator[](size_type i) -> reference {
        return reference(m_values[i], m_flags[i]);
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::operator[](size_type i) const -> const_reference {
        return const_reference(m_values[i], m_flags[i]);
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::front() -> reference {
        return reference(m_values.front(), m_flags.front());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::front() const -> const_reference {
        return const_reference(m_values.front(), m_flags.front());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::back() -> reference {
        return reference(m_values.back(), m_flags.back());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::back() const -> const_reference {
        return const_reference(m_values.back(), m_flags.back());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::begin() noexcept -> iterator {
        return iterator(m_values.begin(), m_flags.begin());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::end() noexcept -> iterator {
        return iterator(m_values.end(), m_flags.end());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::begin() const noexcept -> const_iterator {
        return cbegin();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::end() const noexcept -> const_iterator {
        return cend();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::cbegin() const noexcept -> const_iterator {
        return const_iterator(m_values.cbegin(), m_flags.cbegin());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::cend() const noexcept -> const_iterator {
        return const_iterator(m_values.cend(), m_flags.cend());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::rbegin() noexcept -> reverse_iterator {
        return reverse_iterator(m_values.rbegin(), m_flags.rbegin());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::rend() noexcept -> reverse_iterator {
        return reverse_iterator(m_values.rend(), m_flags.rend());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::rbegin() const noexcept -> const_reverse_iterator {
        return crbegin();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::rend() const noexcept -> const_reverse_iterator {
        return crend();
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(m_values.crbegin(), m_flags.crbegin());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(m_values.crend(), m_flags.crend());
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::value() && noexcept -> base_container_type {
        return m_values;
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::value() & noexcept -> base_container_type & {
        return m_values;
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::value() const & noexcept -> const base_container_type & {
        return m_values;
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::has_value() && noexcept -> flag_container_type {
        return m_flags;
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::has_value() & noexcept -> flag_container_type & {
        return m_flags;
    }

    template<class BC, class FC>
    inline auto optional_sequence<BC, FC>::has_value() const & noexcept -> const flag_container_type & {
        return m_flags;
    }

    template<class BC, class FC>
    inline bool operator==(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return lhs.value() == rhs.value() && lhs.has_value() == rhs.has_value();
    }

    template<class BC, class FC>
    inline bool operator!=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return !(lhs == rhs);
    }

    template<class BC, class FC>
    inline bool operator<(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return lhs.value() < rhs.value() && lhs.has_value() == rhs.has_value();
    }

    template<class BC, class FC>
    inline bool operator<=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return lhs.value() <= rhs.value() && lhs.has_value() == rhs.has_value();
    }

    template<class BC, class FC>
    inline bool operator>(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return lhs.value() > rhs.value() && lhs.has_value() == rhs.has_value();
    }

    template<class BC, class FC>
    inline bool operator>=(const optional_sequence<BC, FC> &lhs, const optional_sequence<BC, FC> &rhs) {
        return lhs.value() >= rhs.value() && lhs.has_value() == rhs.has_value();
    }

    /**********************************
     * optional_array implementation *
     **********************************/

    template<class T, std::size_t I, class BC>
    optional_array<T, I, BC>::optional_array(size_type s, const base_value_type &v)
            : base_type(s, v) {
    }

    template<class T, std::size_t I, class BC>
    template<class CTO, class CBO>
    optional_array<T, I, BC>::optional_array(size_type s, const optional_ref<CTO, CBO> &v)
            : base_type(s, v) {
    }

    /*******************************************************
     * optional_array and optional_vector implementation *
     *******************************************************/

    template<class T, class A, class BC>
    optional_vector<T, A, BC>::optional_vector(size_type s, const base_value_type &v)
            : base_type(s, v) {
    }

    template<class T, class A, class BC>
    template<class CTO, class CBO>
    optional_vector<T, A, BC>::optional_vector(size_type s, const optional_ref<CTO, CBO> &v)
            : base_type(s, v) {
    }

    template<class T, class A, class BC>
    void optional_vector<T, A, BC>::resize(size_type s) {
        // Default to missing
        this->m_values.resize(s);
        this->m_flags.resize(s, false);
    }

    template<class T, class A, class BC>
    void optional_vector<T, A, BC>::resize(size_type s, const base_value_type &v) {
        this->m_values.resize(s, v);
        this->m_flags.resize(s, true);
    }

    template<class T, class A, class BC>
    template<class CTO, class CBO>
    void optional_vector<T, A, BC>::resize(size_type s, const optional_ref<CTO, CBO> &v) {
        this->m_values.resize(s, v.value());
        this->m_flags.resize(s, v.has_value());
    }

    /*************************************
     * optional_iterator implementation *
     *************************************/

    template<class ITV, class ITB>
    optional_iterator<ITV, ITB>::optional_iterator(ITV itv, ITB itb)
            : m_itv(itv), m_itb(itb) {
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator++() -> self_type & {
        ++m_itv;
        ++m_itb;
        return *this;
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator--() -> self_type & {
        --m_itv;
        --m_itb;
        return *this;
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator+=(difference_type n) -> self_type & {
        m_itv += n;
        m_itb += n;
        return *this;
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator-=(difference_type n) -> self_type & {
        m_itv -= n;
        m_itb -= n;
        return *this;
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator-(const self_type &rhs) const -> difference_type {
        return m_itv - rhs.m_itv;
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator*() const -> reference {
        return reference(*m_itv, *m_itb);
    }

    template<class ITV, class ITB>
    auto optional_iterator<ITV, ITB>::operator->() const -> pointer {
        return pointer(operator*());
    }

    template<class ITV, class ITB>
    bool optional_iterator<ITV, ITB>::operator==(const self_type &rhs) const {
        return m_itv == rhs.m_itv && m_itb == rhs.m_itb;
    }

    template<class ITV, class ITB>
    bool optional_iterator<ITV, ITB>::operator<(const self_type &rhs) const {
        return m_itv < rhs.m_itv && m_itb < rhs.m_itb;
    }
}

#endif  // TURBO_META_OPTIONAL_SEQUENCE_H_

