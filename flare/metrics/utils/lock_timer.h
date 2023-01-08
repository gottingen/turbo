
#ifndef  FLARE_VARIABLE_LOCK_TIMER_H_
#define  FLARE_VARIABLE_LOCK_TIMER_H_

#include "flare/times/time.h"             // flare::stop_watcher
#include "flare/base/scoped_lock.h"      // std::lock_guard std::unique_lock
#include "flare/metrics/recorder.h"         // IntRecorder
#include "flare/metrics/latency_recorder.h" // LatencyRecorder

// Monitor the time for acquiring a lock.
// We provide some wrappers of mutex which can also be maintained by 
// std::lock_guard and std::unique_lock to record the time it takes to wait for 
// the acquisition of the very mutex (in microsecond) except for the contention
// caused by condition variables which unlock the mutex before waiting and lock
// the same mutex after waking up.
//
// About Performance: 
// The utilities are designed and implemented to be suitable to measure the
// mutex from all the common scenarios. Saying that you can use them freely 
// without concerning about the overhead. Except that the mutex is very 
// frequently acquired (>1M/s) with very little contention, in which case, the
// overhead of timers and variables is noticable.
// 
// There are two kinds of special Mutex:
//  - MutexWithRecorder: Create a mutex along with a shared IntRecorder which
//                       only records the average latency from intialization
//  - MutexWithLatencyRecorder: Create a mutex along with a shared
//                              LatencyRecorder which also provides percentile
//                              calculation, time window management besides
//                              IntRecorder.
//
// Examples:
// #include "flare/metrics/utils/lock_timer.h"
//
// flare::LatencyRecorder
//     g_mutex_contention("my_mutex_contention");
//                       // ^^^
//                       // you can replace this with a meaningful name
//
// typedef ::flare::MutexWithLatencyRecorder<pthread_mutex_t> my_mutex_t;
//                                       // ^^^
//                                       // you can use std::mutex (since c++11)
//                                       // or fiber_mutex_t (in fiber)
//
// // Define the mutex
// my_mutex_t mutex(g_mutex_contention);
//
// // Use it with std::lock_guard
// void critical_routine_with_lock_guard() {
//     std::lock_guard<my_mutex_t> guard(mutex);
//     // ^^^
//     // Or you can use FLARE_SCOPED_LOCK(mutex) to make it simple
//     ... 
//     doing something inside the critical section
//     ...
//     // and |mutex| is auto unlocked and this contention is recorded out of 
//     // the scope
// }
// 
// // Use it with unique_lock
// void  critical_routine_with_unique_lock() {
//     std::unique_lock<my_mutex_t> lck(mutex);
//     std::condition_variable cond; // available since C++11
//     ...
//     doing something inside the critical section
//     ...
//     cond.wait(lck);  // It's ok if my_mutex_t is defined with the template
//                      // parameter being std::mutex
//     ...
//     doing other things when come back into the critical section
//     ...
//     // and |mutex| is auto unlocked and this contention is recorded out of
//     // the scope
// }

namespace flare {
    namespace utils {
// To be compatible with the old version
        using namespace ::flare;
    }  // namespace utils
}  // namespace flare

namespace flare {

    // Specialize MutexConstructor and MutexDestructor for the Non-RAII mutexes such
    // as pthread_mutex_t
    template<typename Mutex>
    struct MutexConstructor {
        bool operator()(Mutex *) const { return true; }
    };

    template<typename Mutex>
    struct MutexDestructor {
        bool operator()(Mutex *) const { return true; }
    };

// Specialize for pthread_mutex_t
    template<>
    struct MutexConstructor<pthread_mutex_t> {
        bool operator()(pthread_mutex_t *mutex) const {
#ifndef NDEBUG
            const int rc = pthread_mutex_init(mutex, nullptr);
            FLARE_CHECK_EQ(0, rc) << "Fail to init pthread_mutex, " << flare_error(rc);
            return rc == 0;
#else
            return pthread_mutex_init(mutex, nullptr) == 0;
#endif
        }
    };

    template<>
    struct MutexDestructor<pthread_mutex_t> {
        bool operator()(pthread_mutex_t *mutex) const {
#ifndef NDEBUG
            const int rc = pthread_mutex_destroy(mutex);
            FLARE_CHECK_EQ(0, rc) << "Fail to destroy pthread_mutex, " << flare_error(rc);
            return rc == 0;
#else
            return pthread_mutex_destroy(mutex) == 0;
#endif
        }
    };

    namespace metrics_internal {

        template<typename Mutex, typename Recorder,
                typename MCtor, typename MDtor>
        class MutexWithRecorderBase {
            FLARE_DISALLOW_COPY_AND_ASSIGN(MutexWithRecorderBase);

        public:
            typedef Mutex mutex_type;
            typedef Recorder recorder_type;
            typedef MutexWithRecorderBase<Mutex, Recorder,
                    MCtor, MDtor> self_type;

            explicit MutexWithRecorderBase(recorder_type &recorder)
                    : _recorder(&recorder) {
                MCtor()(&_mutex);
            }

            MutexWithRecorderBase() : _recorder(nullptr) {
                MCtor()(&_mutex);
            }

            ~MutexWithRecorderBase() {
                MDtor()(&_mutex);
            }

            void set_recorder(recorder_type &recorder) {
                _recorder = &recorder;
            }

            mutex_type &mutex() { return _mutex; }

            operator mutex_type &() { return _mutex; }

            template<typename T>
            self_type &operator<<(T value) {
                if (_recorder) {
                    *_recorder << value;
                }
                return *this;
            }

        private:
            mutex_type _mutex;
            // We don't own _recorder. Make sure it is valid before the destruction of
            // this instance
            recorder_type *_recorder;
        };

        template<typename Mutex>
        class LockGuardBase {
            FLARE_DISALLOW_COPY_AND_ASSIGN(LockGuardBase);

        public:
            LockGuardBase(Mutex &m)
                    : _timer(m), _lock_guard(m.mutex()) {
                _timer.timer.stop();
            }

        private:

            // This trick makes the recoding happens after the destructor of _lock_guard
            struct TimerAndMutex {
                TimerAndMutex(Mutex &m)
                        : timer(flare::stop_watcher::STARTED), mutex(&m) {}

                ~TimerAndMutex() {
                    *mutex << timer.u_elapsed();
                }

                flare::stop_watcher timer;
                Mutex *mutex;
            };

            // Don't change the order of the fields as the implementation depends on
            // the order of the constructors and destructors
            TimerAndMutex _timer;
            std::lock_guard<typename Mutex::mutex_type> _lock_guard;
        };

        template<typename Mutex>
        class UniqueLockBase {
            FLARE_DISALLOW_COPY_AND_ASSIGN(UniqueLockBase);

        public:
            typedef Mutex mutex_type;

            explicit UniqueLockBase(mutex_type &mutex)
                    : _timer(flare::stop_watcher::STARTED), _lock(mutex.mutex()),
                      _mutex(&mutex) {
                _timer.stop();
            }

            UniqueLockBase(mutex_type &mutex, std::defer_lock_t defer_lock)
                    : _timer(), _lock(mutex.mutex(), defer_lock), _mutex(&mutex) {
            }

            UniqueLockBase(mutex_type &mutex, std::try_to_lock_t try_to_lock)
                    : _timer(flare::stop_watcher::STARTED), _lock(mutex.mutex(), try_to_lock), _mutex(&mutex) {

                _timer.stop();
                if (!owns_lock()) {
                    *_mutex << _timer.u_elapsed();
                }
            }

            ~UniqueLockBase() {
                if (_lock.owns_lock()) {
                    unlock();
                }
            }

            operator std::unique_lock<typename Mutex::mutex_type> &() { return _lock; }

            void lock() {
                _timer.start();
                _lock.lock();
                _timer.stop();
            }

            bool try_lock() {
                _timer.start();
                const bool rc = _lock.try_lock();
                _timer.stop();
                if (!rc) {
                    _mutex->recorder() << _timer.u_elapsed();
                }
                return rc;
            }

            void unlock() {
                _lock.unlock();
                // Recorde the time out of the critical section
                *_mutex << _timer.u_elapsed();
            }

            mutex_type *release() {
                if (_lock.owns_lock()) {
                    // We have to recorde this time in the critical section owtherwise
                    // the event will be lost
                    *_mutex << _timer.u_elapsed();
                }
                mutex_type *saved_mutex = _mutex;
                _mutex = nullptr;
                _lock.release();
                return saved_mutex;
            }

            mutex_type *mutex() { return _mutex; }

            bool owns_lock() const { return _lock.owns_lock(); }

            operator bool() const { return static_cast<bool>(_lock); }

#if __cplusplus >= 201103L

            template<class Rep, class Period>
            bool try_lock_for(
                    const std::chrono::duration<Rep, Period> &timeout_duration) {
                _timer.start();
                const bool rc = _lock.try_lock_for(timeout_duration);
                _timer.stop();
                if (!rc) {
                    *_mutex << _timer.u_elapsed();
                }
                return rc;
            }

            template<class Clock, class Duration>
            bool try_lock_until(
                    const std::chrono::time_point<Clock, Duration> &timeout_time) {
                _timer.start();
                const bool rc = _lock.try_lock_until(timeout_time);
                _timer.stop();
                if (!rc) {
                    // Out of the criticle section. Otherwise the time will be recorded
                    // in unlock
                    *_mutex << _timer.u_elapsed();
                }
                return rc;
            }

#endif

        private:
            // Don't change the order or timer and _lck;
            flare::stop_watcher _timer;
            std::unique_lock<typename Mutex::mutex_type> _lock;
            mutex_type *_mutex;
        };

    }  // namespace detail

// Wappers of Mutex along with a shared LatencyRecorder 
    template<typename Mutex>
    struct MutexWithRecorder
            : public metrics_internal::MutexWithRecorderBase<
                    Mutex, IntRecorder,
                    MutexConstructor<Mutex>, MutexDestructor<Mutex> > {
        typedef metrics_internal::MutexWithRecorderBase<
                Mutex, IntRecorder,
                MutexConstructor<Mutex>, MutexDestructor<Mutex> > Base;

        explicit MutexWithRecorder(IntRecorder &recorder)
                : Base(recorder) {}

        MutexWithRecorder() : Base() {}

    };

// Wappers of Mutex along with a shared LatencyRecorder
    template<typename Mutex>
    struct MutexWithLatencyRecorder
            : public metrics_internal::MutexWithRecorderBase<
                    Mutex, LatencyRecorder,
                    MutexConstructor<Mutex>, MutexDestructor<Mutex> > {
        typedef metrics_internal::MutexWithRecorderBase<
                Mutex, LatencyRecorder,
                MutexConstructor<Mutex>, MutexDestructor<Mutex> > Base;

        explicit MutexWithLatencyRecorder(LatencyRecorder &recorder)
                : Base(recorder) {}

        MutexWithLatencyRecorder() : Base() {}
    };

}  // namespace flare

namespace std {

    // Specialize lock_guard and unique_lock
    template<typename Mutex>
    class lock_guard<flare::MutexWithRecorder<Mutex> >
            : public ::flare::metrics_internal::
            LockGuardBase<::flare::MutexWithRecorder<Mutex> > {
    public:
        typedef ::flare::metrics_internal::
        LockGuardBase<flare::MutexWithRecorder<Mutex> > Base;

        explicit lock_guard(::flare::MutexWithRecorder<Mutex> &mutex)
                : Base(mutex) {}
    };

    template<typename Mutex>
    class lock_guard<flare::MutexWithLatencyRecorder<Mutex> >
            : public ::flare::metrics_internal::
            LockGuardBase<::flare::MutexWithLatencyRecorder<Mutex> > {
    public:
        typedef ::flare::metrics_internal::
        LockGuardBase<flare::MutexWithLatencyRecorder<Mutex> > Base;

        explicit lock_guard(::flare::MutexWithLatencyRecorder<Mutex> &mutex)
                : Base(mutex) {}
    };

    template<typename Mutex>
    class unique_lock<flare::MutexWithRecorder<Mutex> >
            : public ::flare::metrics_internal::
            UniqueLockBase<::flare::MutexWithRecorder<Mutex> > {
    public:
        typedef ::flare::metrics_internal::
        UniqueLockBase<::flare::MutexWithRecorder<Mutex> > Base;
        typedef typename Base::mutex_type mutex_type;

        explicit unique_lock(mutex_type &mutex)
                : Base(mutex) {}

        unique_lock(mutex_type &mutex, std::defer_lock_t defer_lock)
                : Base(mutex, defer_lock) {}

        unique_lock(mutex_type &mutex, std::try_to_lock_t try_to_lock)
                : Base(mutex, try_to_lock) {}
    };

    template<typename Mutex>
    class unique_lock<flare::MutexWithLatencyRecorder<Mutex> >
            : public ::flare::metrics_internal::
            UniqueLockBase<::flare::MutexWithLatencyRecorder<Mutex> > {
    public:
        typedef ::flare::metrics_internal::
        UniqueLockBase<::flare::MutexWithLatencyRecorder<Mutex> > Base;
        typedef typename Base::mutex_type mutex_type;

        explicit unique_lock(mutex_type &mutex)
                : Base(mutex) {}

        unique_lock(mutex_type &mutex, std::defer_lock_t defer_lock)
                : Base(mutex, defer_lock) {}

        unique_lock(mutex_type &mutex, std::try_to_lock_t try_to_lock)
                : Base(mutex, try_to_lock) {}
    };

}  // namespace std

#endif  // FLARE_VARIABLE_LOCK_TIMER_H_
