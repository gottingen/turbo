
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_CACHE_BUCKET_H_
#define FLARE_CONTAINER_CACHE_BUCKET_H_

#include <shared_mutex>
#include <tuple>
#include <unordered_map>

#include "flare/container/cache/item.h"

namespace flare {

    template<class Key, class Value, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
    class cache_bucket {
    public:
        inline cache_item_ptr <Key, Value> set(const Key &key, const Value &value, const uint32_t &expire_sec,
                                        cache_item_ptr <Key, Value> &existing) {
            auto expires_time_point = std::chrono::steady_clock::now() + std::chrono::seconds(expire_sec);
            auto item = std::make_shared<cache_item < Key, Value>>
            (key, value, expires_time_point);
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto it = lookup_.find(key);
                if (it != lookup_.end()) {
                    existing = it->second;
                }
                lookup_[item->key()] = item;
            }
            return item;
        }

        inline cache_item_ptr <Key, Value> remove(const Key &key) {
            cache_item_ptr <Key, Value> item = nullptr;
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto it = lookup_.find(key);
            if (it != lookup_.end()) {
                item = it->second;
                lookup_.erase(it);
            }
            return item;
        }

        bool remove(const cache_item_ptr <Key, Value> &item) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto it = lookup_.find(item->key());
            if (it != lookup_.end() && it->second == item) {
                lookup_.erase(it);
                return true;
            }
            return false;
        }

        cache_item_ptr <Key, Value> get(const Key &key) const {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto it = lookup_.find(key);
            if (it != lookup_.end()) {
                cache_hit_count_++;
                return it->second;
            }
            cache_miss_count_++;
            return nullptr;
        }

        inline void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_, std::try_to_lock);
            if (!lock.owns_lock()) {
                return;
            }
            lookup_.clear();
            cache_hit_count_ = 0;
            cache_miss_count_ = 0;
        }

        inline size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return lookup_.size();
        }

        inline std::tuple<uint64_t, uint64_t> keyspace_stats() const {
            return std::make_tuple<uint64_t, uint64_t>(std::move(cache_hit_count_), std::move(cache_miss_count_));
        }

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<Key, cache_item_ptr < Key, Value>, Hash, KeyEqual>
        lookup_;
        mutable uint64_t cache_hit_count_ = 0;
        mutable uint64_t cache_miss_count_ = 0;
    };

}  // namespace flare

#endif  // FLARE_CONTAINER_CACHE_BUCKET_H_
