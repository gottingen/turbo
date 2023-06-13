// Copyright 2023 The titan-search Authors.
//
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

#ifndef TURBO_CONCURRENT_DOUBLY_BUFFERED_DATA_H_
#define TURBO_CONCURRENT_DOUBLY_BUFFERED_DATA_H_

#include <pthread.h>

#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>
#include <atomic>
#include "turbo/platform/port.h"
#include "turbo/tlog/logging.h"

namespace turbo {

    // This data structure makes Read() almost lock-free by making Modify()
    // *much* slower. It's very suitable for implementing LoadBalancers which
    // have a lot of concurrent read-only ops from many threads and occasional
    // modifications of data. As a side effect, this data structure can store
    // a thread-local data for user.
    //
    // Read(): begin with a thread-local mutex locked then read the foreground
    // instance which will not be changed before the mutex is unlocked. Since the
    // mutex is only locked by Modify() with an empty critical section, the
    // function is almost lock-free.
    //
    // Modify(): Modify background instance which is not used by any Read(), flip
    // foreground and background, lock thread-local mutexes one by one to make
    // sure all existing Read() finish and later Read() see new foreground,
    // then modify background(foreground before flip) again.

    template<typename T>
    void delete_object(void *arg) {
        delete static_cast<T *>(arg);
    }

    class Void {
    };

    template<typename T, typename MUTEX = std::mutex, typename TLS = Void>
    class DoublyBufferedData {
        class Wrapper;

    public:
        class ScopedPtr {
            friend class DoublyBufferedData;

        public:
            ScopedPtr() : _data(nullptr), _w(nullptr) {}

            ~ScopedPtr() {
                if (_w) {
                    _w->EndRead();
                }
            }

            const T *get() const { return _data; }

            const T &operator*() const { return *_data; }

            const T *operator->() const { return _data; }

            TLS &tls() { return _w->user_tls(); }

        private:
            ScopedPtr(const ScopedPtr &);

            ScopedPtr &operator=(const ScopedPtr &);

            const T *_data;
            Wrapper *_w;
        };

        DoublyBufferedData();

        ~DoublyBufferedData();

        // Put foreground instance into ptr. The instance will not be changed until
        // ptr is destructed.
        // This function is not blocked by Read() and Modify() in other threads.
        // Returns 0 on success, -1 otherwise.
        int Read(ScopedPtr *ptr);

        // Modify background and foreground instances. fn(T&, ...) will be called
        // twice. Modify() from different threads are exclusive from each other.
        // NOTE: Call same series of fn to different equivalent instances should
        // result in equivalent instances, otherwise foreground and background
        // instance will be inconsistent.
        template<typename Fn>
        size_t Modify(Fn &fn);

        template<typename Fn, typename Arg1>
        size_t Modify(Fn &fn, const Arg1 &);

        template<typename Fn, typename Arg1, typename Arg2>
        size_t Modify(Fn &fn, const Arg1 &, const Arg2 &);

        // fn(T& background, const T& foreground, ...) will be called to background
        // and foreground instances respectively.
        template<typename Fn>
        size_t ModifyWithForeground(Fn &fn);

        template<typename Fn, typename Arg1>
        size_t ModifyWithForeground(Fn &fn, const Arg1 &);

        template<typename Fn, typename Arg1, typename Arg2>
        size_t ModifyWithForeground(Fn &fn, const Arg1 &, const Arg2 &);

    private:
        template<typename Fn>
        struct WithFG0 {
            WithFG0(Fn &fn, T *data) : _fn(fn), _data(data) {}

            size_t operator()(T &bg) { return _fn(bg, (const T &) _data[&bg == _data]); }

        private:
            Fn &_fn;
            T *_data;
        };

        template<typename Fn, typename Arg1>
        struct WithFG1 {
            WithFG1(Fn &fn, T *data, const Arg1 &arg1) : _fn(fn), _data(data), _arg1(arg1) {}

            size_t operator()(T &bg) { return _fn(bg, (const T &) _data[&bg == _data], _arg1); }

        private:
            Fn &_fn;
            T *_data;
            const Arg1 &_arg1;
        };

        template<typename Fn, typename Arg1, typename Arg2>
        struct WithFG2 {
            WithFG2(Fn &fn, T *data, const Arg1 &arg1, const Arg2 &arg2)
                    : _fn(fn), _data(data), _arg1(arg1), _arg2(arg2) {}

            size_t operator()(T &bg) { return _fn(bg, (const T &) _data[&bg == _data], _arg1, _arg2); }

        private:
            Fn &_fn;
            T *_data;
            const Arg1 &_arg1;
            const Arg2 &_arg2;
        };

        template<typename Fn, typename Arg1>
        struct Closure1 {
            Closure1(Fn &fn, const Arg1 &arg1) : _fn(fn), _arg1(arg1) {}

            size_t operator()(T &bg) { return _fn(bg, _arg1); }

        private:
            Fn &_fn;
            const Arg1 &_arg1;
        };

        template<typename Fn, typename Arg1, typename Arg2>
        struct Closure2 {
            Closure2(Fn &fn, const Arg1 &arg1, const Arg2 &arg2) : _fn(fn), _arg1(arg1), _arg2(arg2) {}

            size_t operator()(T &bg) { return _fn(bg, _arg1, _arg2); }

        private:
            Fn &_fn;
            const Arg1 &_arg1;
            const Arg2 &_arg2;
        };

        const T *UnsafeRead() const { return _data + _index.load(std::memory_order_acquire); }

        Wrapper *AddWrapper();

        void RemoveWrapper(Wrapper *);

        // Foreground and background void.
        T _data[2];

        // Index of foreground instance.
        std::atomic<int> _index;

        // Key to access thread-local wrappers.
        bool _created_key;
        pthread_key_t _wrapper_key;

        // All thread-local instances.
        std::vector<Wrapper *> _wrappers;

        // Sequence access to _wrappers.
        MUTEX _wrappers_mutex;

        // Sequence modifications.
        MUTEX _modify_mutex;
    };

    static const pthread_key_t INVALID_PTHREAD_KEY = (pthread_key_t) -1;

    template<typename T, typename MUTEX, typename TLS>
    class DoublyBufferedDataWrapperBase {
    public:
        TLS &user_tls() { return _user_tls; }

    protected:
        TLS _user_tls;
    };

    template<typename T, typename MUTEX>
    class DoublyBufferedDataWrapperBase<T, MUTEX, Void> {
    };

    template<typename T, typename MUTEX, typename TLS>
    class DoublyBufferedData<T, MUTEX, TLS>::Wrapper : public DoublyBufferedDataWrapperBase<T, MUTEX, TLS> {
        friend class DoublyBufferedData;

    public:
        explicit Wrapper(DoublyBufferedData *c) : _control(c) {}

        ~Wrapper() {
            if (_control != nullptr) {
                _control->RemoveWrapper(this);
            }
        }

        // _mutex will be locked by the calling pthread and DoublyBufferedData.
        // Most of the time, no modifications are done, so the mutex is
        // uncontended and fast.
        inline void BeginRead() { _mutex.lock(); }

        inline void EndRead() { _mutex.unlock(); }

        inline void WaitReadDone() { std::lock_guard<std::mutex> lock(_mutex); }

    private:
        DoublyBufferedData *_control;
        std::mutex _mutex;
    };

    // Called when thread initializes thread-local wrapper.
    template<typename T, typename MUTEX, typename TLS>
    typename DoublyBufferedData<T, MUTEX, TLS>::Wrapper *DoublyBufferedData<T, MUTEX, TLS>::AddWrapper() {
        std::unique_ptr<Wrapper> w(new(std::nothrow) Wrapper(this));
        if (nullptr == w) {
            return nullptr;
        }
        try {
            std::lock_guard<std::mutex> lock(_wrappers_mutex);
            _wrappers.push_back(w.get());
        } catch (std::exception &e) {
            return nullptr;
        }
        return w.release();
    }

    // Called when thread quits.
    template<typename T, typename MUTEX, typename TLS>
    void DoublyBufferedData<T, MUTEX, TLS>::RemoveWrapper(typename DoublyBufferedData<T, MUTEX, TLS>::Wrapper *w) {
        if (nullptr == w) {
            return;
        }
        std::lock_guard<std::mutex> lock(_wrappers_mutex);
        for (size_t i = 0; i < _wrappers.size(); ++i) {
            if (_wrappers[i] == w) {
                _wrappers[i] = _wrappers.back();
                _wrappers.pop_back();
                return;
            }
        }
    }

    template<typename T, typename MUTEX, typename TLS>
    DoublyBufferedData<T, MUTEX, TLS>::DoublyBufferedData() : _index(0), _created_key(false), _wrapper_key(0) {
        _wrappers.reserve(64);
        const int rc = pthread_key_create(&_wrapper_key, delete_object<Wrapper>);
        if (rc != 0) {
            TLOG_ERROR("Fail to pthread_key_create: {} errno: ", rc, errno);
        } else {
            _created_key = true;
        }
        // Initialize _data for some POD types. This is essential for pointer
        // types because they should be Read() as nullptr before any Modify().
        if (std::is_integral<T>::value || std::is_floating_point<T>::value || std::is_pointer<T>::value ||
            std::is_member_function_pointer<T>::value) {
            _data[0] = T();
            _data[1] = T();
        }
    }

    template<typename T, typename MUTEX, typename TLS>
    DoublyBufferedData<T, MUTEX, TLS>::~DoublyBufferedData() {
        // User is responsible for synchronizations between Read()/Modify() and
        // this function.
        if (_created_key) {
            pthread_key_delete(_wrapper_key);
        }

        {
            std::lock_guard<std::mutex> lock(_wrappers_mutex);
            for (size_t i = 0; i < _wrappers.size(); ++i) {
                _wrappers[i]->_control = nullptr;  // hack: disable removal.
                delete _wrappers[i];
            }
            _wrappers.clear();
        }
    }

    template<typename T, typename MUTEX, typename TLS>
    int DoublyBufferedData<T, MUTEX, TLS>::Read(typename DoublyBufferedData<T, MUTEX, TLS>::ScopedPtr *ptr) {
        if (TURBO_UNLIKELY(!_created_key)) {
            return -1;
        }
        Wrapper *w = static_cast<Wrapper *>(pthread_getspecific(_wrapper_key));
        if (TURBO_LIKELY(w != nullptr)) {
            w->BeginRead();
            ptr->_data = UnsafeRead();
            ptr->_w = w;
            return 0;
        }
        w = AddWrapper();
        if (TURBO_LIKELY(w != nullptr)) {
            const int rc = pthread_setspecific(_wrapper_key, w);
            if (rc == 0) {
                w->BeginRead();
                ptr->_data = UnsafeRead();
                ptr->_w = w;
                return 0;
            }
        }
        return -1;
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn>
    size_t DoublyBufferedData<T, MUTEX, TLS>::Modify(Fn &fn) {
        // _modify_mutex sequences modifications. Using a separate mutex rather
        // than _wrappers_mutex is to avoid blocking threads calling
        // AddWrapper() or RemoveWrapper() too long. Most of the time, modifications
        // are done by one thread, contention should be negligible.
        std::lock_guard<std::mutex> lock(_modify_mutex);
        int bg_index = !_index.load(std::memory_order_relaxed);
        // background instance is not accessed by other threads, being safe to
        // modify.
        const size_t ret = fn(_data[bg_index]);
        if (!ret) {
            return 0;
        }

        // Publish, flip background and foreground.
        // The release fence matches with the acquire fence in UnsafeRead() to
        // make readers which just begin to read the new foreground instance see
        // all changes made in fn.
        _index.store(bg_index, std::memory_order_release);
        bg_index = !bg_index;

        // Wait until all threads finishes current reading. When they begin next
        // read, they should see updated _index.
        {
            std::lock_guard<std::mutex> lock(_wrappers_mutex);
            for (size_t i = 0; i < _wrappers.size(); ++i) {
                _wrappers[i]->WaitReadDone();
            }
        }

        const size_t ret2 = fn(_data[bg_index]);
        if (ret2 != ret) {
            TLOG_ERROR("something worong happened, index={}", _index.load(std::memory_order_relaxed));
        }
        return ret2;
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn, typename Arg1>
    size_t DoublyBufferedData<T, MUTEX, TLS>::Modify(Fn &fn, const Arg1 &arg1) {
        Closure1<Fn, Arg1> c(fn, arg1);
        return Modify(c);
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn, typename Arg1, typename Arg2>
    size_t DoublyBufferedData<T, MUTEX, TLS>::Modify(Fn &fn, const Arg1 &arg1, const Arg2 &arg2) {
        Closure2<Fn, Arg1, Arg2> c(fn, arg1, arg2);
        return Modify(c);
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn>
    size_t DoublyBufferedData<T, MUTEX, TLS>::ModifyWithForeground(Fn &fn) {
        WithFG0<Fn> c(fn, _data);
        return Modify(c);
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn, typename Arg1>
    size_t DoublyBufferedData<T, MUTEX, TLS>::ModifyWithForeground(Fn &fn, const Arg1 &arg1) {
        WithFG1<Fn, Arg1> c(fn, _data, arg1);
        return Modify(c);
    }

    template<typename T, typename MUTEX, typename TLS>
    template<typename Fn, typename Arg1, typename Arg2>
    size_t DoublyBufferedData<T, MUTEX, TLS>::ModifyWithForeground(Fn &fn, const Arg1 &arg1,
                                                                   const Arg2 &arg2) {
        WithFG2<Fn, Arg1, Arg2> c(fn, _data, arg1, arg2);
        return Modify(c);
    }

}  // namespace turbo

#endif  // TURBO_CONCURRENT_DOUBLY_BUFFERED_DATA_H_
