
#ifndef TURBO_MEMORY_OBJECT_POOL_H_
#define TURBO_MEMORY_OBJECT_POOL_H_

#include <cstddef>                       // size_t
#include "turbo/memory/ref_ptr.h"
// ObjectPool is a derivative class of ResourcePool to allocate and
// reuse fixed-size objects without identifiers.

namespace turbo {

    // Specialize following classes to override default parameters for type T.
    //   namespace turbo {
    //     template <> struct ObjectPoolBlockMaxSize<Foo> {
    //       static const size_t value = 1024;
    //     };
    //   }

    // Memory is allocated in blocks, memory size of a block will not exceed:
    //   min(ObjectPoolBlockMaxSize<T>::value,
    //       ObjectPoolBlockMaxItem<T>::value * sizeof(T))
    template<typename T>
    struct ObjectPoolBlockMaxSize {
        static const size_t value = 64 * 1024; // bytes
    };
    template<typename T>
    struct ObjectPoolBlockMaxItem {
        static const size_t value = 256;
    };

    // Free objects of each thread are grouped into a chunk before they are merged
    // to the global list. Memory size of objects in one free chunk will not exceed:
    //   min(ObjectPoolFreeChunkMaxItem<T>::value() * sizeof(T),
    //       ObjectPoolBlockMaxSize<T>::value,
    //       ObjectPoolBlockMaxItem<T>::value * sizeof(T))
    template<typename T>
    struct ObjectPoolFreeChunkMaxItem {
        static size_t value() { return 256; }
    };

    // ObjectPool calls this function on newly constructed objects. If this
    // function returns false, the object is destructed immediately and
    // get_object() shall return nullptr. This is useful when the constructor
    // failed internally(namely ENOMEM).
    template<typename T>
    struct ObjectPoolValidator {
        static bool validate(const T *) { return true; }
    };

}  // namespace turbo

#include "turbo/memory/object_pool_inl.h"

namespace turbo {

    // Get an object typed |T|. The object should be cleared before usage.
    // NOTE: T must be default-constructible.
    template<typename T>
    inline T *get_object() {
        return ObjectPool<T>::singleton()->get_object();
    }

    // Get an object whose constructor is T(arg1)
    template<typename T, typename A1>
    inline T *get_object(const A1 &arg1) {
        return ObjectPool<T>::singleton()->get_object(arg1);
    }

    // Get an object whose constructor is T(arg1, arg2)
    template<typename T, typename A1, typename A2>
    inline T *get_object(const A1 &arg1, const A2 &arg2) {
        return ObjectPool<T>::singleton()->get_object(arg1, arg2);
    }

    // Return the object |ptr| back. The object is NOT destructed and will be
    // returned by later get_object<T>. Similar with free/delete, validity of
    // the object is not checked, user shall not return a not-yet-allocated or
    // already-returned object otherwise behavior is undefined.
    // Returns 0 when successful, -1 otherwise.
    template<typename T>
    inline int return_object(T *ptr) {
        return ObjectPool<T>::singleton()->return_object(ptr);
    }

    // Reclaim all allocated objects typed T if caller is the last thread called
    // this function, otherwise do nothing. You rarely need to call this function
    // manually because it's called automatically when each thread quits.
    template<typename T>
    inline void clear_objects() {
        ObjectPool<T>::singleton()->clear_objects();
    }

    // Get description of objects typed T.
    // This function is possibly slow because it iterates internal structures.
    // Don't use it frequently like a "getter" function.
    template<typename T>
    ObjectPoolInfo describe_objects() {
        return ObjectPool<T>::singleton()->describe_objects();
    }

    template<class T>
    struct object_pool_deleter {
        void operator()(T *p) const noexcept;
    };

    // For classes that's both ref-counted and pooled, inheriting from this class
    // can be handy (so that you don't need to write your own `RefTraits`.).
    //
    // Note that reference count is always initialized to one, either after
    // construction or returned by object pool. So use `adopt_ptr` should you
    // want to construct a `ref_ptr` from a raw pointer.
    template<class T>
    using pool_ref_counted = turbo::ref_counted<T, object_pool_deleter<T>>;

    // Interface of `turbo::object_pool::get` does not align very well with
    // `ref_ptr`. It returns a `pooled_ptr`, which itself is a RAII wrapper. To
    // simplify the use of pooled `RefCounted`, we provide this method.
    template<class T,
            class = std::enable_if_t<std::is_base_of_v<pool_ref_counted<T>, T>>>
    ref_ptr<T> get_ref_counted() {
#ifndef NDEBUG
        auto ptr = ref_ptr(adopt_ptr_v, get_object<T>());
        TURBO_DCHECK_EQ(1, ptr->unsafe_ref_count());
        return ptr;
#else
        return ref_ptr(adopt_ptr_v, get_object<T>());
#endif
    }

    template<class T>
    void object_pool_deleter<T>::operator()(T *p) const noexcept {
        TURBO_DCHECK_EQ(p->ref_count_.load(std::memory_order_relaxed), 0);

        // Keep ref-count as 1 for reuse.
        //
        // It shouldn't be necessary to enforce memory ordering here as any ordering
        // requirement should already been satisfied by `ref_counted<T>::deref()`.
        p->ref_count_.store(1, std::memory_order_relaxed);
        return_object<T>(p);
    }

}  // namespace turbo

#endif  // TURBO_MEMORY_OBJECT_POOL_H_
