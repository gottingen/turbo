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
// File: algorithm.h
// -----------------------------------------------------------------------------
//
// This header file contains Google extensions to the standard <algorithm> C++
// header.

#ifndef TURBO_ALGORITHM_ALGORITHM_H_
#define TURBO_ALGORITHM_ALGORITHM_H_

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// equal()
// rotate()
//
// Historical note: Turbo once provided implementations of these algorithms
// prior to their adoption in C++14. New code should prefer to use the std
// variants.
//
// See the documentation for the STL <algorithm> header for more information:
// https://en.cppreference.com/w/cpp/header/algorithm
using std::equal;
using std::rotate;

// linear_search()
//
// Performs a linear search for `value` using the iterator `first` up to
// but not including `last`, returning true if [`first`, `last`) contains an
// element equal to `value`.
//
// A linear search is of O(n) complexity which is guaranteed to make at most
// n = (`last` - `first`) comparisons. A linear search over short containers
// may be faster than a binary search, even when the container is sorted.
template <typename InputIterator, typename EqualityComparable>
bool linear_search(InputIterator first, InputIterator last,
                   const EqualityComparable& value) {
  return std::find(first, last, value) != last;
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_ALGORITHM_ALGORITHM_H_
