// Copyright 2023 The turbo Authors.
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
//

#ifndef TURBO_NETWORK_THREAD_SEMAPHORE_H_
#define TURBO_NETWORK_THREAD_SEMAPHORE_H_

#include <mutex>
#include <condition_variable>

namespace turbo {

class semaphore {
    public:
        explicit semaphore(size_t initial = 0) {
            _count = 0;
        }

        ~semaphore() {}

        void post(size_t n = 1) {
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            _count += n;
            if(n == 1) {
                _condition.notify_one();
            }
            else {
                _condition.notify_all();
            }
        }

        void wait() {
            std::unique_lock<std::recursive_mutex> lock(_mutex);
            while(_count == 0) {
                _condition.wait(lock);
            }
            --_count;
        }

    private:
        size_t _count;
        std::recursive_mutex _mutex;
        std::condition_variable_any _condition;
};

} // namespace turbo

#endif