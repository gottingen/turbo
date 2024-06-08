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
// Created by jeff on 24-6-8.
//

#pragma once

#include <turbo/container/cache/cache_policy.h>
#include <turbo/container/flat_hash_map.h>
#include <list>

namespace turbo {

    /**
     * \brief FIFO (First in, first out) cache policy
     * \details FIFO policy in the case of replacement removes the first added element.
     *
     * That is, consider the following key adding sequence:
     * ```
     * A -> B -> C -> ...
     * ```
     * In the case a cache reaches its capacity, the FIFO replacement candidate policy
     * returns firstly added element `A`. To show that:
     * ```
     * # New key: X
     * Initial state: A -> B -> C -> ...
     * Replacement candidate: A
     * Final state: B -> C -> ... -> X -> ...
     * ```
     * An so on, the next candidate will be `B`, then `C`, etc.
     * \tparam Key Type of a key a policy works with
     */
    template<typename Key>
    class FIFOCachePolicy : public CachePolicyBase<Key> {
    public:
        using fifo_iterator = typename std::list<Key>::const_iterator;

        FIFOCachePolicy() = default;

        ~FIFOCachePolicy() = default;

        void insert(const Key &key) override {
            fifo_queue.emplace_front(key);
            key_lookup[key] = fifo_queue.begin();
        }

        // handle request to the key-element in a cache
        void touch(const Key &key) noexcept override {
            // nothing to do here in the FIFO strategy
            (void) key;
        }

        // handle element deletion from a cache
        void erase(const Key &key) noexcept override {
            auto element = key_lookup[key];
            fifo_queue.erase(element);
            key_lookup.erase(key);
        }

        // return a key of a replacement candidate
        const Key &repl_candidate() const noexcept override {
            return fifo_queue.back();
        }

    private:
        std::list<Key> fifo_queue;
        turbo::flat_hash_map<Key, fifo_iterator> key_lookup;
    };
} // namespace turbo

