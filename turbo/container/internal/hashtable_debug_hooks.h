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
// Provides the internal API for hashtable_debug.h.

#ifndef TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_HOOKS_H_
#define TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_HOOKS_H_

#include <cstddef>

#include <algorithm>
#include <type_traits>
#include <vector>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace hashtable_debug_internal {

// If it is a map, call get<0>().
using std::get;
template <typename T, typename = typename T::mapped_type>
auto GetKey(const typename T::value_type& pair, int) -> decltype(get<0>(pair)) {
  return get<0>(pair);
}

// If it is not a map, return the value directly.
template <typename T>
const typename T::key_type& GetKey(const typename T::key_type& key, char) {
  return key;
}

// Containers should specialize this to provide debug information for that
// container.
template <class Container, typename Enabler = void>
struct HashtableDebugAccess {
  // Returns the number of probes required to find `key` in `c`.  The "number of
  // probes" is a concept that can vary by container.  Implementations should
  // return 0 when `key` was found in the minimum number of operations and
  // should increment the result for each non-trivial operation required to find
  // `key`.
  //
  // The default implementation uses the bucket api from the standard and thus
  // works for `std::unordered_*` containers.
  static size_t GetNumProbes(const Container& c,
                             const typename Container::key_type& key) {
    if (!c.bucket_count()) return {};
    size_t num_probes = 0;
    size_t bucket = c.bucket(key);
    for (auto it = c.begin(bucket), e = c.end(bucket);; ++it, ++num_probes) {
      if (it == e) return num_probes;
      if (c.key_eq()(key, GetKey<Container>(*it, 0))) return num_probes;
    }
  }

  // Returns the number of bytes requested from the allocator by the container
  // and not freed.
  //
  // static size_t AllocatedByteSize(const Container& c);

  // Returns a tight lower bound for AllocatedByteSize(c) where `c` is of type
  // `Container` and `c.size()` is equal to `num_elements`.
  //
  // static size_t LowerBoundAllocatedByteSize(size_t num_elements);
};

}  // namespace hashtable_debug_internal
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_HOOKS_H_
