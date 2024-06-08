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
#include <turbo/container/btree_map.h>
#include <cstddef>
#include <iostream>
#include <map>

namespace turbo {
    /**
     * \brief LFU (Least frequently used) cache policy
     * \details LFU policy in the case of replacement removes the least frequently used
     * element.
     *
     * Each access to an element in the cache increments internal counter (frequency) that
     * represents how many times that particular key has been accessed by someone. When a
     * replacement has to occur the LFU policy just takes a look onto keys' frequencies
     * and remove the least used one. E.g. cache of two elements where `A` has been accessed
     * 10 times and `B` – only 2. When you want to add a key `C` the LFU policy returns `B`
     * as a replacement candidate.
     * \tparam Key Type of a key a policy works with
     */
    template<typename Key>
    class LFUCachePolicy : public CachePolicyBase<Key> {
    public:
        using lfu_iterator = typename std::multimap<std::size_t, Key>::iterator;

        LFUCachePolicy() = default;

        ~LFUCachePolicy() override = default;

        void insert(const Key &key) override {
            constexpr std::size_t INIT_VAL = 1;
            // all new value initialized with the frequency 1
            lfu_storage[key] =
                    frequency_storage.emplace_hint(frequency_storage.cbegin(), INIT_VAL, key);
        }

        void touch(const Key &key) override {
            // get the previous frequency value of a key
            auto elem_for_update = lfu_storage[key];
            auto updated_elem = std::make_pair(elem_for_update->first + 1, elem_for_update->second);
            // update the previous value
            frequency_storage.erase(elem_for_update);
            lfu_storage[key] =
                    frequency_storage.emplace_hint(frequency_storage.cend(), std::move(updated_elem));
        }

        void erase(const Key &key) noexcept override {
            frequency_storage.erase(lfu_storage[key]);
            lfu_storage.erase(key);
        }

        const Key &repl_candidate() const noexcept override {
            // at the beginning of the frequency_storage we have the
            // least frequency used value
            return frequency_storage.cbegin()->second;
        }

    private:
        std::multimap<std::size_t, Key> frequency_storage;
        turbo::flat_hash_map<Key, lfu_iterator> lfu_storage;
    };
} // namespace turbo
