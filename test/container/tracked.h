
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TRACKED_H_
#define TRACKED_H_

#include <stddef.h>
#include <memory>
#include <utility>

namespace flare {
    namespace priv {

        // A class that tracks its copies and moves so that it can be queried in tests.
        template<class T>
        class Tracked {
        public:
            Tracked() {}

            // NOLINTNEXTLINE(runtime/explicit)
            Tracked(const T &val) : val_(val) {}

            Tracked(const Tracked &that)
                    : val_(that.val_),
                      num_moves_(that.num_moves_),
                      num_copies_(that.num_copies_) {
                ++(*num_copies_);
            }

            Tracked(Tracked &&that)
                    : val_(std::move(that.val_)),
                      num_moves_(std::move(that.num_moves_)),
                      num_copies_(std::move(that.num_copies_)) {
                ++(*num_moves_);
            }

            Tracked &operator=(const Tracked &that) {
                val_ = that.val_;
                num_moves_ = that.num_moves_;
                num_copies_ = that.num_copies_;
                ++(*num_copies_);
            }

            Tracked &operator=(Tracked &&that) {
                val_ = std::move(that.val_);
                num_moves_ = std::move(that.num_moves_);
                num_copies_ = std::move(that.num_copies_);
                ++(*num_moves_);
            }

            const T &val() const { return val_; }

            friend bool operator==(const Tracked &a, const Tracked &b) {
                return a.val_ == b.val_;
            }

            friend bool operator!=(const Tracked &a, const Tracked &b) {
                return !(a == b);
            }

            size_t num_copies() { return *num_copies_; }

            size_t num_moves() { return *num_moves_; }

        private:
            T val_;
            std::shared_ptr<size_t> num_moves_ = std::make_shared<size_t>(0);
            std::shared_ptr<size_t> num_copies_ = std::make_shared<size_t>(0);
        };

    }  // namespace priv
}  // namespace flare

#endif  // TRACKED_H_
