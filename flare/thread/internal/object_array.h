
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_THREAD_INTERNAL_OBJECT_ARRAY_H_
#define FLARE_THREAD_INTERNAL_OBJECT_ARRAY_H_


#include <cstdint>

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>
#include <functional>

#include "flare/base/profile.h"
#include "flare/base/annotation.h"
#include "flare/thread/internal/barrier.h"
#include "flare/log/logging.h"
#include "flare/memory/resident.h"

namespace flare {
    namespace thread_internal {

        template<class T>
        struct object_entry {
            std::aligned_storage_t<sizeof(T), alignof(T)> storage;
            static_assert(sizeof(T) == sizeof(storage));
        };

        // Performance note: ANY MODIFICATION TO THIS STRUCTURE MUST KEEP IT TRIVIAL.
        // We'll be accessing it as thread-local variables often, if it's not trivial,
        // we'd pay a dynamic initialization penalty on each access. (For now it's
        // statically initialized as it's trivial and initialized to all zero.).
        template<class T>
        struct object_array_cache {
            std::size_t limit{};
            T *objects{};
        };

        template<class T>
        struct object_array;

        // Stores a series of objects, either initialized or not.
        //
        // Here objects are stored in "structure of arrays" instead of "array of
        // structures" as the fields are accessed unequally frequent.
        template<class T>
        class lazy_init_object_array {
            struct entry_deleter {
                void operator()(object_entry<T> *ptr) noexcept { operator delete(ptr); }
            };

            using aligned_array = std::unique_ptr<object_entry<T>[], entry_deleter>;

        public:
            lazy_init_object_array() = default;

            ~lazy_init_object_array() {
                for (size_t i = 0; i != initialized_.size(); ++i) {
                    if (initialized_[i]) {
                        reinterpret_cast<T *>(&objects_[i])->~T();
                    }
                }
            }

            // Move-only.
            lazy_init_object_array(lazy_init_object_array &&) = default;

            lazy_init_object_array &operator=(lazy_init_object_array &) = default;

            // Expands internal storage.
            //
            // Throwing inside leads to crash.
            void uninitialized_expand(std::size_t new_size) noexcept {
                FLARE_CHECK_GT(new_size, size());
                auto new_entries = allocate_at_least_n_entries(new_size);

                // Move initialized objects.
                for (size_t i = 0; i != size(); ++i) {
                    if (initialized_[i]) {
                        auto from = reinterpret_cast<T *>(&objects_[i]);
                        auto to = &new_entries[i];

                        new(to) T(std::move(*from));
                        from->~T();
                    }  // It was uninitialized otherwise, skip it.
                }

                objects_ = std::move(new_entries);
                initialized_.resize(new_size, false);
            }

            template<class F>
            void initialize_at(std::size_t index, F &&f) {
                FLARE_CHECK_LT(index, initialized_.size());
                FLARE_CHECK(!initialized_[index]);
                initialized_[index] = true;
                std::forward<F>(f)(&objects_[index]);
            }

            void destroy_at(std::size_t index) {
                FLARE_CHECK_LT(index, initialized_.size());
                FLARE_CHECK(initialized_[index]);
                initialized_[index] = false;
                reinterpret_cast<T *>(&objects_[index])->~T();
            }

            bool is_initialized_at(std::size_t index) const noexcept {
                FLARE_CHECK_LT(index, initialized_.size());
                return initialized_[index];
            }

            T *get_at(std::size_t index) noexcept {
                FLARE_CHECK_LT(index, initialized_.size());
                FLARE_CHECK(initialized_[index]);
                return reinterpret_cast<T *>(&objects_[index]);
            }

            T *get_objects_maybe_uninitialized() noexcept {
                static_assert(sizeof(T) == sizeof(object_entry<T>));
                return reinterpret_cast<T *>(objects_.get());
            }

            std::size_t size() const noexcept { return initialized_.size(); }

        private:

            aligned_array allocate_at_least_n_entries(std::size_t desired) {
                // Some memory allocator (e.g., some versions of gperftools) can allocate
                // adjacent memory region (even within a cache-line) to different threads.
                //
                // So as to avoid false-sharing, we align the memory size ourselves.
                auto aligned_size = sizeof(object_entry<T>) * desired;
                aligned_size = (aligned_size + hardware_destructive_interference_size - 1) /
                               hardware_destructive_interference_size * hardware_destructive_interference_size;
                // FIXME: Do we have to call `object_entry<T>`'s ctor? It's POD anyway.
                return aligned_array(reinterpret_cast<object_entry<T> *>(operator new(aligned_size)), {});
            }

        private:
            aligned_array objects_;
            std::vector<bool> initialized_;  // `std::vector<bool>` is hard..
        };

        // This allows us to traverse through all thread's local object array.
        template<class T>
        class object_array_registry {
        public:
            static object_array_registry *instance() {
                static resident_singleton<object_array_registry> instance;
                return instance.get();
            }

            void registery(object_array<T> *array) {
                std::scoped_lock _(lock_);
                FLARE_CHECK(std::find(arrays_.begin(), arrays_.end(), array) ==
                       arrays_.end());
                arrays_.push_back(array);
            }

            void deregister(object_array<T> *array) {
                std::scoped_lock _(lock_);
                auto iter = std::find(arrays_.begin(), arrays_.end(), array);
                FLARE_CHECK(iter != arrays_.end());
                arrays_.erase(iter);
            }

            template<class F>
            void for_each_locked(std::size_t index, F &&f) {
                std::scoped_lock _(lock_);
                for (auto &&e : arrays_) {
                    std::scoped_lock __(e->lock);
                    if (index < e->objects.size()) {
                        f(e);
                    }
                }
            }

            // This method not only traverse the object array, but also broadcasts
            // modification done by `f`, via a heavy barrier.
            template<class F>
            void broadcasting_for_each_locked(std::size_t index, F &&f) {
                for_each_locked(index, std::forward<F>(f));

                // Pairs with the light barrier in `get_local_object_array_at`.
                asymmetric_barrier_heavy();
            }

        private:
            std::mutex lock_;
            std::vector<object_array<T> *> arrays_;
        };

        // Stores dynamically allocated thread-local variables.
        template<class T>
        struct object_array {
            std::mutex lock;  // Synchronizes between traversal and update.
            lazy_init_object_array<T> objects;

            object_array() { object_array_registry<T>::instance()->registery(this); }

            ~object_array() { object_array_registry<T>::instance()->deregister(this); }
        };

        // This class helps us to keep track of current (newest) layout of
        // `object_array<T>`.
        template<class T>
        class object_array_layout {
            using InitializerPtr = std::function<void(void *)> *;

        public:
            // Access newest layout with internal lock held.
            template<class F>
            void with_wewest_layout_locked(F &&f) const noexcept {
                std::scoped_lock _(lock_);
                std::forward<F>(f)(initializers_);
            }

            // Returns: [Version ID, Index in resulting layout].
            template<class F>
            std::size_t create_entry(std::function<void(void *)> *initializer, F &&cb) {
                std::scoped_lock _(lock_);

                // Let's see if we can reuse a slot.
                if (unused_.empty()) {
                    FLARE_CHECK_EQ(max_index_, initializers_.size());
                    initializers_.emplace_back(nullptr);
                    unused_.push_back(max_index_++);
                }

                auto v = unused_.back();
                unused_.pop_back();
                FLARE_CHECK(!initializers_[v]);
                initializers_[v] = initializer;

                // Called after the slot is initialized.
                std::forward<F>(cb)(v);
                return v;
            }

            // Free an index. It's the caller's responsibility to free the slot
            // beforehand.
            template<class F>
            void free_entry(std::size_t index, F &&cb) noexcept {
                std::scoped_lock _(lock_);

                // Called before the slot is freed.
                std::forward<F>(cb)();

                FLARE_CHECK(initializers_[index]);
                initializers_[index] = nullptr;
                unused_.push_back(index);
            }

            static object_array_layout *instance() {
                static resident_singleton<object_array_layout> layout;
                return layout.get();
            }

        private:
            mutable std::mutex lock_;
            std::vector<InitializerPtr> initializers_;
            std::vector<std::size_t> unused_;
            std::size_t max_index_{};
        };

        template<class T>
        inline T *add_to_ptr(T *ptr, std::ptrdiff_t offset) {
            FLARE_CHECK_EQ(offset % sizeof(T), 0ul);
            return reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(ptr) + offset);
        }

        template<class T>
        object_array<T> *get_newest_local_object_array() {
            FLARE_INTERNAL_TLS_MODEL
            thread_local object_array<T> array;

            // Expand internal storage and initialize new slots.
            object_array_layout<T>::instance()->with_wewest_layout_locked([&](auto &&layout) {
                std::scoped_lock _(array.lock);

                // We shouldn't be called otherwise.
                FLARE_CHECK_LT(array.objects.size(), layout.size());

                // Expand the object array.
                auto was = array.objects.size();
                array.objects.uninitialized_expand(layout.size());

                // Initialize the new slots.
                for (size_t i = was; i != array.objects.size(); ++i) {
                    if (layout[i]) {
                        array.objects.initialize_at(i, *layout[i]);
                    }  // Otherwise the slot is freed before we have a chance to initialize
                    // it.
                }
            });
            return &array;
        }

        template<class T>
        [[gnu::noinline]] T *reload_local_object_array_cache(std::ptrdiff_t offset,
                                                             object_array_cache<T> *cache) {
            FLARE_CHECK_EQ(offset % sizeof(T), 0UL);
            auto &&obj_array = get_newest_local_object_array<T>();
            cache->objects = obj_array->objects.get_objects_maybe_uninitialized();
            cache->limit = obj_array->objects.size() * sizeof(T);
            return add_to_ptr(cache->objects, offset);
        }

// Things are tricky here.
//
// Unless we're using "fast" TLS model, this method must not be `inline`-d. This
// is especially the case when we're dynamically linked.
//
// The reason is that we can ge generated in different shared objects, and
// despite of being prohibited by C++ Standard (?), there can be multiple copies
// of TLS (variable `cache`), one in each shared object.
//
// In unfortunate cases, if different isntantiations (i.e., different shared
// object) of us are called, except for the first call, all subsequent calls
// would fail the `FLARE_CHECK` in `reload_local_object_array_cache`, due to inconsistency
// in our `cache` and our callee's `array`.
//
// To workaround this issue, if we're using the slow TLS mode, even if this
// method is in hot path, it's explicitly marked as `noinline`.
#if !defined(FLARE_USE_SLOW_TLS_MODEL)

        template<class T>
        inline T *get_local_object_array_at(std::ptrdiff_t offset) {
#else
            template <class T>
    [[gnu::noinline]] T* get_local_object_array_at(std::ptrdiff_t offset) {
#endif
            FLARE_INTERNAL_TLS_MODEL
            thread_local object_array_cache<T> cache;
            FLARE_CHECK_EQ(static_cast<size_t>(offset) % sizeof(T), 0ul);
            FLARE_CHECK_GE(static_cast<size_t>(offset), 0ul);
            if (FLARE_LIKELY(static_cast<size_t>(offset) < cache.limit)) {
                // Pairs with the heavy barrier in `BroadcastingForEachLocked` above.
                asymmetric_barrier_light();
                return add_to_ptr(cache.objects, offset);
            }
            return reload_local_object_array_cache<T>(offset, &cache);
        }
    }  // namespace thread_internal

}  // namespace flare

#endif  // FLARE_THREAD_INTERNAL_OBJECT_ARRAY_H_
