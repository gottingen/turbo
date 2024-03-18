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

#include <fcntl.h>
#include "turbo/network/poller/pipe.h"
#include "turbo/network/network/sock_util.h"

using namespace std;

namespace turbo {

Pipe::Pipe(const onRead &cb, const EventPoller::Ptr &poller) {
    _poller = poller;
    if (!_poller) {
        _poller = EventPollerPool::Instance().getPoller();
    }
    _pipe = std::make_shared<PipeWrap>();
    auto pipe = _pipe;
    _poller->addEvent(_pipe->readFD(), EventPoller::Event_Read, [cb, pipe](int event) {
#if defined(_WIN32)
        unsigned long nread = 1024;
#else
        int nread = 1024;
#endif //defined(_WIN32)
        ioctl(pipe->readFD(), FIONREAD, &nread);
#if defined(_WIN32)
        std::shared_ptr<char> buf(new char[nread + 1], [](char *ptr) {delete[] ptr; });
        buf.get()[nread] = '\0';
        nread = pipe->read(buf.get(), nread + 1);
        if (cb) {
            cb(nread, buf.get());
        }
#else
        char buf[nread + 1];
        buf[nread] = '\0';
        nread = pipe->read(buf, sizeof(buf));
        if (cb) {
            cb(nread, buf);
        }
#endif // defined(_WIN32)
    });
}

Pipe::~Pipe() {
    if (_pipe) {
        auto pipe = _pipe;
        _poller->delEvent(pipe->readFD(), [pipe](bool success) {});
    }
}

void Pipe::send(const char *buf, int size) {
    _pipe->write(buf, size);
}

}  // namespace turbo