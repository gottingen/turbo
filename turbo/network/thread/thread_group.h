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

#ifndef TURBO_NETWORK_THREAD_THREAD_GROUP_H_
#define TURBO_NETWORK_THREAD_THREAD_GROUP_H_

#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <memory>

namespace turbo {

class thread_group {
    private:
        thread_group(thread_group const &);
        thread_group & operator=(thread_group const &);

    public:
        thread_group() {}
        ~thread_group() {
            _threads.clear();
        }

        bool is_this_thread_in() const {
            auto thread_id = std::this_thread::get_id();
            if (_thread_id == thread_id) {
                return true;
            }
            return _threads.find(thread_id) != _threads.end();
        }

        bool is_thread_in(std::thread *thrd) {
            if (!thrd) {
                return false;
            }
            auto it = _threads.find(thrd->get_id());
            return it != _threads.end();
        }

        template<typename F>
        std::thread *create_thread(F &&threadfunc) {
            auto thread_new = std::make_shared<std::thread>(std::forward<F>(threadfunc));
            _thread_id = thread_new->get_id();
            _threads[_thread_id] = thread_new;
            return thread_new.get();
        }

        void remove_thread(std::thread *thrd) {
            auto it = _threads.find(thrd->get_id());
            if (it != _threads.end()) {
                _threads.erase(it);
            }
        }

        void join_all() {
            if (is_this_thread_in()) {
                throw std::runtime_error("Trying joining itself in thread_group");
            }
            for (auto &it : _threads) {
                if (it.second->joinable()) {
                    it.second->join(); //等待线程主动退出
                }
            }
            _threads.clear();
        }

        size_t size() {
            return _threads.size();
        }

    private:
        std::thread::id _thread_id;
        std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> _threads;
};

}  // namespace turbo

#endif //TURBO_NETWORK_THREAD_THREAD_GROUP_H_