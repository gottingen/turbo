//
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
// Created by jeff on 24-6-5.
//

#pragma once

#include <turbo/container/small_vector.h>
#include <turbo/container/inlined_vector.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/match.h>
#include <turbo/strings/internal/memutil.h>
#include <cstddef>

namespace turbo {

    /// SmallString - A SmallString is just a SmallVector with methods and accessors
    /// that make it work better as a string (e.g. operator+ etc).
    template<unsigned InternalLen>
    class SmallString : public SmallVector<char, InternalLen> {
    public:
        /// Default ctor - Initialize to empty.
        SmallString() = default;

        /// Initialize from a  std::string_view.
        SmallString(std::string_view S) : SmallVector<char, InternalLen>(S.begin(), S.end()) {}

        /// Initialize by concatenating a list of  turbo::string_views.
        SmallString(std::initializer_list<std::string_view> Refs)
                : SmallVector<char, InternalLen>() {
            this->append(Refs);
        }

        /// Initialize with a range.
        template<typename ItTy>
        SmallString(ItTy S, ItTy E) : SmallVector<char, InternalLen>(S, E) {}

        /// @}
        /// @name String Assignment
        /// @{

        using SmallVector<char, InternalLen>::assign;

        /// Assign from a  std::string_view.
        void assign(std::string_view RHS) {
            SmallVectorImpl<char>::assign(RHS.begin(), RHS.end());
        }

        /// Assign from a list of  turbo::string_views.
        void assign(std::initializer_list<std::string_view> Refs) {
            this->clear();
            append(Refs);
        }

        /// @}
        /// @name String Concatenation
        /// @{

        using SmallVector<char, InternalLen>::append;

        /// Append from a  std::string_view.
        void append(std::string_view RHS) {
            SmallVectorImpl<char>::append(RHS.begin(), RHS.end());
        }

        /// Append from a list of  turbo::string_views.
        void append(std::initializer_list<std::string_view> Refs) {
            size_t CurrentSize = this->size();
            size_t SizeNeeded = CurrentSize;
            for (const std::string_view &Ref: Refs)
                SizeNeeded += Ref.size();
            this->resize_for_overwrite(SizeNeeded);
            for (const std::string_view &Ref: Refs) {
                std::copy(Ref.begin(), Ref.end(), this->begin() + CurrentSize);
                CurrentSize += Ref.size();
            }
            assert(CurrentSize == this->size());
        }

        /// @}
        /// @name String Comparison
        /// @{

        /// Check for string equality.  This is more efficient than compare() when
        /// the relative ordering of inequal strings isn't needed.
        bool equals(std::string_view RHS) const {
            return str() == RHS;
        }

        /// Check for string equality, ignoring case.
        bool equals_insensitive(std::string_view RHS) const {
            return compare_insensitive(RHS) == 0;
        }

        /// compare - Compare two strings; the result is negative, zero, or positive
        /// if this string is lexicographically less than, equal to, or greater than
        /// the \p RHS.
        int compare(std::string_view RHS) const {
            return str().compare(RHS);
        }

        /// compare_insensitive - Compare two strings, ignoring case.
        int compare_insensitive(std::string_view rhs) const {
            auto lhs = str();
            auto min_size = std::min(lhs.size(), rhs.size());
            auto r = turbo::strings_internal::memcasecmp(lhs.data(), rhs.data(), min_size);
            if(r == 0) {
                if(lhs.size() == rhs.size())
                    return 0;
                return lhs.size() < rhs.size() ? -1 : 1;
            }
            return r < 0 ? -1 : 1;
        }

        /// @}
        /// @name String Predicates
        /// @{

        /// startswith - Check if this string starts with the given \p Prefix.
        bool starts_with(std::string_view Prefix) const {
            return turbo::starts_with(str(), Prefix);
        }

        /// endswith - Check if this string ends with the given \p Suffix.
        bool ends_with(std::string_view Suffix) const {
            return turbo::ends_with(str(), Suffix);
        }

        /// @}
        /// @name String Searching
        /// @{

        /// find - Search for the first character \p C in the string.
        ///
        /// \return - The index of the first occurrence of \p C, or npos if not
        /// found.
        size_t find(char C, size_t From = 0) const {
            return str().find(C, From);
        }

        /// Search for the first string \p Str in the string.
        ///
        /// \returns The index of the first occurrence of \p Str, or npos if not
        /// found.
        size_t find(std::string_view Str, size_t From = 0) const {
            return str().find(Str, From);
        }

        /// Search for the last character \p C in the string.
        ///
        /// \returns The index of the last occurrence of \p C, or npos if not
        /// found.
        size_t rfind(char C, size_t From = std::string_view::npos) const {
            return str().rfind(C, From);
        }

        /// Search for the last string \p Str in the string.
        ///
        /// \returns The index of the last occurrence of \p Str, or npos if not
        /// found.
        size_t rfind(std::string_view Str) const {
            return str().rfind(Str);
        }

        /// Find the first character in the string that is \p C, or npos if not
        /// found. Same as find.
        size_t find_first_of(char C, size_t From = 0) const {
            return str().find_first_of(C, From);
        }

        /// Find the first character in the string that is in \p Chars, or npos if
        /// not found.
        ///
        /// Complexity: O(size() + Chars.size())
        size_t find_first_of(std::string_view Chars, size_t From = 0) const {
            return str().find_first_of(Chars, From);
        }

        /// Find the first character in the string that is not \p C or npos if not
        /// found.
        size_t find_first_not_of(char C, size_t From = 0) const {
            return str().find_first_not_of(C, From);
        }

        /// Find the first character in the string that is not in the string
        /// \p Chars, or npos if not found.
        ///
        /// Complexity: O(size() + Chars.size())
        size_t find_first_not_of(std::string_view Chars, size_t From = 0) const {
            return str().find_first_not_of(Chars, From);
        }

        /// Find the last character in the string that is \p C, or npos if not
        /// found.
        size_t find_last_of(char C, size_t From = std::string_view::npos) const {
            return str().find_last_of(C, From);
        }

        /// Find the last character in the string that is in \p C, or npos if not
        /// found.
        ///
        /// Complexity: O(size() + Chars.size())
        size_t find_last_of(
                std::string_view Chars, size_t From = std::string_view::npos) const {
            return str().find_last_of(Chars, From);
        }

        /// @}
        /// @name Substring Operations
        /// @{

        /// Return a reference to the substring from [Start, Start + N).
        ///
        /// \param Start The index of the starting character in the substring; if
        /// the index is npos or greater than the length of the string then the
        /// empty substring will be returned.
        ///
        /// \param N The number of characters to included in the substring. If \p N
        /// exceeds the number of characters remaining in the string, the string
        /// suffix (starting with \p Start) will be returned.
        std::string_view substr(size_t Start, size_t N = std::string_view::npos) const {
            if(Start >= this->size())
                return std::string_view();
            return str().substr(Start, N);
        }

        /// Return a reference to the substring from [Start, End).
        ///
        /// \param Start The index of the starting character in the substring; if
        /// the index is npos or greater than the length of the string then the
        /// empty substring will be returned.
        ///
        /// \param End The index following the last character to include in the
        /// substring. If this is npos, or less than \p Start, or exceeds the
        /// number of characters remaining in the string, the string suffix
        /// (starting with \p Start) will be returned.
        std::string_view slice(size_t Start, size_t End) const {
            if(End <= Start || Start >= this->size())
                return std::string_view();

            return str().substr(Start, End - Start);
        }

        // Extra methods.

        /// Explicit conversion to  std::string_view.
        std::string_view str() const { return std::string_view(this->data(), this->size()); }

        // TODO: Make this const, if it's safe...
        const char *c_str() {
            this->push_back(0);
            this->pop_back();
            return this->data();
        }

        template<typename H>
        friend H turbo_hash_value(H h, const turbo::SmallString<InternalLen> &a) {
            auto str = a.str();
            return H::combine(H::combine_contiguous(std::move(h), str.data(), str.size()),str.size());
        }

        /// Implicit conversion to  std::string_view.
        operator std::string_view() const { return str(); }

        explicit operator std::string() const {
            return std::string(this->data(), this->size());
        }

        // Extra operators.
        SmallString &operator=(std::string_view RHS) {
            this->assign(RHS);
            return *this;
        }

        SmallString &operator+=(std::string_view RHS) {
            this->append(RHS.begin(), RHS.end());
            return *this;
        }

        SmallString &operator+=(char C) {
            this->push_back(C);
            return *this;
        }
    };

    template<unsigned M, unsigned N>
    bool operator==(const SmallString<M> &LHS, const SmallString<N> &RHS) {
        return LHS.str() == RHS.str();
    }

    template<unsigned M>
    bool operator==(const SmallString<M> &LHS, std::string_view RHS) {
        return LHS.str() == RHS;
    }

    template<unsigned M>
    bool operator==(std::string_view LHS, const SmallString<M> &RHS) {
        return LHS == RHS.str();
    }

    template<typename Sink, unsigned N>
    void turbo_stringify(Sink &S, const SmallString<N> &Str) {
        S << Str.str();
    }

    template<unsigned N>
    std::ostream &operator<<(std::ostream &OS,  const SmallString<N> &Str) {
        return OS<<Str.str();
    }
} // end namespace turbo
