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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>  // NOLINT(build/c++11)
#include <utility>
#include <vector>

#include <turbo/base/thread_annotations.h>
#include <turbo/functional/any_invocable.h>
#include <turbo/synchronization/mutex.h>

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
      turbo::MutexLock l(&mu_);
      for (size_t i = 0; i < threads_.size(); i++) {
        queue_.push(nullptr);  // Shutdown signal.
      }
    }
    for (auto &t : threads_) {
      t.join();
    }
  }

  // Schedule a function to be run on a ThreadPool thread immediately.
  void Schedule(turbo::AnyInvocable<void()> func) {
    assert(func != nullptr);
    turbo::MutexLock l(&mu_);
    queue_.push(std::move(func));
  }

 private:
  bool WorkAvailable() const TURBO_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
    return !queue_.empty();
  }

  void WorkLoop() {
    while (true) {
      turbo::AnyInvocable<void()> func;
      {
        turbo::MutexLock l(&mu_);
        mu_.Await(turbo::Condition(this, &ThreadPool::WorkAvailable));
        func = std::move(queue_.front());
        queue_.pop();
      }
      if (func == nullptr) {  // Shutdown signal.
        break;
      }
      func();
    }
  }

  turbo::Mutex mu_;
  std::queue<turbo::AnyInvocable<void()>> queue_ TURBO_GUARDED_BY(mu_);
  std::vector<std::thread> threads_;
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_THREAD_POOL_H_
