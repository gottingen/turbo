

#include "flare/thread/thread.h"
#include "flare/base/profile.h"
#include "flare/system/sysinfo.h"
#include "flare/log/logging.h"
#include "flare/bootstrap/bootstrap.h"
#include <signal.h>
#include <algorithm>  // std::sort
#include <unordered_set>
#include <cstdarg>
#include <cstdio>
#include <pthread.h>

#if defined(__APPLE__)


#include <mach/thread_act.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>

#elif defined(__FreeBSD__)
#include <pthread.h>
#include <pthread_np.h>
#include <unistd.h>
#include <thread>
#else
#include <pthread.h>
#include <unistd.h>
#include <thread>
#endif

namespace flare {

    __thread thread::inner_data* local_impl = nullptr;
    /// reserve 0 for main thread
    std::atomic<int32_t> g_thread_id{1};
    __thread int32_t local_thread_id = -1;

    void *thread::thread_func(void *arg) {
        thread *ptr = (thread *) arg;
        std::shared_ptr<thread::inner_data> impl = ptr->_impl;
        local_impl = impl.get();
        local_thread_id = impl->index;
        impl->name = flare::string_format("{}#{}", impl->prefix, impl->index);
        thread::set_name(impl->name);
        FLARE_LOG(INFO) << "start thread: " << impl->name;
        ptr->set_affinity();
        impl->start_latch.count_down();
        impl->func();
        local_impl = nullptr;
        FLARE_LOG(INFO) << "exit thread: " << impl->name;
        return nullptr;
    }

    void thread::set_affinity() {
        return ;
        /*
        FLARE_CHECK(_impl);
        if (_impl->affinity == -1) {
            return;
        }

#if defined(__linux__) && !defined(__ANDROID__)
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            for (size_t i = 0; i < count; i++) {
              CPU_SET(option.affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#elif defined(__FreeBSD__)
            cpuset_t cpuset;
            CPU_ZERO(&cpuset);
            for (size_t i = 0; i < count; i++) {
              CPU_SET(option.affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpuset_t), &cpuset);
#else
        FLARE_CHECK(!flare::core_affinity::supported) << "Attempting to use thread affinity on a unsupported platform";
#endif
         */
    }

    thread::~thread() {
        if (_impl) {
            FLARE_LOG(WARNING) << "thread: " << _impl->name
                               << "was not called before destruction, detach instead.";
            detach();
        }
    }

    std::string thread::name() const {
        FLARE_CHECK(_impl);
        return _impl->name;

    }

    std::string thread::current_name() {
        if(local_impl) {
            return local_impl->name;
        }
        return "";
    }

    void thread::join(void **ptr) {
        if (!_impl) {
            return;
        }
        ::pthread_join(_impl->thread_id, ptr);
        _impl = nullptr;
    }

    void thread::detach() {
        if (!_impl) {
            return;
        }
        if(!_impl->detached) {
            ::pthread_detach(_impl->thread_id);
            _impl = nullptr;
        }
    }

    void do_nothing_handler(int) {}

    static pthread_once_t register_sigurg_once = PTHREAD_ONCE_INIT;

    static void register_sigurg() {
        signal(SIGURG, do_nothing_handler);
    }

    void thread::kill() {
        if (!_impl) {
            return;
        }
        pthread_once(&register_sigurg_once, register_sigurg);
        kill(_impl->thread_id);
    }

    void thread::kill(pthread_t th) {
        pthread_once(&register_sigurg_once, register_sigurg);
        ::pthread_kill(th, SIGURG);
    }

    void thread::set_name(const std::string &name) {
#if defined(__APPLE__)
        pthread_setname_np(name.c_str());
#elif defined(__FreeBSD__)
        pthread_set_name_np(pthread_self(), name.c_str());
#elif !defined(__Fuchsia__)
        pthread_setname_np(pthread_self(), name.c_str());
#endif

    }

    bool thread::start() {
        FLARE_CHECK(_impl);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        if (_impl->stack_size < 2 * 1024 * 1024 || _impl->stack_size > 8 * 1024 * 1024) {
            _impl->stack_size = 8 * 1024 * 1024;
        }
        pthread_attr_setstacksize(&attr, _impl->stack_size);

        _impl->start_latch.count_up();
        auto ret = ::pthread_create(&_impl->thread_id, &attr, &thread::thread_func, (void*)this);
        pthread_attr_destroy(&attr);
        if (ret != 0) {
            return false;
        }
        _impl->start_latch.wait();
        return true;
    }

    void thread::initialize_impl(std::function<void()> &&f) {
        FLARE_CHECK(_impl == nullptr);
        _impl = std::make_shared<inner_data>();
        FLARE_CHECK(_impl);
        _impl->func = std::forward<std::function<void()>>(f);
        _impl->index = thread_index_pre_alloc();
    }

    int32_t thread::thread_index() {
        if (FLARE_UNLIKELY(local_thread_id == -1)) {
            if (flare::sysinfo::get_tid() != flare::sysinfo::get_main_thread_pid()) {
                local_thread_id = g_thread_id.fetch_add(1, std::memory_order_relaxed);
            } else {
                local_thread_id = 0;
            }
        }
        return local_thread_id;
    }

    int32_t thread::thread_index_pre_alloc() {
        return g_thread_id.fetch_add(1, std::memory_order_relaxed);
    }

    namespace detail {

        class thread_exit_helper {
        public:
            typedef void (*Fn)(void *);

            typedef std::pair<Fn, void *> Pair;

            ~thread_exit_helper() {
                // Call function reversely.
                while (!_fns.empty()) {
                    Pair back = _fns.back();
                    _fns.pop_back();
                    // Notice that _fns may be changed after calling Fn.
                    back.first(back.second);
                }
            }

            int add(Fn fn, void *arg) {
                try {
                    if (_fns.capacity() < 16) {
                        _fns.reserve(16);
                    }
                    _fns.emplace_back(fn, arg);
                } catch (...) {
                    errno = ENOMEM;
                    return -1;
                }
                return 0;
            }

            void remove(Fn fn, void *arg) {
                std::vector<Pair>::iterator
                        it = std::find(_fns.begin(), _fns.end(), std::make_pair(fn, arg));
                if (it != _fns.end()) {
                    std::vector<Pair>::iterator ite = it + 1;
                    for (; ite != _fns.end() && ite->first == fn && ite->second == arg;
                           ++ite) {}
                    _fns.erase(it, ite);
                }
            }

        private:
            std::vector<Pair> _fns;
        };

        static pthread_key_t thread_atexit_key;
        static pthread_once_t thread_atexit_once = PTHREAD_ONCE_INIT;

        static void delete_thread_exit_helper(void *arg) {
            delete static_cast<thread_exit_helper *>(arg);
        }

        static void helper_exit_global() {
            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (h) {
                pthread_setspecific(detail::thread_atexit_key, nullptr);
                delete h;
            }
        }

        static void make_thread_atexit_key() {
            if (pthread_key_create(&thread_atexit_key, delete_thread_exit_helper) != 0) {
                fprintf(stderr, "Fail to create thread_atexit_key, abort\n");
                abort();
            }
            // If caller is not pthread, delete_thread_exit_helper will not be called.
            // We have to rely on atexit().
            atexit(helper_exit_global);
        }

        detail::thread_exit_helper *get_or_new_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);

            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (nullptr == h) {
                h = new(std::nothrow)
                detail::thread_exit_helper;
                if (nullptr != h) {
                    pthread_setspecific(detail::thread_atexit_key, h);
                }
            }
            return h;
        }

        detail::thread_exit_helper *get_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);
            return (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
        }

        static void call_single_arg_fn(void *fn) {
            ((void (*)()) fn)();
        }

    }  // namespace detail


    bool thread::run_in_thread() const {
        if (_impl) {
            return pthread_self() == _impl->thread_id;
        }
        return false;
    }

    int thread::atexit(void (*fn)()) {
        if (nullptr == fn) {
            errno = EINVAL;
            return -1;
        }
        return atexit(detail::call_single_arg_fn, (void *) fn);
    }


    int thread::atexit(void (*fn)(void *), void *arg) {
        if (nullptr == fn) {
            errno = EINVAL;
            return -1;
        }
        detail::thread_exit_helper *h = detail::get_or_new_thread_exit_helper();
        if (h) {
            return h->add(fn, arg);
        }
        errno = ENOMEM;
        return -1;
    }

    void thread::atexit_cancel(void (*fn)()) {
        if (nullptr != fn) {
            atexit_cancel(detail::call_single_arg_fn, (void *) fn);
        }
    }

    void thread::atexit_cancel(void (*fn)(void *), void *arg) {
        if (fn != nullptr) {
            detail::thread_exit_helper *h = detail::get_thread_exit_helper();
            if (h) {
                h->remove(fn, arg);
            }
        }
    }

    thread::native_handler_type thread::native_handler() {
        if (local_impl) {
            return local_impl->thread_id;
        }
        // in main thread or create not by flare thread
        pthread_t mid = pthread_self();
        return mid;
    }
    // for main thread will index '0'
    FLARE_BOOTSTRAP(0, [] { thread::thread_index(); });

}  // namespace flare
