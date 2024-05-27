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

#ifndef TURBO_RANDOM_INTERNAL_CHI_SQUARE_H_
#define TURBO_RANDOM_INTERNAL_CHI_SQUARE_H_

// The chi-square statistic.
//
// Useful for evaluating if `D` independent random variables are behaving as
// expected, or if two distributions are similar.  (`D` is the degrees of
// freedom).
//
// Each bucket should have an expected count of 10 or more for the chi square to
// be meaningful.

#include <cassert>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

constexpr const char kChiSquared[] = "chi-squared";

// Returns the measured chi square value, using a single expected value.  This
// assumes that the values in [begin, end) are uniformly distributed.
template <typename Iterator>
double ChiSquareWithExpected(Iterator begin, Iterator end, double expected) {
  // Compute the sum and the number of buckets.
  assert(expected >= 10);  // require at least 10 samples per bucket.
  double chi_square = 0;
  for (auto it = begin; it != end; it++) {
    double d = static_cast<double>(*it) - expected;
    chi_square += d * d;
  }
  chi_square = chi_square / expected;
  return chi_square;
}

// Returns the measured chi square value, taking the actual value of each bucket
// from the first set of iterators, and the expected value of each bucket from
// the second set of iterators.
template <typename Iterator, typename Expected>
double ChiSquare(Iterator it, Iterator end, Expected eit, Expected eend) {
  double chi_square = 0;
  for (; it != end && eit != eend; ++it, ++eit) {
    if (*it > 0) {
      assert(*eit > 0);
    }
    double e = static_cast<double>(*eit);
    double d = static_cast<double>(*it - *eit);
    if (d != 0) {
      assert(e > 0);
      chi_square += (d * d) / e;
    }
  }
  assert(it == end && eit == eend);
  return chi_square;
}

// ======================================================================
// The following methods can be used for an arbitrary significance level.
//

// Calculates critical chi-square values to produce the given p-value using a
// bisection search for a value within epsilon, relying on the monotonicity of
// ChiSquarePValue().
double ChiSquareValue(int dof, double p);

// Calculates the p-value (probability) of a given chi-square value.
double ChiSquarePValue(double chi_square, int dof);

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_CHI_SQUARE_H_
