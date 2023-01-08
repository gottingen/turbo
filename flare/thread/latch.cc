
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/thread/latch.h"

namespace flare {

    latch::latch(uint32_t count) : _data(std::make_shared<inner_data>()) {
        _data->count = count;
    }

    void latch::count_down(uint32_t update) {
        FLARE_CHECK(_data->count > 0) << "flare::latch::count_down() called too many times";
        _data->count -= update;
        if (_data->count == 0) {
            std::unique_lock lk(_data->mutex);
            _data->cond.notify_all();
        }
    }

    void latch::count_up(uint32_t update) {
        _data->count += update;
    }

    bool latch::try_wait() const noexcept {
        std::unique_lock lk(_data->mutex);
        FLARE_CHECK_GE(_data->count, 0u);
        return !_data->count;
    }

    void latch::wait() const {
        std::unique_lock lk(_data->mutex);
        FLARE_CHECK_GE(_data->count, 0u);
        return _data->cond.wait(lk, [this] { return _data->count == 0; });
    }

    void latch::arrive_and_wait(std::ptrdiff_t update) {
        count_down(update);
        wait();
    }

}  // namespace flare