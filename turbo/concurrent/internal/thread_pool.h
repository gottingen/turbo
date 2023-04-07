// Copyright 2020 The Turbo Authors.
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

#ifndef TURBO_CONCURRENT_INTERNAL_THREAD_POOL_H_
#define TURBO_CONCURRENT_INTERNAL_THREAD_POOL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>  // NOLINT(build/c++11)
#include <utility>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "turbo/platform/thread_annotations.h"
#include "turbo/meta/any_invocable.h"
#include "turbo/concurrent/notification.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace synchronization_internal {

        // A simple ThreadPool implementation for tests.
        class ThreadPool {
        public:
            explicit ThreadPool(int num_threads) {
                threads_.reserve(num_threads);
                for (int i = 0; i < num_threads; ++i) {
                    threads_.push_back(std::thread(&ThreadPool::WorkLoop, this));
                }
            }

            ThreadPool(const ThreadPool &) = delete;

            ThreadPool &operator=(const ThreadPool &) = delete;

            ~ThreadPool() {
                {
                    stop_ = true;
                    for(size_t i = 0; i < threads_.size(); ++i) {
                        StopOne(nullptr);
                    }
                    cv_.notify_all();
                }
                for (auto &t: threads_) {
                    t.join();
                }
            }

            // Schedule a function to be run on a ThreadPool thread immediately.
            void Schedule(turbo::AnyInvocable<void()> func) {
                assert(func != nullptr);
                std::unique_lock l(mu_);
                queue_.push(std::move(func));
                cv_.notify_one();
            }
            void StopOne(turbo::AnyInvocable<void()> func) {
                std::unique_lock l(mu_);
                queue_.push(std::move(func));
                cv_.notify_one();
            }

        private:
            bool WorkAvailable() const TURBO_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
                return !queue_.empty();
            }

            void WorkLoop() {
                while (!stop_) {
                    turbo::AnyInvocable<void()> func;
                    {
                        std::unique_lock l(mu_);
                        cv_.wait(l);
                        func = std::move(queue_.front());
                        queue_.pop();
                    }
                    if (func == nullptr) {  // Shutdown signal.
                        break;
                    }
                    func();
                }
            }

            std::mutex mu_;
            std::condition_variable cv_;
            bool stop_{false};
            std::queue<turbo::AnyInvocable<void()>> queue_ TURBO_GUARDED_BY(mu_);
            std::vector<std::thread> threads_;
        };

    }  // namespace synchronization_internal
    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONCURRENT_INTERNAL_THREAD_POOL_H_
