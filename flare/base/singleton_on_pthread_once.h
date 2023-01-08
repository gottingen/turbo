

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_SINGLETON_ON_PTHREAD_ONCE_H_
#define FLARE_BASE_SINGLETON_ON_PTHREAD_ONCE_H_

#include <pthread.h>
#include "flare/base/static_atomic.h"

namespace flare {

    template<typename T>
    class GetLeakySingleton {
    public:
        static std::atomic<intptr_t> g_leaky_singleton_untyped;
        static pthread_once_t g_create_leaky_singleton_once;

        static void create_leaky_singleton();
    };

    template<typename T>
    std::atomic<intptr_t> GetLeakySingleton<T>::g_leaky_singleton_untyped = 0;

    template<typename T>
    pthread_once_t GetLeakySingleton<T>::g_create_leaky_singleton_once = PTHREAD_ONCE_INIT;

    template<typename T>
    void GetLeakySingleton<T>::create_leaky_singleton() {
        T *obj = new T;
        g_leaky_singleton_untyped.store(reinterpret_cast<intptr_t>(obj));
    }

    // To get a never-deleted singleton of a type T, just call get_leaky_singleton<T>().
    // Most daemon threads or objects that need to be always-on can be created by
    // this function.
    // This function can be called safely before main() w/o initialization issues of
    // global variables.
    template<typename T>
    inline T *get_leaky_singleton() {
        const intptr_t value = GetLeakySingleton<T>::g_leaky_singleton_untyped.load(std::memory_order_acquire);
        if (value) {
            return reinterpret_cast<T *>(value);
        }
        pthread_once(&GetLeakySingleton<T>::g_create_leaky_singleton_once,
                     GetLeakySingleton<T>::create_leaky_singleton);
        return reinterpret_cast<T *>(
                GetLeakySingleton<T>::g_leaky_singleton_untyped.load(std::memory_order_acquire));
    }

    // True(non-nullptr) if the singleton is created.
    // The returned object (if not nullptr) can be used directly.
    template<typename T>
    inline T *has_leaky_singleton() {
        return reinterpret_cast<T *>(GetLeakySingleton<T>::g_leaky_singleton_untyped.load(std::memory_order_acquire));
    }

} // namespace flare

#endif // FLARE_BASE_SINGLETON_ON_PTHREAD_ONCE_H_
