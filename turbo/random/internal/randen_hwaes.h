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

#ifndef TURBO_RANDOM_INTERNAL_RANDEN_HWAES_H_
#define TURBO_RANDOM_INTERNAL_RANDEN_HWAES_H_

#include <turbo/base/config.h>

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss High German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// RandenHwAes implements the basic state manipulation methods.
class RandenHwAes {
 public:
  static void Generate(const void* keys, void* state_void);
  static void Absorb(const void* seed_void, void* state_void);
  static const void* GetKeys();
};

// HasRandenHwAesImplementation returns true when there is an accelerated
// implementation, and false otherwise.  If there is no implementation,
// then attempting to use it will abort the program.
bool HasRandenHwAesImplementation();

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_RANDEN_HWAES_H_
