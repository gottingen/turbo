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

#ifndef TURBO_CONTAINER_DYNAMIC_BITSET_H_
#define TURBO_CONTAINER_DYNAMIC_BITSET_H_

#include <memory>
#include <vector>
#include <algorithm>
#include <string>
#include <string_view>
#include <functional>
#include <type_traits>
#include <limits>
#include <cmath>
#include <cassert>
#include "turbo/base/bits.h"
#include "turbo/meta/span.h"
#include "turbo/meta/closure.h"
#include "turbo/meta/iterator_base.h"

namespace turbo {

    template<class B, bool is_const>
    class bitset_reference;

    template<class B, bool is_const>
    class bitset_iterator;

    /******************
     * dyamic_bitset *
     ******************/

    template<class B>
    class dynamic_bitset_base;

    template<class B, class A>
    class dynamic_bitset;

    template<class X>
    class dynamic_bitset_view;

    template<class X>
    struct dynamic_bitset_traits;

    template<class B, class A>
    struct dynamic_bitset_traits<dynamic_bitset<B, A>> {
        using storage_type = std::vector<B, A>;
        using block_type = typename storage_type::value_type;
    };

    template<class X>
    struct dynamic_bitset_traits<dynamic_bitset_view<X>> {
        using storage_type = turbo::Span<X>;
        using block_type = typename storage_type::value_type;
    };

    template<class X>
    struct container_internals;

    template<class X>
    struct container_internals<turbo::Span<X>> {
        using value_type = typename turbo::Span<X>::value_type;
        static_assert(std::is_scalar<value_type>::value, "");
        using allocator_type = std::allocator<value_type>;
        using size_type = std::size_t;
        using difference_type = typename turbo::Span<X>::difference_type;
    };

    template<class X, class A>
    struct container_internals<std::vector<X, A>> {
        using value_type = X;
        static_assert(std::is_scalar<value_type>::value, "");
        using allocator_type = A;
        using size_type = typename std::vector<X>::size_type;
        using difference_type = typename std::vector<X>::difference_type;
    };

    template<class B>
    class dynamic_bitset_base {
    public:

        using self_type = dynamic_bitset_base<B>;
        using derived_class = B;

        using storage_type = typename dynamic_bitset_traits<B>::storage_type;
        using block_type = typename dynamic_bitset_traits<B>::block_type;
        using temporary_type = dynamic_bitset<block_type, std::allocator<block_type>>;

        using allocator_type = typename container_internals<storage_type>::allocator_type;
        using value_type = bool;
        using reference = bitset_reference<derived_class, false>;
        using const_reference = bitset_reference<derived_class, true>;

        using pointer = typename reference::pointer;
        using const_pointer = typename const_reference::pointer;
        using size_type = typename container_internals<storage_type>::size_type;
        using difference_type = typename storage_type::difference_type;
        using iterator = bitset_iterator<derived_class, false>;
        using const_iterator = bitset_iterator<derived_class, true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using const_block_iterator = typename storage_type::const_iterator;

        bool empty() const noexcept;

        size_type size() const noexcept;

        void swap(self_type &rhs);

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

        const_block_iterator block_begin() const noexcept;

        const_block_iterator block_end() const noexcept;

        template<class R>
        self_type &operator&=(const dynamic_bitset_base<R> &rhs);

        template<class R>
        self_type &operator|=(const dynamic_bitset_base<R> &rhs);

        template<class R>
        self_type &operator^=(const dynamic_bitset_base<R> &rhs);

        temporary_type operator<<(size_type pos);

        self_type &operator<<=(size_type pos);

        temporary_type operator>>(size_type pos);

        self_type &operator>>=(size_type pos);

        self_type &set();

        self_type &set(size_type pos, value_type value = true);

        self_type &reset();

        self_type &reset(size_type pos);

        self_type &flip();

        self_type &flip(size_type pos);

        bool all() const noexcept;

        bool any() const noexcept;

        bool none() const noexcept;

        size_type count() const noexcept;

        size_type block_count() const noexcept;

        block_type *data() noexcept;

        const block_type *data() const noexcept;

        template<class Y>
        bool operator==(const dynamic_bitset_base<Y> &rhs) const noexcept;

        template<class Y>
        bool operator!=(const dynamic_bitset_base<Y> &rhs) const noexcept;

        derived_class &derived_cast();

        const derived_class &derived_cast() const;

    protected:

        dynamic_bitset_base(const storage_type &buffer, std::size_t size);

        ~dynamic_bitset_base() = default;

        dynamic_bitset_base(const dynamic_bitset_base &rhs) = default;

        dynamic_bitset_base(dynamic_bitset_base &&rhs) = default;

        dynamic_bitset_base &operator=(const dynamic_bitset_base &rhs) = default;

        dynamic_bitset_base &operator=(dynamic_bitset_base &&rhs) = default;

        size_type m_size;
        storage_type m_buffer;

        static constexpr std::size_t s_bits_per_block = CHAR_BIT * sizeof(block_type);

        size_type compute_block_count(size_type bits_count) const noexcept;

        size_type block_index(size_type pos) const noexcept;

        size_type bit_index(size_type pos) const noexcept;

        block_type bit_mask(size_type pos) const noexcept;

        size_type count_extra_bits() const noexcept;

        void zero_unused_bits();

    private:

        // Make views and buffers friends
        template<typename BB>
        friend
        class dynamic_bitset_base;
    };

// NOTE this view ZEROS out remaining bits!
    template<class X>
    class dynamic_bitset_view
            : public dynamic_bitset_base<dynamic_bitset_view<X>> {
    public:

        using base_class = dynamic_bitset_base<dynamic_bitset_view<X>>;
        using storage_type = typename base_class::storage_type;
        using block_type = typename base_class::block_type;

        dynamic_bitset_view(block_type *ptr, std::size_t size);

        dynamic_bitset_view() = default;

        ~dynamic_bitset_view() = default;

        dynamic_bitset_view(const dynamic_bitset_view &rhs) = default;

        dynamic_bitset_view(dynamic_bitset_view &&rhs) = default;

        dynamic_bitset_view &operator=(const dynamic_bitset_view &rhs) = default;

        dynamic_bitset_view &operator=(dynamic_bitset_view &&rhs) = default;

        void resize(std::size_t sz);
    };

    namespace detail_bitset {
        template<class T>
        constexpr T integer_ceil(T n, T div) {
            return (n + div - T(1)) / div;
        }
    }

    template<class X>
    inline dynamic_bitset_view<X>::dynamic_bitset_view(block_type *ptr, std::size_t size)
            : base_class(storage_type(ptr, detail_bitset::integer_ceil(size, base_class::s_bits_per_block)), size) {
        base_class::zero_unused_bits();
    }

    template<class X>
    inline void dynamic_bitset_view<X>::resize(std::size_t sz) {
        if (sz != this->m_size) {
#if defined(XTL_NO_EXCEPTIONS)
            std::fprintf(stderr, "cannot resize bitset_view\n");
        std::terminate();
#else
            throw std::runtime_error("cannot resize bitset_view");
#endif
        }
    }

    template<class B, class A = std::allocator<B>>
    class dynamic_bitset;

    template<class B>
    auto operator~(const dynamic_bitset_base<B> &lhs);

    template<class L, class R>
    auto operator&(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs);

    template<class L, class R>
    auto operator|(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs);

    template<class L, class R>
    auto operator^(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs);

    template<class B>
    void swap(const dynamic_bitset_base<B> &lhs, const dynamic_bitset_base<B> &rhs);

/*********************
 * bitset_reference *
 *********************/

    template<class B, bool is_const>
    class bitset_reference {
    public:

        using self_type = bitset_reference<B, is_const>;
        using pointer = std::conditional_t<is_const,
                const closure_pointer<const self_type>,
                closure_pointer<self_type>>;

        operator bool() const noexcept;

        bitset_reference(const self_type &) = default;

        bitset_reference(self_type &&) = default;

        self_type &operator=(const self_type &) noexcept;

        self_type &operator=(self_type &&) noexcept;

        self_type &operator=(bool) noexcept;

        bool operator~() const noexcept;

        self_type &operator&=(bool) noexcept;

        self_type &operator|=(bool) noexcept;

        self_type &operator^=(bool) noexcept;

        self_type &flip() noexcept;

        pointer operator&() noexcept;

    private:

        using block_type = typename dynamic_bitset_traits<B>::block_type;
        using closure_type = std::conditional_t<is_const, const block_type &, block_type &>;

        bitset_reference(closure_type block, block_type pos);

        void assign(bool) noexcept;

        void set() noexcept;

        void reset() noexcept;

        closure_type m_block;
        const block_type m_mask;

        template<class BO, bool is_const_other>
        friend
        class bitset_reference;

        friend class dynamic_bitset_base<B>;
    };

/********************
 * bitset_iterator *
 ********************/

    template<class B, bool is_const>
    class bitset_iterator : public random_access_iterator_base<bitset_iterator<B, is_const>,
            typename dynamic_bitset_base<B>::value_type,
            typename dynamic_bitset_base<B>::difference_type,
            std::conditional_t<is_const,
                    typename dynamic_bitset_base<B>::const_pointer,
                    typename dynamic_bitset_base<B>::pointer>,
            std::conditional_t<is_const,
                    typename dynamic_bitset_base<B>::const_reference,
                    typename dynamic_bitset_base<B>::reference>> {
    public:

        using self_type = bitset_iterator<B, is_const>;
        using container_type = dynamic_bitset_base<B>;
        using value_type = typename container_type::value_type;
        using reference = std::conditional_t<is_const,
                typename container_type::const_reference,
                typename container_type::reference>;
        using pointer = std::conditional_t<is_const,
                typename container_type::const_pointer,
                typename container_type::pointer>;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;
        using base_type = random_access_iterator_base<self_type, value_type, difference_type, pointer, reference>;

        using container_reference = std::conditional_t<is_const, const container_type &, container_type &>;
        using container_pointer = std::conditional_t<is_const, const container_type *, container_type *>;

        bitset_iterator() noexcept;

        bitset_iterator(container_reference c, size_type index) noexcept;

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

        container_pointer p_container;
        size_type m_index;
    };

    template<class B, class Allocator>
    class dynamic_bitset
            : public dynamic_bitset_base<dynamic_bitset<B, Allocator>> {
    public:

        using allocator_type = Allocator;
        using storage_type = std::vector<B, Allocator>;

        using base_type = dynamic_bitset_base<dynamic_bitset<B, Allocator>>;
        using self_type = dynamic_bitset<B, Allocator>;
        using block_type = B;

        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;

        using pointer = typename reference::pointer;
        using const_pointer = typename const_reference::pointer;
        using size_type = typename storage_type::size_type;
        using difference_type = typename storage_type::difference_type;
        using iterator = bitset_iterator<self_type, false>;
        using const_iterator = bitset_iterator<self_type, true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using base_type::base_type;
        using base_type::begin;
        using base_type::cbegin;
        using base_type::end;
        using base_type::cend;
        using base_type::rbegin;
        using base_type::rend;
        using base_type::size;

        dynamic_bitset();

        explicit dynamic_bitset(const allocator_type &allocator);

        dynamic_bitset(size_type count, bool b, const allocator_type &alloc = allocator_type());

        explicit dynamic_bitset(size_type count, const allocator_type &alloc = allocator_type());

        dynamic_bitset(std::initializer_list<bool> init, const allocator_type &alloc = allocator_type());

        template<class BlockInputIt>
        dynamic_bitset(BlockInputIt first, BlockInputIt last, const allocator_type &alloc = allocator_type());

        dynamic_bitset(const dynamic_bitset &rhs);

        // Allow creation from views for e.g. temporary creation
        template<class Y>
        dynamic_bitset(const dynamic_bitset_base<Y> &rhs);

        ~dynamic_bitset() = default;

        dynamic_bitset(dynamic_bitset &&rhs) = default;

        dynamic_bitset &operator=(const dynamic_bitset &rhs) = default;

        dynamic_bitset &operator=(dynamic_bitset &&rhs) = default;

        void assign(size_type count, bool b);

        template<class BlockInputIt>
        void assign(BlockInputIt first, BlockInputIt last);

        void assign(std::initializer_list<bool> init);

        size_type max_size() const noexcept;

        void reserve(size_type new_cap);

        size_type capacity() const noexcept;

        allocator_type get_allocator() const;

        void resize(size_type size, bool b = false);

        void clear() noexcept;

        void push_back(bool b);

        void pop_back();
    };

/**********************************
 * dynamic_bitset implementation *
 **********************************/

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset()
            : base_type(storage_type(), size_type(0)) {
    }

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset(const allocator_type &allocator)
            : base_type(storage_type(allocator), size_type(0)) {
    }

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset(size_type count, bool b, const allocator_type &alloc)
            : base_type(storage_type(this->compute_block_count(count), b ? ~block_type(0) : block_type(0), alloc),
                        count) {
        this->zero_unused_bits();
    }

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset(size_type count, const allocator_type &alloc)
            : base_type(storage_type(this->compute_block_count(count), block_type(0), alloc), count) {
    }

    template<class B, class A>
    template<class BlockInputIt>
    inline dynamic_bitset<B, A>::dynamic_bitset(BlockInputIt first, BlockInputIt last, const allocator_type &alloc)
            : base_type(storage_type(first, last, alloc),
                        size_type(std::distance(first, last)) * base_type::s_bits_per_block) {
    }

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset(std::initializer_list<bool> init, const allocator_type &alloc)
            : dynamic_bitset(init.size(), alloc) {
        std::copy(init.begin(), init.end(), begin());
    }

    template<class B, class A>
    inline dynamic_bitset<B, A>::dynamic_bitset(const dynamic_bitset &rhs)
            : base_type(storage_type(rhs.block_begin(), rhs.block_end()), rhs.size()) {
    }

    template<class B, class A>
    template<class Y>
    inline dynamic_bitset<B, A>::dynamic_bitset(const dynamic_bitset_base<Y> &rhs)
            : base_type(storage_type(rhs.block_begin(), rhs.block_end()), rhs.size()) {
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::assign(size_type count, bool b) {
        resize(count);
        b ? this->set() : this->reset();
    }

    template<class B, class A>
    template<class BlockInputIt>
    inline void dynamic_bitset<B, A>::assign(BlockInputIt first, BlockInputIt last) {
        resize(size_type(std::distance(first, last)) * base_type::s_bits_per_block);
        std::copy(first, last, this->m_buffer.begin());
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::assign(std::initializer_list<bool> init) {
        resize(init.size());
        std::copy(init.begin(), init.end(), begin());
    }

    template<class B, class A>
    inline auto dynamic_bitset<B, A>::get_allocator() const -> allocator_type {
        return base_type::m_buffer.get_allocator();
    }

    template<class B>
    inline bool dynamic_bitset_base<B>::empty() const noexcept {
        return m_size == 0;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::size() const noexcept -> size_type {
        return m_size;
    }

    template<class B, class A>
    inline auto dynamic_bitset<B, A>::max_size() const noexcept -> size_type {
        return base_type::m_buffer.max_size() * base_type::s_bits_per_block;
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::reserve(size_type new_cap) {
        base_type::m_buffer.reserve(this->compute_block_count(new_cap));
    }

    template<class B, class A>
    inline auto dynamic_bitset<B, A>::capacity() const noexcept -> size_type {
        return base_type::m_buffer.capacity() * base_type::s_bits_per_block;
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::resize(size_type asize, bool b) {
        size_type old_block_count = base_type::block_count();
        size_type new_block_count = base_type::compute_block_count(asize);
        block_type value = b ? ~block_type(0) : block_type(0);

        if (new_block_count != old_block_count) {
            base_type::m_buffer.resize(new_block_count, value);
        }

        if (b && asize > base_type::m_size) {
            size_type extra_bits = base_type::count_extra_bits();
            if (extra_bits > 0) {
                base_type::m_buffer[old_block_count - 1] |= (value << extra_bits);
            }
        }

        base_type::m_size = asize;
        base_type::zero_unused_bits();
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::clear() noexcept {
        base_type::m_buffer.clear();
        base_type::m_size = size_type(0);
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::push_back(bool b) {
        size_type s = size();
        resize(s + 1);
        this->set(s, b);
    }

    template<class B, class A>
    inline void dynamic_bitset<B, A>::pop_back() {
        size_type old_block_count = base_type::m_buffer.size();
        size_type new_block_count = base_type::compute_block_count(base_type::m_size - 1);

        if (new_block_count != old_block_count) {
            base_type::m_buffer.pop_back();
        }

        --base_type::m_size;
        base_type::zero_unused_bits();
    }

    template<class B>
    inline void dynamic_bitset_base<B>::swap(self_type &rhs) {
        using std::swap;
        swap(m_buffer, rhs.m_buffer);
        swap(m_size, rhs.m_size);
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::at(size_type i) -> reference {
        // TODO add real check, remove m_buffer.at ...
        return reference(m_buffer.at(block_index(i)), bit_index(i));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::at(size_type i) const -> const_reference {
        // TODO add real check, remove m_buffer.at ...
        return const_reference(m_buffer.at(block_index(i)), bit_index(i));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator[](size_type i) -> reference {
        return reference(m_buffer[block_index(i)], bit_index(i));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator[](size_type i) const -> const_reference {
        return const_reference(m_buffer[block_index(i)], bit_index(i));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::front() -> reference {
        return (*this)[0];
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::front() const -> const_reference {
        return (*this)[0];
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::back() -> reference {
        return (*this)[m_size - 1];
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::back() const -> const_reference {
        return (*this)[m_size - 1];
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::begin() noexcept -> iterator {
        return iterator(*this, size_type(0));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::end() noexcept -> iterator {
        return iterator(*this, size());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::begin() const noexcept -> const_iterator {
        return cbegin();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::end() const noexcept -> const_iterator {
        return cend();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::cbegin() const noexcept -> const_iterator {
        return const_iterator(*this, size_type(0));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::cend() const noexcept -> const_iterator {
        return const_iterator(*this, size());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::rbegin() noexcept -> reverse_iterator {
        return reverse_iterator(end());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::rend() noexcept -> reverse_iterator {
        return reverse_iterator(begin());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::rbegin() const noexcept -> const_reverse_iterator {
        return crbegin();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::rend() const noexcept -> const_reverse_iterator {
        return crend();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cend());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cbegin());
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::block_begin() const noexcept -> const_block_iterator {
        return m_buffer.begin();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::block_end() const noexcept -> const_block_iterator {
        return m_buffer.end();
    }

    template<class B>
    template<class R>
    inline auto dynamic_bitset_base<B>::operator&=(const dynamic_bitset_base<R> &rhs) -> self_type & {
        size_type size = block_count();
        for (size_type i = 0; i < size; ++i) {
            m_buffer[i] &= rhs.m_buffer[i];
        }
        return *this;
    }

    template<class B>
    template<class R>
    inline auto dynamic_bitset_base<B>::operator|=(const dynamic_bitset_base<R> &rhs) -> self_type & {
        size_type size = block_count();
        for (size_type i = 0; i < size; ++i) {
            m_buffer[i] |= rhs.m_buffer[i];
        }
        return *this;
    }

    template<class B>
    template<class R>
    inline auto dynamic_bitset_base<B>::operator^=(const dynamic_bitset_base<R> &rhs) -> self_type & {
        size_type size = block_count();
        for (size_type i = 0; i < size; ++i) {
            m_buffer[i] ^= rhs.m_buffer[i];
        }
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator<<(size_type pos) -> temporary_type {
        temporary_type tmp(this->derived_cast());
        tmp <<= pos;
        return tmp;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator<<=(size_type pos) -> self_type & {
        if (pos >= m_size) {
            return reset();
        }

        if (pos > 0) {
            size_type last = block_count() - 1;
            size_type div = pos / s_bits_per_block;
            size_type r = bit_index(pos);
            block_type *b = &m_buffer[0];

            if (r != 0) {
                size_type rs = s_bits_per_block - r;
                for (size_type i = last - div; i > 0; --i) {
                    b[i + div] = (b[i] << r) | (b[i - 1] >> rs);
                }
                b[div] = b[0] << r;
            } else {
                for (size_type i = last - div; i > 0; --i) {
                    b[i + div] = b[i];
                }
                b[div] = b[0];
            }

            std::fill_n(m_buffer.begin(), div, block_type(0));
            zero_unused_bits();
        }
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator>>(size_type pos) -> temporary_type {
        temporary_type tmp(this->derived_cast());
        tmp >>= pos;
        return tmp;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::operator>>=(size_type pos) -> self_type & {
        if (pos >= m_size) {
            return reset();
        }

        if (pos > 0) {
            size_type last = block_count() - 1;
            size_type div = pos / s_bits_per_block;
            size_type r = bit_index(pos);
            block_type *b = &m_buffer[0];

            if (r != 0) {
                size_type ls = s_bits_per_block - r;
                for (size_type i = div; i < last; ++i) {
                    b[i - div] = (b[i] >> r) | (b[i + 1] << ls);
                }
                b[last - div] = b[last] >> r;
            } else {
                for (size_type i = div; i <= last; ++i) {
                    b[i - div] = b[i];
                }
            }

            std::fill_n(m_buffer.begin() + static_cast<std::ptrdiff_t>(block_count() - div), div, block_type(0));
        }
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::set() -> self_type & {
        std::fill(m_buffer.begin(), m_buffer.end(), ~block_type(0));
        zero_unused_bits();
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::set(size_type pos, value_type value) -> self_type & {
        if (value) {
            m_buffer[block_index(pos)] |= bit_mask(pos);
        } else {
            reset(pos);
        }
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::reset() -> self_type & {
        std::fill(m_buffer.begin(), m_buffer.end(), block_type(0));
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::reset(size_type pos) -> self_type & {
        m_buffer[block_index(pos)] &= ~bit_mask(pos);
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::flip() -> self_type & {
        size_type size = block_count();
        for (size_type i = 0; i < size; ++i) {
            m_buffer[i] = ~m_buffer[i];
        }
        zero_unused_bits();
        return *this;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::flip(size_type pos) -> self_type & {
        m_buffer[block_index(pos)] ^= bit_mask(pos);
        return *this;
    }

    template<class B>
    inline bool dynamic_bitset_base<B>::all() const noexcept {
        if (empty())
            return true;

        size_type extra_bits = count_extra_bits();
        constexpr block_type all_ones = ~block_type(0);

        size_type size = extra_bits != 0 ? block_count() - 1 : block_count();
        for (size_type i = 0; i < size; ++i) {
            if (m_buffer[i] != all_ones)
                return false;
        }

        if (extra_bits != 0) {
            block_type mask = ~(~block_type(0) << extra_bits);
            if (m_buffer.back() != mask)
                return false;
        }

        return true;
    }

    template<class B>
    inline bool dynamic_bitset_base<B>::any() const noexcept {
        size_type size = block_count();
        for (size_type i = 0; i < size; ++i) {
            if (m_buffer[i])
                return true;
        }
        return false;
    }

    template<class B>
    inline bool dynamic_bitset_base<B>::none() const noexcept {
        return !any();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::count() const noexcept -> size_type {
        static constexpr unsigned char table[] =
                {
                        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
                };
        size_type res = 0;
        const unsigned char *p = static_cast<const unsigned char *>(static_cast<const void *>(&m_buffer[0]));
        size_type length = m_buffer.size() * sizeof(block_type);
        for (size_type i = 0; i < length; ++i, ++p) {
            res += table[*p];
        }
        return res;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::block_count() const noexcept -> size_type {
        return m_buffer.size();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::data() noexcept -> block_type * {
        return m_buffer.data();
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::data() const noexcept -> const block_type * {
        return m_buffer.data();
    }

    template<class B>
    template<class Y>
    inline bool dynamic_bitset_base<B>::operator==(const dynamic_bitset_base<Y> &rhs) const noexcept {
        bool is_equal = m_size == rhs.m_size;
        if (!is_equal) { return false; }

        // we know that block type of lhs & rhs is the same
        auto n_blocks = block_count();

        for (std::size_t i = 0; i < n_blocks; ++i) {
            if (m_buffer[i] != rhs.m_buffer[i]) {
                return false;
            }
        }

        return true;
    }

    template<class B>
    template<class Y>
    inline bool dynamic_bitset_base<B>::operator!=(const dynamic_bitset_base<Y> &rhs) const noexcept {
        return !(*this == rhs);
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::derived_cast() -> derived_class & {
        return *(reinterpret_cast<derived_class *>(this));
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::derived_cast() const -> const derived_class & {
        return *(reinterpret_cast<const derived_class *>(this));
    }

    template<class B>
    inline dynamic_bitset_base<B>::dynamic_bitset_base(const storage_type &buffer, std::size_t size)
            : m_size(size), m_buffer(buffer) {
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::compute_block_count(size_type bits_count) const noexcept -> size_type {
        return bits_count / s_bits_per_block
               + static_cast<size_type>(bits_count % s_bits_per_block != 0);
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::block_index(size_type pos) const noexcept -> size_type {
        return pos / s_bits_per_block;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::bit_index(size_type pos) const noexcept -> size_type {
        return pos % s_bits_per_block;
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::bit_mask(size_type pos) const noexcept -> block_type {
        return block_type(1) << bit_index(pos);
    }

    template<class B>
    inline auto dynamic_bitset_base<B>::count_extra_bits() const noexcept -> size_type {
        return bit_index(size());
    }

    template<class B>
    inline void dynamic_bitset_base<B>::zero_unused_bits() {
        size_type extra_bits = count_extra_bits();
        if (extra_bits != 0) {
            m_buffer.back() &= ~(~block_type(0) << extra_bits);
        }
    }

    template<class B>
    inline auto operator~(const dynamic_bitset_base<B> &lhs) {
        using temporary_type = typename dynamic_bitset_base<B>::temporary_type;
        temporary_type res(lhs.derived_cast());
        res.flip();
        return res;
    }

    template<class L, class R>
    inline auto operator&(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs) {
        using temporary_type = typename dynamic_bitset_base<L>::temporary_type;
        temporary_type res(lhs.derived_cast());
        res &= rhs;
        return res;
    }

    template<class L, class R>
    inline auto operator|(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs) {
        using temporary_type = typename dynamic_bitset_base<L>::temporary_type;
        temporary_type res(lhs.derived_cast());
        res |= rhs;
        return res;
    }

    template<class L, class R>
    inline auto operator^(const dynamic_bitset_base<L> &lhs, const dynamic_bitset_base<R> &rhs) {
        using temporary_type = typename dynamic_bitset_base<L>::temporary_type;
        temporary_type res(lhs.derived_cast());
        res ^= rhs;
        return res;
    }

    template<class B>
    inline void swap(const dynamic_bitset_base<B> &lhs, const dynamic_bitset_base<B> &rhs) {
        return lhs.swap(rhs);
    }

/************************************
 * bitset_reference implementation *
 ************************************/

    template<class B, bool C>
    inline bitset_reference<B, C>::bitset_reference(closure_type block, block_type pos)
            : m_block(block), m_mask(block_type(1) << pos) {
    }

    template<class B, bool C>
    inline bitset_reference<B, C>::operator bool() const noexcept {
        return (m_block & m_mask) != 0;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator=(const self_type &rhs) noexcept -> self_type & {
        assign(rhs);
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator=(self_type &&rhs) noexcept -> self_type & {
        assign(rhs);
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator=(bool rhs) noexcept -> self_type & {
        assign(rhs);
        return *this;
    }

    template<class B, bool C>
    inline bool bitset_reference<B, C>::operator~() const noexcept {
        return (m_block & m_mask) == 0;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator&=(bool rhs) noexcept -> self_type & {
        if (!rhs) {
            reset();
        }
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator|=(bool rhs) noexcept -> self_type & {
        if (rhs) {
            set();
        }
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator^=(bool rhs) noexcept -> self_type & {
        return rhs ? flip() : *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::flip() noexcept -> self_type & {
        m_block ^= m_mask;
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_reference<B, C>::operator&() noexcept -> pointer {
        return pointer(*this);
    }

    template<class B, bool C>
    inline void bitset_reference<B, C>::assign(bool rhs) noexcept {
        rhs ? set() : reset();
    }

    template<class B, bool C>
    inline void bitset_reference<B, C>::set() noexcept {
        m_block |= m_mask;
    }

    template<class B, bool C>
    inline void bitset_reference<B, C>::reset() noexcept {
        m_block &= ~m_mask;
    }

/***********************************
 * bitset_iterator implementation *
 ***********************************/

    template<class B, bool C>
    inline bitset_iterator<B, C>::bitset_iterator() noexcept
            : p_container(nullptr), m_index(0) {
    }

    template<class B, bool C>
    inline bitset_iterator<B, C>::bitset_iterator(container_reference c, size_type index) noexcept
            : p_container(&c), m_index(index) {
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator++() -> self_type & {
        ++m_index;
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator--() -> self_type & {
        --m_index;
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator+=(difference_type n) -> self_type & {
        difference_type res = static_cast<difference_type>(m_index) + n;
        m_index = static_cast<size_type>(res);
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator-=(difference_type n) -> self_type & {
        difference_type res = static_cast<difference_type>(m_index) - n;
        m_index = static_cast<size_type>(res);
        return *this;
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator-(const self_type &rhs) const -> difference_type {
        return difference_type(m_index - rhs.m_index);
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator*() const -> reference {
        return (*p_container)[m_index];
    }

    template<class B, bool C>
    inline auto bitset_iterator<B, C>::operator->() const -> pointer {
        return &(operator*());
    }

    template<class B, bool C>
    inline bool bitset_iterator<B, C>::operator==(const self_type &rhs) const {
        return p_container == rhs.p_container && m_index == rhs.m_index;
    }

    template<class B, bool C>
    inline bool bitset_iterator<B, C>::operator<(const self_type &rhs) const {
        return p_container == rhs.p_container && m_index < rhs.m_index;
    }


} // namespace turbo

#endif  //  TURBO_CONTAINER_DYNAMIC_BITSET_H_
