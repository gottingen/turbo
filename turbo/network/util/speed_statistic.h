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

#ifndef TURBO_NETWORK_UTIL_SPEED_STATISTIC_H_
#define TURBO_NETWORK_UTIL_SPEED_STATISTIC_H_

#include "turbo/network/util/time_ticker.h"


namespace turbo {

class BytesSpeed {
public:
    BytesSpeed() = default;
    ~BytesSpeed() = default;

    /**
     * 添加统计字节
     */
    BytesSpeed &operator+=(size_t bytes) {
        _bytes += bytes;
        if (_bytes > 1024 * 1024) {
            //数据大于1MB就计算一次网速
            computeSpeed();
        }
        return *this;
    }

    /**
     * 获取速度，单位bytes/s
     */
    int getSpeed() {
        if (_ticker.elapsedTime() < 1000) {
            //获取频率小于1秒，那么返回上次计算结果
            return _speed;
        }
        return computeSpeed();
    }

private:
    int computeSpeed() {
        auto elapsed = _ticker.elapsedTime();
        if (!elapsed) {
            return _speed;
        }
        _speed = (int)(_bytes * 1000 / elapsed);
        _ticker.resetTime();
        _bytes = 0;
        return _speed;
    }

private:
    int _speed = 0;
    size_t _bytes = 0;
    Ticker _ticker;
};

} /* namespace turbo */
#endif /* TURBO_NETWORK_UTIL_SPEED_STATISTIC_H_ */