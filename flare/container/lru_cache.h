
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_LRU_CACHE_H_
#define FLARE_CONTAINER_LRU_CACHE_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>

#include "flare/container/cache/bucket.h"
#include "flare/container/parallel_ring_queue.h"
#include "flare/container/cache/config.h"
#include "flare/container/cache/item.h"
#include "flare/container/cache/policy.h"
#include "flare/container/cache/ram_policy.h"

namespace flare {


    template<class Key, class Value, uint8_t bucket_bits = 5, class Hash = std::hash<Key>,
            class KeyEqual = std::equal_to<Key>>
    class lru_cache {
        using ItemQueue = parallel_ring_queue<cache_item_ptr<Key, Value>>;

        static constexpr uint32_t kBucketsNum = (1 << bucket_bits);
        static constexpr uint32_t kBucketMask = (kBucketsNum - 1);

    public:

        lru_cache() {}

        explicit lru_cache(const cache_config &config) : cfg_(config) {}


        ~lru_cache() { stop(); }

        template<typename KeyEstimator = ram_usage<Key>, typename ValueEstimator = ram_usage<Key>>
        inline void use_ram_policy(int64_t max_ram_bytes_used = kDefaultMaxRamBytesUsed) {
            policy_ = std::make_unique<ram_cache_policy<Key, Value, KeyEstimator, ValueEstimator>>
                    (max_ram_bytes_used,
                     std::bind(&lru_cache<Key, Value, bucket_bits, Hash, KeyEqual>::notify_gc, this));
        }

        const cache_item_ptr<Key, Value> get(const Key &key) {
            auto item = get_bucket(key).get(key);
            if (item == nullptr) {
                return nullptr;
            }

            if (item->expired()) {
                del(key);
                return nullptr;
            }
            promote_buffer_.push_back(item);
            return item;
        }

        const cache_item_ptr<Key, Value> set(const Key &key, const Value &value) {
            return set(key, value, std::numeric_limits<double>::max());
        }

        const cache_item_ptr<Key, Value> set(const Key &key, const Value &value, double gen_item_time_threshold) {
            return set(key, value, cfg_.item_expire_sec_, gen_item_time_threshold);
        }

        const cache_item_ptr<Key, Value> set(const Key &key, const Value &value, uint32_t expire_sec,
                                             double gen_item_time_threshold = std::numeric_limits<double>::max()) {
            if (gen_item_time_threshold < cfg_.item_gen_time_threshold_ms_) {
                return nullptr;
            }
            cache_item_ptr<Key, Value> existing = nullptr;
            auto item = get_bucket(key).set(key, value, expire_sec, existing);
            if (!promote_buffer_.push_back(item)) {
                get_bucket(item->key()).remove(item);
                return nullptr;
            }
            if (existing != nullptr) {
                delete_buffer_.push_back(existing);
            }
            on_cache_set(item);
            return item;
        }

        bool del(const Key &key) {
            auto item = get_bucket(key).remove(key);
            if (item != nullptr) {
                delete_buffer_.push_back(item);
                on_cache_del(item);
                return true;
            }
            return false;
        }

        template<class ValGenFunc, typename... Args>
        const cache_item_ptr<Key, Value> get_or_set(const Key &key, uint32_t expire_sec, const ValGenFunc &val_gen_func,
                                                    const Args &... args) {
            auto item = get(key);
            if (item == nullptr) {
                return set(key, val_gen_func(args...), expire_sec);
            }

            return item;
        }

        void clear() {
            for (auto &&bucket : buckets_) {
                bucket.clear();
            }
            item_num_ = 0;
            list_.clear();
            policy_->clear();
        }

        void stop() {
            if (!stop_) {
                stop_ = true;
                worker_.join();
            }
        }

        void start() {
            if (!stop_) {
                return;
            }
            init();
            stop_ = false;
            worker_ = std::thread(&lru_cache::ThreadFunc, this);
        }

        inline size_t size() const { return item_num_.load(); }

        inline size_t item_num_in_bucket() {
            size_t item_num = 0;
            for (auto &&bucket : buckets_) {
                item_num += bucket.size();
            }
            return item_num;
        }

        std::string dump() const {
            auto[cache_hit_count, cache_miss_count] = keyspace_stats();
            std::stringstream ss;
            ss << "{\"cache\":{\"policy\":" << policy_->to_string()
               << ",\"statistic\":{\"cache_stats\":{\"cache_hit_count\":" << cache_hit_count
               << ",\"cache_miss_count\":" << cache_miss_count << "}}}}";
            return ss.str();
        }

        inline std::tuple<uint64_t, uint64_t> keyspace_stats() const {
            uint64_t cache_hit_count = 0;
            uint64_t cache_miss_count = 0;
            for (auto &bucket : buckets_) {
                auto[hit, miss] = bucket.keyspace_stats();
                cache_hit_count += hit;
                cache_miss_count += miss;
            }

            return std::make_tuple<uint64_t, uint64_t>(std::move(cache_hit_count), std::move(cache_miss_count));
        }

    private:

        void on_cache_set(const cache_item_ptr<Key, Value> &item) { policy_->on_cache_set(item->key(), item->value()); }

        void on_cache_del(const cache_item_ptr<Key, Value> &item) { policy_->on_cache_del(item->key(), item->value()); }

        void notify_gc() { gc_flag_ = true; }

        void init() {
            if (cfg_.delete_buffer_len_ != delete_buffer_.capacity()) {
                delete_buffer_.reserve(cfg_.delete_buffer_len_);
            }
            if (cfg_.promote_buffer_len_ != promote_buffer_.capacity()) {
                promote_buffer_.reserve(cfg_.promote_buffer_len_);
            }
            if (cfg_.max_item_num_ == 0) {
                cfg_.max_item_num_ = cache_config::kDefaultMaxItemNum;
            }
            if (cfg_.prune_batch_size_ == 0) {
                cfg_.max_item_num_ = cache_config::kDefaultPruneBatchSize;
            }
            if (cfg_.item_expire_sec_ == 0) {
                cfg_.max_item_num_ = cache_config::kDefaultCacheItemExpireSec;
            }
            FLARE_LOG(INFO) << "init cache config successfully, max_item_num: "
                            << cfg_.max_item_num_ << ", prune_batch_size_: " << cfg_.prune_batch_size_
                            << ", promote_per_times_: " << cfg_.promote_per_times_ << ", delete_buffer_len_: "
                            << cfg_.delete_buffer_len_
                            << ", promote_buffer_len: " << cfg_.promote_buffer_len_ << ", item_expire_sec_: "
                            << cfg_.item_expire_sec_
                            << ", item_gen_time_threshold_ms_: " << cfg_.item_gen_time_threshold_ms_
                            << ", worker_sleep_ms_: " << cfg_.worker_sleep_ms_;
        }

        inline cache_bucket<Key, Value, Hash, KeyEqual> &get_bucket(const Key &key) {
            auto h = Hash()(key);
            return buckets_[h & kBucketMask];
        }

        void do_delete(cache_item_ptr<Key, Value> &item) {
            if (item->list_iter_ == nullptr) {
                item->set_deleted();
            } else {
                item_num_--;
                list_.erase(*item->list_iter_);
                item->list_iter_ = nullptr;
            }
        }

        bool do_promote(cache_item_ptr<Key, Value> &item) {
            if (item->is_delete()) {
                return false;
            }
            if (item->list_iter_ != nullptr) {
                auto times = cfg_.promote_per_times_;
                item->incr_promote_times();
                if (item->should_promote(times)) {
                    list_.splice(list_.begin(), list_, *item->list_iter_);
                    item->reset_status();
                }
                return false;
            }
            item_num_++;
            list_.push_front(item);
            item->list_iter_ = std::make_unique<cache_list_iterator<Key, Value>>
                    (list_.begin());
            return true;
        }

        void gc() {
            FLARE_LOG(DEBUG) << "cache do gc, list_size: " << list_.size();
            for (uint32_t i = 0; i < cfg_.prune_batch_size_ && !list_.empty(); ++i) {
                auto item = list_.back();
                get_bucket(item->key()).remove(item);
                item_num_--;
                item->set_deleted();
                item->list_iter_ = nullptr;
                list_.pop_back();
                on_cache_del(item);
            }
        }

        void ThreadFunc() {
            while (!stop_) {
                if (gc_flag_) {
                    gc();
                    gc_flag_ = false;
                }

                if (auto[stat, item] = promote_buffer_.pop_front(); stat && item != nullptr) {
                    if (do_promote(item) && item_num_.load() > cfg_.max_item_num_) {
                        notify_gc();
                    }
                } else if (auto[stat, item] = delete_buffer_.pop_front(); stat && item != nullptr) {
                    do_delete(item);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.worker_sleep_ms_));
            }
        }

    private:
        std::list<cache_item_ptr<Key, Value>> list_;
        std::atomic<int64_t> item_num_ = 0;
        std::array<cache_bucket<Key, Value, Hash, KeyEqual>, kBucketsNum>
                buckets_;
        ItemQueue delete_buffer_;
        ItemQueue promote_buffer_;
        std::atomic<bool> gc_flag_;
        std::atomic_bool stop_ = true;
        std::thread worker_;
        cache_config cfg_;
        cache_policy_ptr<Key, Value> policy_ = std::make_unique<empty_cache_policy<Key, Value>>
                ();
    };

}  // namespace flare

#endif  // FLARE_CONTAINER_LRU_CACHE_H_
