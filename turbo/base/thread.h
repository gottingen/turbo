// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
// Created by jeff on 24-1-2.
//

#ifndef TURBO_BASE_THREAD_H_
#define TURBO_BASE_THREAD_H_

#include "turbo/system/threading.h"
#include <mutex>
#include <string>
#include <condition_variable>
#include "turbo/status/status.h"

namespace turbo {

    struct ThreadOptions {
        size_t stack_size{0};
    };

    enum ThreadStatus {
        kThreadNotStarted,
        kThreadStarting,
        kThreadRunning,
        kThreadStopped,
        kThreadJoined,
    };

    class SimpleThread {
    public:


        explicit SimpleThread(const std::string& name_prefix);

        SimpleThread(const std::string& name_prefix, const ThreadOptions& options);

        virtual ~SimpleThread();

        virtual void run() = 0;

        turbo::Status start();

        void stop();

        void join();

        virtual turbo::Status thread_main();

        std::string name_prefix() const { return name_prefix_; }

        bool has_been_started() const;
        bool has_been_joined() const;
    private:
        const std::string name_prefix_;
        std::string name_;
        const ThreadOptions options_;
        PlatformThreadHandle thread_;
        std::mutex mu_;
        std::condition_variable cond_;
        PlatformThreadId tid_;
        bool _stop{false};
        ThreadStatus status_{ThreadStatus::kThreadNotStarted};
    };
}  // namespace turbo

#endif  // TURBO_BASE_THREAD_H_
