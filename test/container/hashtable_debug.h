
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef HASHTABLE_DEBUG_H_
#define HASHTABLE_DEBUG_H_

#include <cstddef>
#include <algorithm>
#include <type_traits>
#include <vector>

namespace flare {
    namespace priv {

        // Returns the number of probes required to lookup `key`.  Returns 0 for a
        // search with no collisions.  Higher values mean more hash collisions occurred;
        // however, the exact meaning of this number varies according to the container
        // type.
        template<typename C>
        size_t GetHashtableDebugNumProbes(
                const C &c, const typename C::key_type &key) {
            return flare::priv::hashtable_debug_internal::
            HashtableDebugAccess<C>::GetNumProbes(c, key);
        }

        // Gets a histogram of the number of probes for each elements in the container.
        // The sum of all the values in the vector is equal to container.size().
        template<typename C>
        std::vector<size_t> GetHashtableDebugNumProbesHistogram(const C &container) {
            std::vector<size_t> v;
            for (auto it = container.begin(); it != container.end(); ++it) {
                size_t num_probes = GetHashtableDebugNumProbes(
                        container,
                        flare::priv::hashtable_debug_internal::GetKey<C>(*it, 0));
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
        template<typename C>
        HashtableDebugProbeSummary GetHashtableDebugProbeSummary(const C &container) {
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
        template<typename C>
        size_t AllocatedByteSize(const C &c) {
            return flare::priv::hashtable_debug_internal::
            HashtableDebugAccess<C>::AllocatedByteSize(c);
        }

        // Returns a tight lower bound for AllocatedByteSize(c) where `c` is of type `C`
        // and `c.size()` is equal to `num_elements`.
        template<typename C>
        size_t LowerBoundAllocatedByteSize(size_t num_elements) {
            return flare::priv::hashtable_debug_internal::
            HashtableDebugAccess<C>::LowerBoundAllocatedByteSize(num_elements);
        }

    }  // namespace priv
}  // namespace flare

#endif  // HASHTABLE_DEBUG_H_
