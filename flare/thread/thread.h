
#ifndef FLARE_THREAD_THREAD_H_
#define FLARE_THREAD_THREAD_H_

#include <functional>
#include <vector>
#include <string>
#include <pthread.h>
#include <memory>
#include <functional>
#include "flare/thread/affinity.h"
#include "flare/thread/latch.h"
#include "flare/memory/ref_ptr.h"
#include "flare/base/type_traits.h"
#include "flare/strings/str_format.h"

namespace flare {

    class thread {
    public:
        typedef pthread_t native_handler_type;

        thread() = default;

        thread(const thread &) = default;

        thread &operator=(const thread &) = default;

        thread(thread &&) = default;


        thread &operator=(thread &&) = default;

        // Start a new thread using the given affinity that calls func.
        template<typename F, typename = typename std::enable_if<std::is_same<thread,
                typename flare::remove_cvref<F>::type>::value == false>>
        explicit thread(F &&f) {
            initialize(std::forward<F>(f));
        }


        template<class F, class... Args,
                typename = typename std::enable_if<std::is_same<thread,
                        typename flare::remove_cvref<F>::type>::value == false>>
        explicit thread(F &&f, Args &&... args) {
            initialize(std::forward<F>(f), std::forward<Args>(args)...);
        }

        template<typename F,
                typename = typename std::enable_if<
                        std::is_same<thread, typename flare::remove_cvref<F>::type>::value == false>>
        void initialize(F &&f) {
            initialize_impl([f = std::forward<F>(f)] {
                f();
            });
        }

        template<class F, class... Args,
                typename = typename std::enable_if<
                        std::is_same<thread, typename flare::remove_cvref<F>::type>::value == false>>
        void initialize(F &&f, Args &&... args) {
            auto proc = [f = std::forward<F>(f),
                    args = std::make_tuple(std::forward<Args>(args)...)] {
                std::apply(f, std::move(args));
            };
            initialize_impl(proc);
        }

        void set_stack_size(size_t size) {
            FLARE_CHECK(_impl);
            _impl->stack_size = size;
        }

        void set_affinity(uint32_t n) {
            FLARE_CHECK(_impl);
            _impl->affinity = n;
        }

        void set_affinity_group(uint32_t n) {
            FLARE_CHECK(_impl);
            _impl->group = n;
        }

        void set_prefix(const std::string &prefix) {
            FLARE_CHECK(_impl);
            _impl->prefix = prefix;
        }

        ~thread();

        bool start();

        // join() blocks until the thread completes.
        void join(void **ptr = nullptr);

        void detach();

        void kill();

        std::string name() const;

        static std::string current_name();

        bool run_in_thread() const;

        // set_name() sets the name of the currently executing thread for displaying
        // in a debugger.
        template<class... Args>
        static void set_name(const std::string_view &fmt, Args &&... args) {
            std::string name = flare::string_format(fmt, std::forward<Args>(args)...);
            set_name(name);
        }

        static void set_name(const std::string &name);

        static void kill(pthread_t th);

        static int32_t thread_index();

        static int atexit(void (*fn)(void *), void *arg);

        static int atexit(void (*fn)());

        static void atexit_cancel(void (*fn)());

        static void atexit_cancel(void (*fn)(void *), void *arg);

        static native_handler_type native_handler();
    public:
        struct inner_data {
            size_t stack_size{8 * 1024 * 1024};
            std::function<void()> func;
            int32_t affinity{-1};
            int32_t group{-1};
            int32_t index{-1};
            std::string prefix;
            std::string name;
            pthread_t thread_id;
            latch start_latch;
            std::atomic<bool> running{false};
            std::atomic<bool> detached{false};
        };

    private:


        void set_affinity();

        static void *thread_func(void *arg);

        void initialize_impl(std::function<void()> &&f);

        static int32_t thread_index_pre_alloc();

        std::shared_ptr<inner_data> _impl = nullptr;
    };


}  // namespace flare

#endif  // FLARE_THREAD_THREAD_H_
