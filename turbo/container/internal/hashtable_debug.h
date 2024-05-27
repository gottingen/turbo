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
// This library provides APIs to debug the probing behavior of hash tables.
//
// In general, the probing behavior is a black box for users and only the
// side effects can be measured in the form of performance differences.
// These APIs give a glimpse on the actual behavior of the probing algorithms in
// these hashtables given a specified hash function and a set of elements.
//
// The probe count distribution can be used to assess the quality of the hash
// function for that particular hash table. Note that a hash function that
// performs well in one hash table implementation does not necessarily performs
// well in a different one.
//
// This library supports std::unordered_{set,map}, dense_hash_{set,map} and
// turbo::{flat,node,string}_hash_{set,map}.

#ifndef TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_
#define TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <vector>

#include <turbo/container/internal/hashtable_debug_hooks.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

// Returns the number of probes required to lookup `key`.  Returns 0 for a
// search with no collisions.  Higher values mean more hash collisions occurred;
// however, the exact meaning of this number varies according to the container
// type.
template <typename C>
size_t GetHashtableDebugNumProbes(
    const C& c, const typename C::key_type& key) {
  return turbo::container_internal::hashtable_debug_internal::
      HashtableDebugAccess<C>::GetNumProbes(c, key);
}

// Gets a histogram of the number of probes for each elements in the container.
// The sum of all the values in the vector is equal to container.size().
template <typename C>
std::vector<size_t> GetHashtableDebugNumProbesHistogram(const C& container) {
  std::vector<size_t> v;
  for (auto it = container.begin(); it != container.end(); ++it) {
    size_t num_probes = GetHashtableDebugNumProbes(
        container,
        turbo::container_internal::hashtable_debug_internal::GetKey<C>(*it, 0));
    v.resize((std::max)(v.size(), num_probes + 1));
    v[num_probes]++;
  }
  return v;
}

struct HashtableDebugProbeSummary {
  size_t total_elements;
  size_t total_num_probes;
  double mean;
};

// Gets a summary of the probe count distribution for the elements in the
// container.
template <typename C>
HashtableDebugProbeSummary GetHashtableDebugProbeSummary(const C& container) {
  auto probes = GetHashtableDebugNumProbesHistogram(container);
  HashtableDebugProbeSummary summary = {};
  for (size_t i = 0; i < probes.size(); ++i) {
    summary.total_elements += probes[i];
    summary.total_num_probes += probes[i] * i;
  }
  summary.mean = 1.0 * summary.total_num_probes / summary.total_elements;
  return summary;
}

// Returns the number of bytes requested from the allocator by the container
// and not freed.
template <typename C>
size_t AllocatedByteSize(const C& c) {
  return turbo::container_internal::hashtable_debug_internal::
      HashtableDebugAccess<C>::AllocatedByteSize(c);
}

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_HASHTABLE_DEBUG_H_
