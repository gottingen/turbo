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

#ifndef TURBO_NETWORK_POLLER_PIPE_H_
#define TURBO_NETWORK_POLLER_PIPE_H_

#include <functional>
#include "turbo/network/poller/pipe_wrap.h"
#include "turbo/network/poller/event_poller.h"


namespace turbo {

class Pipe {
public:
    using onRead = std::function<void(int size, const char *buf)>;

    Pipe(const onRead &cb = nullptr, const EventPoller::Ptr &poller = nullptr);
    ~Pipe();

    void send(const char *send, int size = 0);

private:
    std::shared_ptr<PipeWrap> _pipe;
    EventPoller::Ptr _poller;
};

}  // namespace turbo
#endif /* TURBO_NETWORK_POLLER_PIPE_H_ */