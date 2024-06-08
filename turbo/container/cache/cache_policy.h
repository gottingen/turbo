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

#include <turbo/container/flat_hash_set.h>

namespace turbo {

    // Cache policy abstract base class
    // Key Type of a key a policy works with
    template<typename Key>
    class CachePolicyBase {
    public:
        virtual ~CachePolicyBase() = default;

         // insert a key into the cache
         // key Key that should be used by the policy
        virtual void insert(const Key &key) = 0;

         // Handle request to the key-element in a cache
        virtual void touch(const Key &key) = 0;

         // erase a key from the cache
        virtual void erase(const Key &key) = 0;

         // return a key of a displacement candidate
         // return Replacement candidate according to selected policy
        virtual const Key &repl_candidate() const = 0;
    };

    // Basic no caching policy class
    // Preserve any key provided. erase procedure can get rid of any added keys
    // without specific rules: a replacement candidate will be the first element in the
    // underlying container. As unordered container can be used in the implementation
    // there are no warranties that the first/last added key will be erased
    // Key Type of a key a policy works with
    template<typename Key>
    class NoCachePolicy : public CachePolicyBase<Key> {
    public:
        NoCachePolicy() = default;

        ~NoCachePolicy() noexcept override = default;

        void insert(const Key &key) override {
            key_storage.emplace(key);
        }

        void touch(const Key &key) noexcept override {
            // do not do anything
            (void) key;
        }

        void erase(const Key &key) noexcept override {
            key_storage.erase(key);
        }

        // return a key of a displacement candidate
        const Key &repl_candidate() const noexcept override {
            return *key_storage.cbegin();
        }

    private:
        turbo::flat_hash_set<Key> key_storage;
    };
} // namespace turbo

