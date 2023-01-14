
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_REUSE_ID_H_
#define TURBO_BASE_REUSE_ID_H_

#include <cstddef>
#include <mutex>
#include <vector>
#include <algorithm>
#include <type_traits>
#include "turbo/memory/resident.h"

namespace turbo {

    template<typename T, typename Tag, T Max = std::numeric_limits<T>::max()/2>
    class reuse_id {
    public:
        static_assert(std::is_integral<T>::value, "T must be integral");
        static reuse_id *instance() {
            static resident_singleton<reuse_id> ia;
            return ia.get();
        }

        T next() {
            std::scoped_lock lk(_mutex);
            if (!_recycled.empty()) {
                auto rc = _recycled.back();
                _recycled.pop_back();
                return rc;
            }
            if(_current >= _max) {
                return _max;
            }
            return _current++;
        }

        bool free(T index) {
            std::scoped_lock lk(_mutex);
            if(index >= _current) {
                return false;
            }
            if (index + 1 == _current) {
                _current--;
                return true;
            }
            _recycled.push_back(index);
            return true;
        }

    private:
        reuse_id() = default;

        ~reuse_id() = default;

        void do_shrink() {
            std::sort(_recycled.begin(), _recycled.end());
            while (_recycled.back() + 1 == _current) {
                _recycled.pop_back();
                _current--;
            }
        }
        friend class resident_singleton<reuse_id>;
    private:
        std::mutex _mutex;
        T _current{0};
        T _max{Max};
        std::vector<T> _recycled;

    };
}  // namespace turbo

#endif  // TURBO_BASE_REUSE_ID_H_
