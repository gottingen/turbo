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

#ifndef TURBO_NETWORK_UTIL_TIME_TICKER_H_
#define TURBO_NETWORK_UTIL_TIME_TICKER_H_

#include <cassert>
#include "turbo/network/util/logger.h"

namespace turbo {

class Ticker {
public:
    /**
     * 此对象可以用于代码执行时间统计，以可以用于一般计时
     * @param min_ms 开启码执行时间统计时，如果代码执行耗时超过该参数，则打印警告日志
     * @param ctx 日志上下文捕获，用于捕获当前日志代码所在位置
     * @param print_log 是否打印代码执行时间
     */
    Ticker(uint64_t min_ms = 0,
           LogContextCapture ctx = LogContextCapture(Logger::Instance(), LWarn, __FILE__, "", __LINE__),
           bool print_log = false) : _ctx(std::move(ctx)) {
        if (!print_log) {
            _ctx.clear();
        }
        _created = _begin = getCurrentMillisecond();
        _min_ms = min_ms;
    }

    ~Ticker() {
        uint64_t tm = createdTime();
        if (tm > _min_ms) {
            _ctx << "take time: " << tm << "ms" << ", thread may be overloaded";
        } else {
            _ctx.clear();
        }
    }

    /**
     * 获取上次resetTime后至今的时间，单位毫秒
     */
    uint64_t elapsedTime() const {
        return getCurrentMillisecond() - _begin;
    }

    /**
     * 获取从创建至今的时间，单位毫秒
     */
    uint64_t createdTime() const {
        return getCurrentMillisecond() - _created;
    }

    /**
     * 重置计时器
     */
    void resetTime() {
        _begin = getCurrentMillisecond();
    }

private:
    uint64_t _min_ms;
    uint64_t _begin;
    uint64_t _created;
    LogContextCapture _ctx;
};

class SmoothTicker {
public:
    /**
     * 此对象用于生成平滑的时间戳
     * @param reset_ms 时间戳重置间隔，没间隔reset_ms毫秒, 生成的时间戳会同步一次系统时间戳
     */
    SmoothTicker(uint64_t reset_ms = 10000) {
        _reset_ms = reset_ms;
        _ticker.resetTime();
    }

    ~SmoothTicker() {}

    /**
     * 返回平滑的时间戳，防止由于网络抖动导致时间戳不平滑
     */
    uint64_t elapsedTime() {
        auto now_time = _ticker.elapsedTime();
        if (_first_time == 0) {
            if (now_time < _last_time) {
                auto last_time = _last_time - _time_inc;
                double elapse_time = (now_time - last_time);
                _time_inc += (elapse_time / ++_pkt_count) / 3;
                auto ret_time = last_time + _time_inc;
                _last_time = (uint64_t) ret_time;
                return (uint64_t) ret_time;
            }
            _first_time = now_time;
            _last_time = now_time;
            _pkt_count = 0;
            _time_inc = 0;
            return now_time;
        }

        auto elapse_time = (now_time - _first_time);
        _time_inc += elapse_time / ++_pkt_count;
        auto ret_time = _first_time + _time_inc;
        if (elapse_time > _reset_ms) {
            _first_time = 0;
        }
        _last_time = (uint64_t) ret_time;
        return (uint64_t) ret_time;
    }

    /**
     * 时间戳重置为0开始
     */
    void resetTime() {
        _first_time = 0;
        _pkt_count = 0;
        _ticker.resetTime();
    }

private:
    double _time_inc = 0;
    uint64_t _first_time = 0;
    uint64_t _last_time = 0;
    uint64_t _pkt_count = 0;
    uint64_t _reset_ms;
    Ticker _ticker;
};

#if !defined(NDEBUG)
#define TimeTicker() Ticker __ticker(5,WarnL,true)
#define TimeTicker1(tm) Ticker __ticker1(tm,WarnL,true)
#define TimeTicker2(tm, log) Ticker __ticker2(tm,log,true)
#else
#define TimeTicker()
#define TimeTicker1(tm)
#define TimeTicker2(tm,log)
#endif

} /* namespace turbo */
#endif /* TURBO_NETWORK_UTIL_TIME_TICKER_H_ */