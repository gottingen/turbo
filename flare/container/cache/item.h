
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_CACHE_ITEM_H_
#define FLARE_CONTAINER_CACHE_ITEM_H_

#include <chrono>
#include <cstdint>
#include <ctime>
#include <list>
#include <memory>

namespace flare {

    template<class Key, class Value>
    class cache_item;

    template<class Key, class Value>
    using cache_item_ptr = std::shared_ptr<cache_item<Key, Value>>;

    template<class Key, class Value>
    using cache_list_iterator = typename std::list<cache_item_ptr<Key, Value>>::iterator;

    template<class Key, class Value>
    class cache_item {
    public:

        cache_item(const Key &key, const Value &value, const std::chrono::steady_clock::time_point &expires_time_point)
                : key_(key), value_(value), expires_time_point_(expires_time_point) {}

        inline bool expired() const { return expires_time_point_ < std::chrono::steady_clock::now(); }

        inline bool should_promote(uint32_t promote_per_times) {
            return !is_delete_ && promote_times_ >= promote_per_times;
        }

        inline void incr_promote_times() { promote_times_++; }

        inline void reset_status() {
            promote_times_ = 0;
            is_delete_ = false;
        }

        inline void set_deleted() { is_delete_ = true; }

        inline const Key &key() const { return key_; }

        inline const Value &value() const { return value_; }

        /**
         * @brief 是否为已删除状态
         * @return 是否为已删除状态
         */
        inline bool is_delete() const { return is_delete_; }

    private:

        template<class K, class V, uint8_t B, class H, class E>
        friend
        class Cache;

        Key key_;
        Value value_;
        uint32_t promote_times_ = 0;
        bool is_delete_ = false;
        std::chrono::steady_clock::time_point expires_time_point_;
        std::unique_ptr<cache_list_iterator<Key, Value>> list_iter_ = nullptr;
    };

}  // namespace flare

#endif  // FLARE_CONTAINER_CACHE_ITEM_H_
