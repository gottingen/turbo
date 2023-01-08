
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_CACHE_CONFIG_H_
#define FLARE_CONTAINER_CACHE_CONFIG_H_

#include <cstdint>

namespace flare {

    struct cache_config {
        static constexpr uint32_t kDefaultMaxItemNum = 1024;
        static constexpr uint32_t kDefaultPruneBatchSize = 50;
        static constexpr uint32_t kDefaultPromotePerTimes = 5;
        static constexpr uint32_t kDefaultDeleteBufferLen = 1 << 10;
        static constexpr uint32_t kDefaultPromoteBufferLen = 1 << 10;
        static constexpr uint32_t kDefaultCacheItemExpireSec = 86400;
        static constexpr double kDefaultGenItemTimeThresholdMs = 0.5;
        static constexpr uint32_t kDefaultWorkerSleepMs = 100;

        uint32_t max_item_num_ = kDefaultMaxItemNum;

        uint32_t prune_batch_size_ = kDefaultPruneBatchSize;

        uint32_t promote_per_times_ = kDefaultPromotePerTimes;

        uint32_t item_expire_sec_ = kDefaultCacheItemExpireSec;

        double item_gen_time_threshold_ms_ = kDefaultGenItemTimeThresholdMs;

        uint32_t delete_buffer_len_ = kDefaultDeleteBufferLen;

        uint32_t promote_buffer_len_ = kDefaultPromoteBufferLen;

        uint32_t worker_sleep_ms_ = kDefaultWorkerSleepMs;
    };

}  // namespace flare

#endif  // FLARE_CONTAINER_CACHE_CONFIG_H_
