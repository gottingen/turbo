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
// This file provides the Google-internal implementation of LowLevelHash.
//
// LowLevelHash is a fast hash function for hash tables, the fastest we've
// currently (late 2020) found that passes the SMHasher tests. The algorithm
// relies on intrinsic 128-bit multiplication for speed. This is not meant to be
// secure - just fast.
//
// It is closely based on a version of wyhash, but does not maintain or
// guarantee future compatibility with it.

#ifndef TURBO_HASH_INTERNAL_LOW_LEVEL_HASH_H_
#define TURBO_HASH_INTERNAL_LOW_LEVEL_HASH_H_

#include <stdint.h>
#include <stdlib.h>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace hash_internal {

// Hash function for a byte array. A 64-bit seed and a set of five 64-bit
// integers are hashed into the result.
//
// To allow all hashable types (including string_view and Span) to depend on
// this algorithm, we keep the API low-level, with as few dependencies as
// possible.
uint64_t LowLevelHash(const void* data, size_t len, uint64_t seed,
                      const uint64_t salt[5]);

// Same as above except the length must be greater than 16.
uint64_t LowLevelHashLenGt16(const void* data, size_t len, uint64_t seed,
                             const uint64_t salt[5]);

}  // namespace hash_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_HASH_INTERNAL_LOW_LEVEL_HASH_H_
