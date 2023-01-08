
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_CACHE_RAW_CONFIG_H_
#define FLARE_CONTAINER_CACHE_RAW_CONFIG_H_

#include <atomic>
#include <functional>
#include <sstream>

#include "flare/container/cache/item.h"
#include "flare/container/cache/policy.h"
#include "flare/log/logging.h"

namespace flare {

    // default 32MB
    static constexpr int64_t kDefaultMaxRamBytesUsed = 1L << 25;

    template<class T>
    struct ram_usage {
        uint64_t operator()(const T &t) const { return sizeof(t); }
    };

    template<class Key, class Value, class KeyEstimator = ram_usage<Key>, class ValueEstimator = ram_usage<Key>>
    class ram_cache_policy final : public cache_policy<Key, Value> {
    public:
        static constexpr uint64_t kCacheItemBaseSize = sizeof(cache_item < Key, Value >) - sizeof(Key) - sizeof(Value);

        ram_cache_policy(const uint64_t &max_ram_bytes_used, const std::function<void(void)> &callback) : callback_(
                callback) {
            if (max_ram_bytes_used <= 0) {
                max_ram_bytes_used_ = kDefaultMaxRamBytesUsed;
            } else {
                max_ram_bytes_used_ = max_ram_bytes_used;
            }
        }

        void on_cache_set(const Key &key, const Value &value) override {
            uint64_t key_size = key_ram_usage_estimator_(key);
            uint64_t value_size = value_ram_usage_estimator_(value);
            ram_bytes_used_ += (key_size + value_size + kCacheItemBaseSize);
            FLARE_LOG(DEBUG)<<"cache set, ram_bytes_used_: " << ram_bytes_used_;
            if (ram_bytes_used_.load() >= max_ram_bytes_used_) {
                callback_();
            }
        }

        void on_cache_del(const Key &key, const Value &value) override {
            uint64_t key_size = key_ram_usage_estimator_(key);
            uint64_t value_size = value_ram_usage_estimator_(value);
            ram_bytes_used_ -= (key_size + value_size + kCacheItemBaseSize);
        }

        inline void clear() override { ram_bytes_used_ = 0; }

        std::string to_string() const override {
            std::stringstream ss;
            double ram_bytes_used = ram_bytes_used_.load() <= 0 ? 0 : ram_bytes_used_.load();
            ss << "{\"ram_cache_policy\":{\"max_ram_bytes_used\":" << max_ram_bytes_used_
               << ",\"ram_bytes_used\":" << ram_bytes_used << ",\"\%usage\":" << ram_bytes_used / max_ram_bytes_used_
               << "}}";
            return ss.str();
        }

    private:
        int64_t max_ram_bytes_used_ = kDefaultMaxRamBytesUsed;  // 最大的内存使用量，按字节计算
        std::function<void(void)> callback_;  // 回调函数
        KeyEstimator key_ram_usage_estimator_;  // key 的 ram 占用估算器
        ValueEstimator value_ram_usage_estimator_;  // value 的 ram 占用估算器
        std::atomic<int64_t> ram_bytes_used_ = 0;  // 使用的内存量，按字节计算
    };
}  // namespace flare

#endif  // FLARE_CONTAINER_CACHE_RAW_CONFIG_H_
