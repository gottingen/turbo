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
//

#include "turbo/times/stop_watcher.h"
#include <cassert>

namespace turbo {

    StopWatcher::StopWatcher() : _start(), _stop() {
        reset();
    }

    void StopWatcher::reset() {
        _start = turbo::Now();
    }

    const StopWatcher& StopWatcher::stop() {
        auto n = turbo::Now();
        if(_stop <= _start) {
            _stop = n;
        }
        return *this;
    }
    turbo::Duration StopWatcher::elapsed() const{
        TURBO_ASSERT(_stop > _start);
        return _stop - _start;
    }
}  // namespace turbo
