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
#include "turbo/log/logging.h"

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

        inline void WaitReadDone() { std::lock_guard<MUTEX> lock(_mutex); }

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
            std::lock_guard<MUTEX> lock(_wrappers_mutex);
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
        std::lock_guard<MUTEX> lock(_wrappers_mutex);
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
            std::lock_guard<MUTEX> lock(_wrappers_mutex);
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
        std::lock_guard<MUTEX> lock(_modify_mutex);
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
            std::lock_guard<MUTEX> lock(_wrappers_mutex);
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
/*
 * Run on (12 X 4400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 1280 KiB (x6)
  L3 Unified 18432 KiB (x1)
Load Average: 2.44, 1.80, 0.85
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
--------------------------------------------------------------------------------------
Benchmark                                            Time             CPU   Iterations
--------------------------------------------------------------------------------------
BM_SingleGetFlag_bool/threads:1                  0.241 ns        0.241 ns   1000000000
BM_SingleGetFlag_bool/threads:2                  0.120 ns        0.240 ns   2000000000
BM_SingleGetFlag_bool/threads:4                  0.063 ns        0.251 ns   2772672648
BM_SingleGetFlag_bool/threads:8                  0.045 ns        0.357 ns   1904039680
BM_SingleGetFlag_bool/threads:16                 0.034 ns        0.468 ns   1517329744
BM_SingleGetFlag_int16_t/threads:1               0.236 ns        0.236 ns   1000000000
BM_SingleGetFlag_int16_t/threads:2               0.119 ns        0.238 ns   2000000000
BM_SingleGetFlag_int16_t/threads:4               0.067 ns        0.269 ns   2712307884
BM_SingleGetFlag_int16_t/threads:8               0.046 ns        0.364 ns   1840661688
BM_SingleGetFlag_int16_t/threads:16              0.036 ns        0.479 ns   1442514560
BM_SingleGetFlag_uint16_t/threads:1              0.234 ns        0.234 ns   1000000000
BM_SingleGetFlag_uint16_t/threads:2              0.120 ns        0.240 ns   2000000000
BM_SingleGetFlag_uint16_t/threads:4              0.063 ns        0.251 ns   2786541452
BM_SingleGetFlag_uint16_t/threads:8              0.045 ns        0.363 ns   1858151992
BM_SingleGetFlag_uint16_t/threads:16             0.035 ns        0.479 ns   1514100496
BM_SingleGetFlag_int32_t/threads:1               0.238 ns        0.238 ns   1000000000
BM_SingleGetFlag_int32_t/threads:2               0.122 ns        0.244 ns   2000000000
BM_SingleGetFlag_int32_t/threads:4               0.065 ns        0.262 ns   2552994140
BM_SingleGetFlag_int32_t/threads:8               0.046 ns        0.371 ns   1869080200
BM_SingleGetFlag_int32_t/threads:16              0.037 ns        0.474 ns   1589571136
BM_SingleGetFlag_uint32_t/threads:1              0.230 ns        0.230 ns   1000000000
BM_SingleGetFlag_uint32_t/threads:2              0.120 ns        0.239 ns   2000000000
BM_SingleGetFlag_uint32_t/threads:4              0.063 ns        0.251 ns   2738512508
BM_SingleGetFlag_uint32_t/threads:8              0.043 ns        0.345 ns   1859093688
BM_SingleGetFlag_uint32_t/threads:16             0.036 ns        0.474 ns   1589898512
BM_SingleGetFlag_int64_t/threads:1               0.235 ns        0.235 ns   1000000000
BM_SingleGetFlag_int64_t/threads:2               0.121 ns        0.242 ns   2000000000
BM_SingleGetFlag_int64_t/threads:4               0.063 ns        0.250 ns   2787110200
BM_SingleGetFlag_int64_t/threads:8               0.044 ns        0.349 ns   1878117000
BM_SingleGetFlag_int64_t/threads:16              0.035 ns        0.464 ns   1532827728
BM_SingleGetFlag_uint64_t/threads:1              0.245 ns        0.245 ns   1000000000
BM_SingleGetFlag_uint64_t/threads:2              0.120 ns        0.239 ns   2000000000
BM_SingleGetFlag_uint64_t/threads:4              0.063 ns        0.251 ns   2714069968
BM_SingleGetFlag_uint64_t/threads:8              0.045 ns        0.364 ns   1620935688
BM_SingleGetFlag_uint64_t/threads:16             0.035 ns        0.472 ns   1504979744
BM_SingleGetFlag_double/threads:1                0.231 ns        0.231 ns   1000000000
BM_SingleGetFlag_double/threads:2                0.121 ns        0.241 ns   2000000000
BM_SingleGetFlag_double/threads:4                0.060 ns        0.241 ns   2851158132
BM_SingleGetFlag_double/threads:8                0.046 ns        0.365 ns   1880751752
BM_SingleGetFlag_double/threads:16               0.036 ns        0.465 ns   1487889072
BM_SingleGetFlag_float/threads:1                 0.235 ns        0.235 ns   1000000000
BM_SingleGetFlag_float/threads:2                 0.121 ns        0.242 ns   2000000000
BM_SingleGetFlag_float/threads:4                 0.069 ns        0.277 ns   2401789992
BM_SingleGetFlag_float/threads:8                 0.044 ns        0.354 ns   1825677056
BM_SingleGetFlag_float/threads:16                0.038 ns        0.473 ns   1629596288
BM_SingleGetFlag_String/threads:1                 21.7 ns         21.7 ns     32397279
BM_SingleGetFlag_String/threads:2                  129 ns          259 ns      3738618
BM_SingleGetFlag_String/threads:4                  158 ns          621 ns      1125284
BM_SingleGetFlag_String/threads:8                  194 ns         1504 ns       457464
BM_SingleGetFlag_String/threads:16                6017 ns        71383 ns        16000
BM_SingleGetFlag_VectorOfStrings/threads:1        21.8 ns         21.7 ns     31396132
BM_SingleGetFlag_VectorOfStrings/threads:2         129 ns          259 ns      3603690
BM_SingleGetFlag_VectorOfStrings/threads:4         166 ns          655 ns      1109920
BM_SingleGetFlag_VectorOfStrings/threads:8         213 ns         1653 ns       443240
BM_SingleGetFlag_VectorOfStrings/threads:16       2879 ns        33525 ns        16000
BM_SingleGetFlag_OptionalInt/threads:1           0.230 ns        0.230 ns   1000000000
BM_SingleGetFlag_OptionalInt/threads:2           0.120 ns        0.240 ns   2000000000
BM_SingleGetFlag_OptionalInt/threads:4           0.063 ns        0.251 ns   2804535972
BM_SingleGetFlag_OptionalInt/threads:8           0.046 ns        0.367 ns   1867399560
BM_SingleGetFlag_OptionalInt/threads:16          0.036 ns        0.480 ns   1602370320
BM_SingleGetFlag_OptionalString/threads:1         20.9 ns         20.9 ns     33365712
BM_SingleGetFlag_OptionalString/threads:2         96.3 ns          193 ns      3389716
BM_SingleGetFlag_OptionalString/threads:4          182 ns          703 ns       983512
BM_SingleGetFlag_OptionalString/threads:8          207 ns         1579 ns       436936
BM_SingleGetFlag_OptionalString/threads:16       11947 ns       141159 ns        21520
BM_SingleGetFlag_Duration/threads:1              0.451 ns        0.450 ns   1000000000
BM_SingleGetFlag_Duration/threads:2              0.234 ns        0.468 ns   1457142742
BM_SingleGetFlag_Duration/threads:4              0.123 ns        0.493 ns   1421860400
BM_SingleGetFlag_Duration/threads:8              0.088 ns        0.708 ns    800000000
BM_SingleGetFlag_Duration/threads:16             0.056 ns        0.761 ns    884753296
BM_SingleGetFlag_UDT/threads:1                    19.4 ns         19.4 ns     35604812
BM_SingleGetFlag_UDT/threads:2                     121 ns          242 ns      3016376
BM_SingleGetFlag_UDT/threads:4                     174 ns          688 ns       984480
BM_SingleGetFlag_UDT/threads:8                     200 ns         1558 ns       437336
BM_SingleGetFlag_UDT/threads:16                   2828 ns        33680 ns        81024
BM_ManyGetFlag_bool/threads:1                    0.367 ns        0.367 ns   1000000512
BM_ManyGetFlag_bool/threads:2                    0.192 ns        0.384 ns   1797033984
BM_ManyGetFlag_bool/threads:4                    0.102 ns        0.407 ns   1672900608
BM_ManyGetFlag_bool/threads:8                    0.064 ns        0.513 ns   1308524544
BM_ManyGetFlag_int16_t/threads:1                 0.372 ns        0.372 ns   1000000512
BM_ManyGetFlag_int16_t/threads:2                 0.190 ns        0.380 ns   1796298752
BM_ManyGetFlag_int16_t/threads:4                 0.100 ns        0.399 ns   1722232832
BM_ManyGetFlag_int16_t/threads:8                 0.068 ns        0.543 ns   1290584064
BM_ManyGetFlag_uint16_t/threads:1                0.365 ns        0.365 ns   1000000512
BM_ManyGetFlag_uint16_t/threads:2                0.190 ns        0.380 ns   1782835200
BM_ManyGetFlag_uint16_t/threads:4                0.100 ns        0.400 ns   1743142912
BM_ManyGetFlag_uint16_t/threads:8                0.067 ns        0.533 ns   1309335552
BM_ManyGetFlag_int32_t/threads:1                 0.369 ns        0.369 ns   1000000512
BM_ManyGetFlag_int32_t/threads:2                 0.201 ns        0.402 ns   1751091200
BM_ManyGetFlag_int32_t/threads:4                 0.100 ns        0.400 ns   1740677120
BM_ManyGetFlag_int32_t/threads:8                 0.065 ns        0.522 ns   1192263680
BM_ManyGetFlag_uint32_t/threads:1                0.376 ns        0.376 ns   1000000512
BM_ManyGetFlag_uint32_t/threads:2                0.211 ns        0.421 ns   1633337344
BM_ManyGetFlag_uint32_t/threads:4                0.103 ns        0.411 ns   1708937216
BM_ManyGetFlag_uint32_t/threads:8                0.065 ns        0.516 ns   1182900224
BM_ManyGetFlag_int64_t/threads:1                 0.383 ns        0.383 ns   1000000512
BM_ManyGetFlag_int64_t/threads:2                 0.193 ns        0.387 ns   1803782144
BM_ManyGetFlag_int64_t/threads:4                 0.097 ns        0.387 ns   1776975872
BM_ManyGetFlag_int64_t/threads:8                 0.063 ns        0.505 ns   1353351168
BM_ManyGetFlag_uint64_t/threads:1                0.365 ns        0.365 ns   1000000512
BM_ManyGetFlag_uint64_t/threads:2                0.190 ns        0.381 ns   1851277312
BM_ManyGetFlag_uint64_t/threads:4                0.099 ns        0.396 ns   1775931392
BM_ManyGetFlag_uint64_t/threads:8                0.065 ns        0.520 ns   1333583872
BM_ManyGetFlag_double/threads:1                  0.460 ns        0.460 ns   1000000512
BM_ManyGetFlag_double/threads:2                  0.242 ns        0.484 ns   1367752704
BM_ManyGetFlag_double/threads:4                  0.125 ns        0.500 ns   1390780416
BM_ManyGetFlag_double/threads:8                  0.073 ns        0.581 ns   1179148288
BM_ManyGetFlag_float/threads:1                   0.562 ns        0.562 ns   1000000512
BM_ManyGetFlag_float/threads:2                   0.287 ns        0.575 ns   1213546496
BM_ManyGetFlag_float/threads:4                   0.144 ns        0.574 ns   1215410176
BM_ManyGetFlag_float/threads:8                   0.093 ns        0.746 ns    897146880
BM_ManyGetFlag_String/threads:1                   23.1 ns         23.1 ns     28793856
BM_ManyGetFlag_String/threads:2                   23.0 ns         46.1 ns     20480000
BM_ManyGetFlag_String/threads:4                   10.6 ns         42.2 ns     26595328
BM_ManyGetFlag_String/threads:8                   4.95 ns         39.6 ns     17473536
BM_ManyGetFlag_VectorOfStrings/threads:1          22.5 ns         22.5 ns     29986816
BM_ManyGetFlag_VectorOfStrings/threads:2          27.6 ns         55.3 ns     11888640
BM_ManyGetFlag_VectorOfStrings/threads:4          10.2 ns         40.6 ns     25448448
BM_ManyGetFlag_VectorOfStrings/threads:8          5.13 ns         41.0 ns     17670144
BM_ManyGetFlag_OptionalInt/threads:1             0.414 ns        0.414 ns   1000000512
BM_ManyGetFlag_OptionalInt/threads:2             0.219 ns        0.438 ns   1601107968
BM_ManyGetFlag_OptionalInt/threads:4             0.110 ns        0.440 ns   1582391296
BM_ManyGetFlag_OptionalInt/threads:8             0.086 ns        0.688 ns    819200000
BM_ManyGetFlag_OptionalString/threads:1           21.5 ns         21.5 ns     31802368
BM_ManyGetFlag_OptionalString/threads:2           30.5 ns         61.0 ns     20480000
BM_ManyGetFlag_OptionalString/threads:4           16.1 ns         64.4 ns     16146432
BM_ManyGetFlag_OptionalString/threads:8           5.02 ns         40.1 ns     17522688
BM_ManyGetFlag_Duration/threads:1                 4.79 ns         4.79 ns    141706240
BM_ManyGetFlag_Duration/threads:2                 2.49 ns         4.99 ns    139610112
BM_ManyGetFlag_Duration/threads:4                 1.31 ns         5.23 ns    126611456
BM_ManyGetFlag_Duration/threads:8                0.702 ns         5.62 ns    122970112
BM_ManyGetFlag_UDT/threads:1                      20.0 ns         20.0 ns     34460672
BM_ManyGetFlag_UDT/threads:2                      23.1 ns         46.2 ns     20480000
BM_ManyGetFlag_UDT/threads:4                      13.6 ns         54.5 ns     20369408
BM_ManyGetFlag_UDT/threads:8                      4.45 ns         35.6 ns     19628032
BM_ThreadedFindCommandLineFlag/threads:1           115 ns          115 ns      5992448
BM_ThreadedFindCommandLineFlag/threads:2          58.7 ns          117 ns      5955584
BM_ThreadedFindCommandLineFlag/threads:4          31.0 ns          124 ns      5419008
BM_ThreadedFindCommandLineFlag/threads:8          18.6 ns          149 ns      4710400
BM_ThreadedFindCommandLineFlag/threads:16         13.0 ns          176 ns      4079616

 */