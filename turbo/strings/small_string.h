// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#ifndef TURBO_STRING_SMALL_STRING_H_
#define TURBO_STRING_SMALL_STRING_H_

#include <string>
#include <cstring>
#include <algorithm>
#include <array>
#include <type_traits>
#include <string_view>
#include "turbo/platform/port.h"

namespace turbo {

    template<typename CharT, size_t N>
    class small_string {
    public:
        static constexpr size_t max_size = N;

        using value_type = CharT;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = CharT &;
        using const_reference = const CharT &;
        using pointer = CharT *;
        using const_pointer = const CharT *;
        using iterator = CharT *;
        using const_iterator = const CharT *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static_assert(std::is_trivially_copyable_v<CharT>);

        constexpr small_string() = default;

        small_string(std::basic_string_view<CharT> str) {
            TURBO_ASSERT(str.size() <= N);
            _size = str.size();
            std::copy_n(str.data(), _size, _data.data());
        }

        small_string(const CharT *str, size_t len) {
            TURBO_ASSERT(len <= N);
            _size = len;
            std::copy_n(str, _size, _data.data());
        }

        small_string(const small_string &other) {
            _size = other._size;
            std::copy_n(other._data.data(), _size, _data.data());
        }

        small_string &operator=(const small_string &other) {
            _size = other._size;
            std::copy_n(other._data.data(), _size, _data.data());
            return *this;
        }

        small_string(small_string &&other) noexcept {
            _size = other._size;
            std::copy_n(other._data.data(), _size, _data.data());
        }

        small_string &operator=(small_string &&other) noexcept {
            _size = other._size;
            std::copy_n(other._data.data(), _size, _data.data());
            return *this;
        }


        size_t size() const {
            return _size;
        }

        size_t length() const {
            return _size;
        }

        bool empty() const {
            return _size == 0;
        }

        void clear() {
            _size = 0;
        }

        void resize(size_t n) {
            TURBO_ASSERT(n <= N);
            _size = n;
        }

        void resize(size_t n, CharT c) {
            TURBO_ASSERT(n <= N);
            _size = n;
            std::fill_n(_data.data(), _size, c);
        }

        const CharT *c_str() const {
            _data[_size] = '\0';
            return _data.data();
        }

        iterator begin() {
            return _data.data();
        }

        iterator end() {
            return _data.data() + _size;
        }

        const_iterator begin() const {
            return _data.data();
        }

        const_iterator end() const {
            return _data.data() + _size;
        }

        const_iterator cbegin() const {
            return _data.data();
        }

        const_iterator cend() const {
            return _data.data() + _size;
        }

        reverse_iterator rbegin() {
            return reverse_iterator(end());
        }

        reverse_iterator rend() {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crend() const {
            return const_reverse_iterator(begin());
        }

        CharT &operator[](size_t pos) {
            TURBO_ASSERT(pos < _size);
            return _data[pos];
        }

        const CharT &operator[](size_t pos) const {
            TURBO_ASSERT(pos < _size);
            return _data[pos];
        }

        CharT &at(size_t pos) {
            TURBO_ASSERT(pos < _size);
            return _data[pos];
        }

        const CharT &at(size_t pos) const {
            TURBO_ASSERT(pos < _size);
            return _data[pos];
        }

        CharT &front() {
            TURBO_ASSERT(_size > 0);
            return _data[0];
        }

        const CharT &front() const {
            TURBO_ASSERT(_size > 0);
            return _data[0];
        }

        CharT &back() {
            TURBO_ASSERT(_size > 0);
            return _data[_size - 1];
        }

        const CharT &back() const {
            TURBO_ASSERT(_size > 0);
            return _data[_size - 1];
        }

        void push_back(CharT c) {
            TURBO_ASSERT(_size < N);
            _data[_size++] = c;
        }

        void pop_back() {
            TURBO_ASSERT(_size > 0);
            --_size;
        }

        small_string &append(std::basic_string_view<CharT> str) {
            TURBO_ASSERT(_size + str.size() <= N);
            std::copy_n(str.data(), str.size(), _data.data() + _size);
            _size += str.size();
            return *this;
        }

        small_string &append(const CharT *str, size_t len) {
            TURBO_ASSERT(_size + len <= N);
            std::copy_n(str, len, _data.data() + _size);
            _size += len;
            return *this;
        }

        small_string &append(const small_string &other) {
            TURBO_ASSERT(_size + other._size <= N);
            std::copy_n(other._data.data(), other._size, _data.data() + _size);
            _size += other._size;
            return *this;
        }

        small_string &append(size_t n, CharT c) {
            TURBO_ASSERT(_size + n <= N);
            std::fill_n(_data.data() + _size, n, c);
            _size += n;
            return *this;
        }

        small_string &append(std::initializer_list<CharT> ilist) {
            TURBO_ASSERT(_size + ilist.size() <= N);
            std::copy_n(ilist.begin(), ilist.size(), _data.data() + _size);
            _size += ilist.size();
            return *this;
        }

        small_string &operator+=(std::basic_string_view<CharT> str) {
            return append(str);
        }

        small_string &operator+=(const CharT *str) {
            return append(str, std::char_traits<CharT>::length(str));
        }

        small_string &operator+=(CharT c) {
            return append(1, c);
        }

        small_string &operator+=(const small_string &other) {
            return append(other);
        }

        small_string &operator+=(std::initializer_list<CharT> ilist) {
            return append(ilist);
        }

        int compare(std::basic_string_view<CharT> str) const {
            std::basic_string_view<CharT> lhs(_data.data(), _size);
            return lhs.compare(str);
        }

        int compare(const small_string &other) const {
            std::basic_string_view<CharT> lhs(_data.data(), _size);
            std::basic_string_view<CharT> rhs(other._data.data(), other._size);
            return lhs.compare(rhs);
        }

        operator std::basic_string_view<CharT>() const {
            return std::basic_string_view<CharT>(_data.data(), _size);
        }

        operator std::basic_string<CharT>() const {
            return std::basic_string<CharT>(_data.data(), _size);
        }

    private:
        // some unicode may need 2 '\0' to terminate
        size_t _size{0};
        std::array<CharT, N + 2> _data;
    };
}
#endif // TURBO_STRING_SMALL_STRING_H_
