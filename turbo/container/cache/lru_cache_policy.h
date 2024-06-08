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

#pragma once

#include <turbo/container/cache/cache_policy.h>
#include <turbo/container/flat_hash_map.h>
#include <list>

namespace turbo {
    /**
     * \brief LRU (Least Recently Used) cache policy
     * \details LRU policy in the case of replacement removes the least recently used element.
     * That is, in the case of replacement necessity, that cache policy returns a key that
     * has not been touched recently. For example, cache maximum size is 3 and 3 elements have
     * been added - `A`, `B`, `C`. Then the following actions were made:
     * ```
     * Cache placement order: A, B, C
     * Cache elements: A, B, C
     * # Cache access:
     * - A touched, B touched
     * # LRU element in the cache: C
     * # Cache access:
     * - B touched, C touched
     * # LRU element in the cache: A
     * # Put new element: D
     * # LRU replacement candidate: A
     *
     * Cache elements: B, C, D
     * ```
     * \tparam Key Type of a key a policy works with
     */
    template<typename Key>
    class LRUCachePolicy : public CachePolicyBase<Key> {
    public:
        using lru_iterator = typename std::list<Key>::iterator;

        LRUCachePolicy() = default;

        ~LRUCachePolicy() = default;

        void insert(const Key &key) override {
            lru_queue.emplace_front(key);
            key_finder[key] = lru_queue.begin();
        }

        void touch(const Key &key) override {
            // move the touched element at the beginning of the lru_queue
            lru_queue.splice(lru_queue.begin(), lru_queue, key_finder[key]);
        }

        void erase(const Key &key) noexcept override {
            // must be exist
            auto element = key_finder[key];
            lru_queue.erase(element);
            key_finder.erase(key);
        }

        // return a key of a displacement candidate
        const Key &repl_candidate() const noexcept override {
            return lru_queue.back();
        }

    private:
        std::list<Key> lru_queue;
        turbo::flat_hash_map<Key, lru_iterator> key_finder;
    };
} // namespace turbo

