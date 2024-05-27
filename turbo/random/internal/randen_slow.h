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

#ifndef TURBO_RANDOM_INTERNAL_RANDEN_SLOW_H_
#define TURBO_RANDOM_INTERNAL_RANDEN_SLOW_H_

#include <cstddef>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss High German.
// RandenSlow implements the basic state manipulation methods for
// architectures lacking AES hardware acceleration intrinsics.
class RandenSlow {
 public:
  static void Generate(const void* keys, void* state_void);
  static void Absorb(const void* seed_void, void* state_void);
  static const void* GetKeys();
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_RANDEN_SLOW_H_
