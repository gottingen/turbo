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
#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <turbo/container/flat_hash_map.h>

namespace turbo {
    /**
     * \brief Wrapper over the given value type to allow safe returning of a value from the cache
     */
    template<typename V>
    using WrappedValue = std::shared_ptr<V>;

    /**
     * \brief Fixed sized cache that can be used with different policy types (e.g. LRU, FIFO, LFU)
     * \tparam Key Type of a key (should be hashable)
     * \tparam Value Type of a value stored in the cache
     * \tparam Policy Type of a policy to be used with the cache
     * \tparam HashMap Type of a hashmap to use for cache operations. Should have `std::unordered_map`
     * compatible interface
     */
    template<typename Key, typename Value, template<typename> class Policy = NoCachePolicy,
            typename HashMap = turbo::flat_hash_map<Key, WrappedValue<Value>>>
    class fixed_sized_cache {
    public:
        using map_type = HashMap;
        using value_type = typename map_type::mapped_type;
        using iterator = typename map_type::iterator;
        using const_iterator = typename map_type::const_iterator;
        using operation_guard = typename std::lock_guard<std::mutex>;
        using on_erase_cb =
                typename std::function<void(const Key &key, const value_type &value)>;

        /**
         * \brief Fixed sized cache constructor
         * \throw std::invalid_argument
         * \param[in] max_size Maximum size of the cache
         * \param[in] policy Cache policy to use
         * \param[in] on_erase on_erase_cb function to be called when cache's element get erased
         */
        explicit fixed_sized_cache(
                size_t max_size, const Policy<Key> policy = Policy<Key>{},
                on_erase_cb on_erase = [](const Key &, const value_type &) {})
                : cache_policy{policy}, max_cache_size{max_size}, on_erase_callback{on_erase} {
            if (max_cache_size == 0) {
                throw std::invalid_argument{"Size of the cache should be non-zero"};
            }
        }

        ~fixed_sized_cache() noexcept {
            clear();
        }

        // put a key-value pair into the cache
        // if the key is already present in the cache, the value will be updated
        // key Key to be used in the cache
        // value Value to be stored in the cache
        // cb Callback function to be called when the element is erased
        void put(const Key &key, const Value &value, on_erase_cb cb = nullptr) noexcept {
            operation_guard lock{safe_op};
            auto elem_it = find_elem(key);

            if (elem_it == cache_items_map.end()) {
                // add new element to the cache
                if (cache_items_map.size() + 1 > max_cache_size) {
                    auto disp_candidate_key = cache_policy.repl_candidate();

                    erase(disp_candidate_key, cb);
                }

                insert(key, value);
            } else {
                // update previous value
                update(key, value);
            }
        }

        /**
         * \brief Try to get an element by the given key from the cache
         * \param[in] key Get element by key
         * \return Pair of iterator that points to the element and boolean value that shows
         * whether get operation has been successful or not. If pair's boolean value is false,
         * the element is not presented in the cache. If pair's boolean value is true,
         * returned iterator can be used to get access to the element
         */
        std::pair<value_type, bool> try_get(const Key &key) const noexcept {
            operation_guard lock{safe_op};
            const auto result = get_internal(key);

            return std::make_pair(result.second ? result.first->second : nullptr,
                                  result.second);
        }

        /**
         * \brief Get element from the cache if present
         * \warning This method will change in the future with an optional class capabilities
         * to avoid throwing exceptions
         * \throw std::range_error
         * \param[in] key Get element by key
         * \return Reference to the value stored by the specified key in the cache
         */
        value_type get_or_die(const Key &key) const {
            operation_guard lock{safe_op};
            auto elem = get_internal(key);

            if (elem.second) {
                return elem.first->second;
            } else {
                throw std::range_error{"No such element in the cache"};
            }
        }

        /**
         * \brief Check whether the given key is presented in the cache
         * \param[in] key Element key to check
         * \retval true Element is presented in the case
         * \retval false Element is not presented in the case
         */
        bool contains(const Key &key) const noexcept {
            operation_guard lock{safe_op};
            return find_elem(key) != cache_items_map.cend();
        }

        /**
         * \brief Get number of elements in cache
         * \return Number of elements currently stored in the cache
         */
        std::size_t size() const {
            operation_guard lock{safe_op};

            return cache_items_map.size();
        }

        /**
         * Remove an element specified by key
         * \param[in] key Key parameter
         * \retval true if an element specified by key was found and deleted
         * \retval false if an element is not present in a cache
         */
        bool remove(const Key &key, on_erase_cb cb = nullptr) {
            operation_guard lock{safe_op};

            auto elem = find_elem(key);

            if (elem == cache_items_map.end()) {
                return false;
            }

            erase_itr(elem,cb);

            return true;
        }

        void set_prune_callback(on_erase_cb cb) {
            operation_guard lock{safe_op};
            on_erase_callback = cb;
        }

        size_t prune(size_t size_to_reserve, on_erase_cb cb = nullptr) {
            operation_guard lock{safe_op};
            size_t pruned = 0;
            while (cache_items_map.size() > size_to_reserve) {
                auto disp_candidate_key = cache_policy.repl_candidate();
                erase(disp_candidate_key, cb);
                pruned++;
            }
            return pruned;
        }

    protected:
        void clear() {
            operation_guard lock{safe_op};

            std::for_each(begin(), end(),
                          [&](const std::pair<const Key, value_type> &el) { cache_policy.erase(el.first); });
            cache_items_map.clear();
        }

        const_iterator begin() const noexcept {
            return cache_items_map.cbegin();
        }

        const_iterator end() const noexcept {
            return cache_items_map.cend();
        }

    protected:
        void insert(const Key &key, const Value &value) {
            cache_policy.insert(key);
            cache_items_map.emplace(std::make_pair(key, std::make_shared<Value>(value)));
        }

        void erase_itr(const_iterator elem, on_erase_cb cb) {
            auto prune_cb = cb ? cb : on_erase_callback;
            cache_policy.erase(elem->first);
            prune_cb(elem->first, elem->second);
            cache_items_map.erase(elem);
        }

        void erase(const Key &key, on_erase_cb cb) {
            auto elem_it = find_elem(key);

            erase_itr(elem_it, cb);
        }

        void update(const Key &key, const Value &value) {
            cache_policy.touch(key);
            cache_items_map[key] = std::make_shared<Value>(value);
        }

        const_iterator find_elem(const Key &key) const {
            return cache_items_map.find(key);
        }

        std::pair<const_iterator, bool> get_internal(const Key &key) const noexcept {
            auto elem_it = find_elem(key);

            if (elem_it != end()) {
                cache_policy.touch(key);
                return {elem_it, true};
            }

            return {elem_it, false};
        }

    private:
        map_type cache_items_map;
        mutable Policy<Key> cache_policy;
        mutable std::mutex safe_op;
        std::size_t max_cache_size;
        on_erase_cb on_erase_callback;
    };
} // namespace turbo

