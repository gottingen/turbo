
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_PARALLEL_RING_QUEUE_H_
#define FLARE_CONTAINER_PARALLEL_RING_QUEUE_H_

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <shared_mutex>
#include <tuple>

namespace flare {

    template<typename Item, typename Mutex = std::shared_mutex>
    class parallel_ring_queue {
        static constexpr uint32_t kDefaultCapacity = 1 << 10;
        static constexpr uint32_t kMaxCapacity = uint32_t(1) << 31;
        static constexpr uint32_t kMinCapacity = 2;

    public:
        parallel_ring_queue() { ring_queue_ = new Item[capacity_]; }

        explicit parallel_ring_queue(uint32_t capacity) {
            capacity_ = fix_capacity(capacity);
            ring_queue_ = new Item[capacity_];
        }

        parallel_ring_queue(const parallel_ring_queue<Item> &queue) = delete;

        parallel_ring_queue<Item> &operator=(const parallel_ring_queue<Item> &queue) = delete;

        ~parallel_ring_queue() { delete[] ring_queue_; }


        inline uint32_t capacity() const { return capacity_; }

        inline void reserve(uint32_t capacity) {
            capacity_ = fix_capacity(capacity);
            delete[] ring_queue_;
            ring_queue_ = new Item[capacity_];
        }

        inline bool push_back(const Item &item);

        inline std::tuple<bool, Item> pop_front();

        inline std::tuple<bool, Item> front();

        inline uint32_t size();

        inline bool is_empty() {
            std::shared_lock<Mutex> lk(mutex_);
            return front_ == rear_;
        }

        inline bool is_full() {
            std::shared_lock<Mutex> lk(mutex_);
            return to_circle_idx(rear_ + 1) == front_;
        }

    private:

        inline uint32_t to_circle_idx(uint32_t idx) { return capacity_ == 0 ? 0 : idx & (capacity_ - 1); }

        inline bool sync_is_empty() { return front_ == rear_; }

        inline bool sync_is_full() { return to_circle_idx(rear_ + 1) == front_; }

        uint32_t fix_capacity(uint32_t capacity) const;

        uint32_t power_of_two_for_size(uint32_t size) const;

        uint32_t highest_one_bit(uint32_t i) const;

    private:
        Item *ring_queue_ = nullptr;
        uint32_t rear_ = 0;
        uint32_t front_ = 0;
        uint32_t capacity_ = kDefaultCapacity;
        mutable Mutex mutex_;
    };

    template<typename Item, typename Mutex>
    bool parallel_ring_queue<Item,Mutex>::push_back(const Item &item) {
        std::unique_lock<Mutex> lk(mutex_);
        if (sync_is_full()) {
            return false;
        }
        ring_queue_[to_circle_idx(rear_)] = item;
        rear_ = to_circle_idx(rear_ + 1);
        return true;
    }

    template<typename Item, typename Mutex>
    std::tuple<bool, Item> parallel_ring_queue<Item,Mutex>::pop_front() {
        std::unique_lock<Mutex> lk(mutex_);
        if (sync_is_empty()) {
            return std::make_tuple(false, Item());
        }
        auto item = ring_queue_[to_circle_idx(front_)];
        front_ = to_circle_idx(front_ + 1);

        return std::make_tuple(true, item);
    }

    template<typename Item, typename Mutex>
    std::tuple<bool, Item> parallel_ring_queue<Item,Mutex>::front() {
        std::shared_lock<Mutex> lk(mutex_);
        if (sync_is_empty()) {
            return std::make_tuple(false, Item());
        }
        return std::make_tuple(true, ring_queue_[to_circle_idx(front_)]);
    }

    template<typename Item, typename Mutex>
    uint32_t parallel_ring_queue<Item,Mutex>::size() {
        if (capacity_ == 0) {
            return 0;
        }
        std::shared_lock<Mutex> lk(mutex_);
        return (rear_ >= front_) ? (rear_ - front_) : (rear_ + capacity_ - front_);
    }

    template<typename Item, typename Mutex>
    uint32_t parallel_ring_queue<Item,Mutex>::fix_capacity(uint32_t capacity) const {
        if (capacity == 0) {
            return kDefaultCapacity;
        }
        if (capacity > kMaxCapacity) {
            return kMaxCapacity;
        }
        if (capacity == 1) {
            return kMinCapacity;
        }
        return power_of_two_for_size(capacity);
    }

    template<typename Item, typename Mutex>
    uint32_t parallel_ring_queue<Item,Mutex>::power_of_two_for_size(uint32_t size) const {
        if (size <= 0) {
            return 0;
        } else {
            return size == 1 ? 1 : highest_one_bit(size - 1) << 1;
        }
    }

    template<typename Item, typename Mutex>
    uint32_t parallel_ring_queue<Item,Mutex>::highest_one_bit(uint32_t i) const {
        i |= (i >> 1);
        i |= (i >> 2);
        i |= (i >> 4);
        i |= (i >> 8);
        i |= (i >> 16);
        return i - (i >> 1);
    }
}  // namespace flare

#endif  // FLARE_CONTAINER_PARALLEL_RING_QUEUE_H_
