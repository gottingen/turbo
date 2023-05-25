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

#ifndef TURBO_CONCURRENT_HASH_LOCK_H_
#define TURBO_CONCURRENT_HASH_LOCK_H_

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <memory>
#include <vector>
#include <string_view>
#include "turbo/platform/port.h"


namespace turbo {


    class HashLock {
    public:
        explicit HashLock(int hash_power);

        ~HashLock() = default;

        HashLock(const HashLock &) = delete;

        HashLock &operator=(const HashLock &) = delete;

        unsigned Size() const;

        void Lock(const std::string_view &key);

        void UnLock(const std::string_view &key);

        void LockShared(const std::string_view &key);

        void UnLockShared(const std::string_view &key);

        std::vector<std::shared_mutex *> MultiGet(const std::vector<std::string> &keys);

    private:
        int hash_power_;
        unsigned hash_mask_;
        std::vector<std::unique_ptr<std::shared_mutex>> mutex_pool_;

        unsigned HashImpl(const std::string_view &key) const;
    };

    class SharedLockGuard {
    public:
        explicit SharedLockGuard(HashLock *lock_mgr, std::string_view key) : lock_mgr_(lock_mgr), key_(key) {
            lock_mgr->Lock(key_);
        }

        ~SharedLockGuard() { lock_mgr_->UnLock(key_); }

        SharedLockGuard(const SharedLockGuard &) = delete;

        SharedLockGuard &operator=(const SharedLockGuard &) = delete;

    private:
        HashLock *lock_mgr_ = nullptr;
        std::string_view key_;
    };


    class LockGuard {
    public:
        explicit LockGuard(HashLock *lock_mgr, std::string_view key) : lock_mgr_(lock_mgr), key_(key) {
            lock_mgr->Lock(key_);
        }

        ~LockGuard() { lock_mgr_->UnLock(key_); }

        LockGuard(const LockGuard &) = delete;

        LockGuard &operator=(const LockGuard &) = delete;

    private:
        HashLock *lock_mgr_ = nullptr;
        std::string_view key_;
    };

    class MultiSharedLockGuard {
    public:
        explicit MultiSharedLockGuard(HashLock *lock_mgr, const std::vector<std::string> &keys) : lock_mgr_(lock_mgr) {
            locks_ = lock_mgr_->MultiGet(keys);
            for (const auto &iter: locks_) {
                iter->lock_shared();
            }
        }

        ~MultiSharedLockGuard() {
            for (auto iter = locks_.rbegin(); iter != locks_.rend(); ++iter) {
                (*iter)->unlock_shared();
            }
        }

        MultiSharedLockGuard(const MultiSharedLockGuard &) = delete;

        MultiSharedLockGuard &operator=(const MultiSharedLockGuard &) = delete;

    private:
        HashLock *lock_mgr_ = nullptr;
        std::vector<std::shared_mutex *> locks_;
    };

    class MultiLockGuard {
    public:
        explicit MultiLockGuard(HashLock *lock_mgr, const std::vector<std::string> &keys) : lock_mgr_(lock_mgr) {
            locks_ = lock_mgr_->MultiGet(keys);
            for (const auto &iter: locks_) {
                iter->lock();
            }
        }

        ~MultiLockGuard() {
            for (auto iter = locks_.rbegin(); iter != locks_.rend(); ++iter) {
                (*iter)->unlock();
            }
        }

        MultiLockGuard(const MultiLockGuard &) = delete;

        MultiLockGuard &operator=(const MultiLockGuard &) = delete;

    private:
        HashLock *lock_mgr_ = nullptr;
        std::vector<std::shared_mutex *> locks_;
    };


}
#endif  // TURBO_CONCURRENT_HASH_LOCK_H_
