
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_BIT_ITERATOR_H_
#define FLARE_BASE_MATH_BIT_ITERATOR_H_

#include <bitset>
#include <limits>
#include "flare/base/profile.h"
#include "flare/base/math/countl_zero.h"
#include "flare/base/math/countr_zero.h"


namespace flare::base {

    static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;

    /**
     * Returns the index of the first set bit.
     * Result is undefined if bitset.any() == false.
     */
    template<size_t N>
    static FLARE_FORCE_INLINE size_t get_first_set(const std::bitset<N> &bitset) {
        static_assert(N <= ulong_bits, "bitset too large");
        return count_trailing_zeros(bitset.to_ulong());
    }

    /**
     * Returns the index of the last set bit in the bitset.
     * Result is undefined if bitset.any() == false.
     */
    template<size_t N>
    static FLARE_FORCE_INLINE size_t get_last_set(const std::bitset<N> &bitset) {
        static_assert(N <= ulong_bits, "bitset too large");
        return ulong_bits - 1 - countl_zero(bitset.to_ulong());
    }

    template<size_t N>
    class set_iterator : public std::iterator<std::input_iterator_tag, int> {
    private:
        void advance() {
            if (_bitset.none()) {
                _index = -1;
            } else {
                auto shift = get_first_set(_bitset) + 1;
                _index += shift;
                _bitset >>= shift;
            }
        }

    public:
        set_iterator(std::bitset<N> bitset, int offset = 0)
                : _bitset(bitset), _index(offset - 1) {
            static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
            _bitset >>= offset;
            advance();
        }

        void operator++() {
            advance();
        }

        int operator*() const {
            return _index;
        }

        bool operator==(const set_iterator &other) const {
            return _index == other._index;
        }

        bool operator!=(const set_iterator &other) const {
            return !(*this == other);
        }

    private:
        std::bitset<N> _bitset;
        int _index;
    };

    template<size_t N>
    class set_range {
    public:
        using iterator = set_iterator<N>;
        using value_type = int;

        set_range(std::bitset<N> bitset, int offset = 0)
                : _bitset(bitset), _offset(offset) {
        }

        iterator begin() const { return iterator(_bitset, _offset); }

        iterator end() const { return iterator(0); }

    private:
        std::bitset<N> _bitset;
        int _offset;
    };

    template<size_t N>
    FLARE_FORCE_INLINE set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0) {
        return set_range<N>(bitset, offset);
    }

}  // namespace flare::base

#endif  // FLARE_BASE_MATH_BIT_ITERATOR_H_
