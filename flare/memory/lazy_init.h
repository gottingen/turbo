
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_MEMORY_LAZY_INIT_H_
#define FLARE_MEMORY_LAZY_INIT_H_


#include <optional>
#include <utility>

#include "flare/log/logging.h"

namespace flare {

    template<class T>
    class lazy_init {
    public:
        template<class... Args>
        void init(Args &&... args) {
            value_.emplace(std::forward<Args>(args)...);
        }

        void destroy() { value_ = std::nullopt; }

        T *operator->() {
            FLARE_DCHECK(value_);
            return &*value_;
        }

        const T *operator->() const {
            FLARE_DCHECK(value_);
            return &*value_;
        }

        T &operator*() {
            FLARE_DCHECK(value_);
            return *value_;
        }

        const T &operator*() const {
            FLARE_DCHECK(value_);
            return *value_;
        }

        explicit operator bool() const { return !!value_; }

    private:
        std::optional<T> value_;
    };

}  // namespace flare

#endif  // FLARE_MEMORY_LAZY_INIT_H_
