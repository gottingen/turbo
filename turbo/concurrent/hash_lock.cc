// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/concurrent/hash_lock.h"

#include <set>
#include <string>
#include <thread>

namespace turbo {


    HashLock::HashLock(int hash_power) : hash_power_(hash_power), hash_mask_((1U << hash_power) - 1) {
        for (unsigned i = 0; i < Size(); i++) {
            mutex_pool_.emplace_back(new std::shared_mutex{});
        }
    }

    unsigned HashLock::HashImpl(const std::string_view &key) const {
        return std::hash<std::string_view>{}(std::string_view{key.data(), key.size()}) & hash_mask_;
    }

    unsigned HashLock::Size() const { return (1U << hash_power_); }

    void HashLock::Lock(const std::string_view &key) { mutex_pool_[HashImpl(key)]->lock(); }

    void HashLock::UnLock(const std::string_view &key) { mutex_pool_[HashImpl(key)]->unlock(); }

    void HashLock::LockShared(const std::string_view &key) { mutex_pool_[HashImpl(key)]->lock_shared(); }

    void HashLock::UnLockShared(const std::string_view &key) { mutex_pool_[HashImpl(key)]->unlock_shared(); }

    std::vector<std::shared_mutex *> HashLock::MultiGet(const std::vector<std::string> &keys) {
        std::set<unsigned, std::greater<unsigned>> to_acquire_indexes;
        // We are using the `set` to avoid retrieving the mutex, as well as guarantee to retrieve
        // the order of locks.
        //
        // For example, we need lock the key `A` and `B` and they have the same lock HashImpl
        // index, it will be deadlock if lock the same mutex twice. Besides, we also need
        // to order the mutex before acquiring locks since different threads may acquire
        // same keys with different order.
        for (const auto &key: keys) {
            to_acquire_indexes.insert(HashImpl(key));
        }

        std::vector<std::shared_mutex *> locks;
        locks.reserve(to_acquire_indexes.size());
        for (auto index: to_acquire_indexes) {
            locks.emplace_back(mutex_pool_[index].get());
        }
        return locks;
    }

}  // namespace turbo