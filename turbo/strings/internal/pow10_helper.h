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
// This test helper library contains a table of powers of 10, to guarantee
// precise values are computed across the full range of doubles. We can't rely
// on the pow() function, because not all standard libraries ship a version
// that is precise.
#ifndef TURBO_STRINGS_INTERNAL_POW10_HELPER_H_
#define TURBO_STRINGS_INTERNAL_POW10_HELPER_H_

#include <vector>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

// Computes the precise value of 10^exp. (I.e. the nearest representable
// double to the exact value, rounding to nearest-even in the (single) case of
// being exactly halfway between.)
double Pow10(int exp);

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_POW10_HELPER_H_
