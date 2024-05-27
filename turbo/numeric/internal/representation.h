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

#ifndef TURBO_NUMERIC_INTERNAL_REPRESENTATION_H_
#define TURBO_NUMERIC_INTERNAL_REPRESENTATION_H_

#include <limits>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace numeric_internal {

// Returns true iff long double is represented as a pair of doubles added
// together.
inline constexpr bool IsDoubleDouble() {
  // A double-double value always has exactly twice the precision of a double
  // value--one double carries the high digits and one double carries the low
  // digits. This property is not shared with any other common floating-point
  // representation, so this test won't trigger false positives. For reference,
  // this table gives the number of bits of precision of each common
  // floating-point representation:
  //
  //                type     precision
  //         IEEE single          24 b
  //         IEEE double          53
  //     x86 long double          64
  //       double-double         106
  //      IEEE quadruple         113
  //
  // Note in particular that a quadruple-precision float has greater precision
  // than a double-double float despite taking up the same amount of memory; the
  // quad has more of its bits allocated to the mantissa than the double-double
  // has.
  return std::numeric_limits<long double>::digits ==
         2 * std::numeric_limits<double>::digits;
}

}  // namespace numeric_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_NUMERIC_INTERNAL_REPRESENTATION_H_
