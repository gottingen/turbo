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
#include "turbo/base/thread.h"
#include "turbo/log/logging.h"

namespace turbo {

    SimpleThread::SimpleThread(const std::string& name_prefix) : name_prefix_(name_prefix) , name_(){

    }

    SimpleThread::~SimpleThread() {
        TLOG_CHECK(!has_been_joined(), "thread has been started");
    }
    bool SimpleThread::has_been_started() const {
        return status_ == kThreadRunning;
    }

    bool SimpleThread::has_been_joined() const {
        return status_ == kThreadJoined || status_ == kThreadNotStarted;

    }
    turbo::Status SimpleThread::start() {
        TLOG_CHECK(!has_been_started(), "thread has been started");
        std::unique_lock lock(mu_);
        auto r = thread_main();
        if(!r.ok()){
            return r;
        }
        cond_.wait(lock, [this] { return has_been_started(); });
        return turbo::ok_status();

    }

    void SimpleThread::stop() {
        _stop = true;
        if(status_ >= kThreadStopped) {
            return;
        }
        status_ = kThreadStopped;
    }

    void SimpleThread::join() {
        if(status_ == kThreadNotStarted){
            return;
        }
        if(status_ == kThreadStopped){
            PlatformThread::join(thread_);
        }
        status_ = kThreadJoined;

    }

}  // namespace turbo
