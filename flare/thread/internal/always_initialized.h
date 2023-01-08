
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_THREAD_INTERNAL_ALWAYS_INITIALIZED_H_
#define FLARE_THREAD_INTERNAL_ALWAYS_INITIALIZED_H_


#include <utility>
#include <functional>
#include "flare/thread/internal/object_array.h"

namespace flare::thread_internal {

    // Same as `thread_local_store<T>` except that objects are initialized eagerly (also
    // nondeterministically.). Note that `T`'s constructor may not touch other TLS
    // variables, otherwise the behavior is undefined.
    //
    // Performs slightly better. For internal use only.
    //
    // Instances of `T` in different threads are guaranteed to reside in different
    // cache line. However, if `T` itself allocates memory, there's no guarantee on
    // how memory referred by `T` in different threads are allocated.
    //
    // IT'S EXPLICITLY NOT SUPPORTED TO CONSTRUCT / DESTROY OTHER THREAD-LOCAL
    // VARIABLES IN CONSTRUCTOR / DESTRUCTOR OF THIS CLASS.
    template<class T>
    class thread_local_always_initialized {
    public:
        thread_local_always_initialized();

        ~thread_local_always_initialized();

        // Initialize object with a customized initializer.
        template<class F>
        explicit thread_local_always_initialized(F &&initializer);

        // Noncopyable / nonmovable.
        thread_local_always_initialized(const thread_local_always_initialized &) = delete;

        thread_local_always_initialized &operator=(const thread_local_always_initialized &) =
        delete;

        // Accessors.
        T *get() const;

        T *operator->() const { return get(); }

        T &operator*() const { return *get(); }

        // Traverse through all instances among threads.
        //
        // CAUTION: Called with internal lock held. You may not touch TLS in `f`.
        // (Maybe we should name it as `ForEachUnsafe` or `ForEachLocked`?)
        template<class F>
        void for_each(F &&f) const;

    private:
        // Placed as the first member to keep accessing it quick.
        //
        // Always a multiple of `kEntrySize`.
        std::ptrdiff_t offset_;
        std::function<void(void *)> initializer_;
    };

    template<class T>
    thread_local_always_initialized<T>::thread_local_always_initialized()
            : thread_local_always_initialized([](void *ptr) { new(ptr) T(); }) {}

    template<class T>
    template<class F>
    thread_local_always_initialized<T>::thread_local_always_initialized(F &&initializer)
            : initializer_(std::forward<F>(initializer)) {
        // Initialize the slot in every thread (that has allocated the slot in its
        // thread-local object array.).
        auto slot_initializer = [&](auto index) {
            // Called with layout lock held. Nobody else should be resizing its own
            // object array or mutating the (type-specific) global layout.

            // Initialize all slots (if the slot itself has already been allocated in
            // corresponding thread's object array) immediately so that we don't need to
            // check for initialization on `Get()`.
            object_array_registry<T>::instance()->broadcasting_for_each_locked(
                    index, [&](auto *p) { p->objects.initialize_at(index, initializer_); });
        };

        // Allocate a slot.
        offset_ = object_array_layout<T>::instance()->create_entry(&initializer_, slot_initializer) * sizeof(T);
    }

    template<class T>
    thread_local_always_initialized<T>::~thread_local_always_initialized() {
        FLARE_CHECK_EQ(static_cast<size_t>(offset_) % sizeof(T), 0ul);
        auto index = offset_ / sizeof(T);

        // The slot is freed after we have destroyed all instances.
        object_array_layout<T>::instance()->free_entry(index, [&] {
            // Called with layout lock held.

            // Destory all instances. (We actually reconstructed a new one at the place
            // we were at.)
            object_array_registry<T>::instance()->broadcasting_for_each_locked(
                    index, [&](auto *p) { p->objects.destroy_at(index); });
        });
    }

    template<class T>
    inline T *thread_local_always_initialized<T>::get() const {
        return get_local_object_array_at<T>(offset_);
    }

    template<class T>
    template<class F>
    void thread_local_always_initialized<T>::for_each(F &&f) const {
        FLARE_DCHECK_EQ(static_cast<size_t>(offset_) % sizeof(T), 0ul);
        auto index = offset_ / sizeof(T);

        object_array_registry<T>::instance()->for_each_locked(
                index, [&](auto *p) { f(p->objects.get_at(index)); });
    }
}  // namespace flare::thread_internal

#endif  // FLARE_THREAD_INTERNAL_ALWAYS_INITIALIZED_H_
