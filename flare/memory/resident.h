//
// Created by jeff.li.
//

#ifndef FLARE_MEMORY_RESIDENT_H_
#define FLARE_MEMORY_RESIDENT_H_


#include <new>  // For placement new. @sa: https://stackoverflow.com/q/4010281
#include <type_traits>
#include <utility>

namespace flare {

    namespace detail {

        template<class T>
        class resident_impl {
        public:
            // Noncopyable / nonmovable.
            resident_impl(const resident_impl &) = delete;

            resident_impl &operator=(const resident_impl &) = delete;

            // Accessors.
            T *get() noexcept { return reinterpret_cast<T *>(&storage_); }

            const T *get() const noexcept {
                return reinterpret_cast<const T *>(&storage_);
            }

            T *operator->() noexcept { return get(); }

            const T *operator->() const noexcept { return get(); }

            T &operator*() noexcept { return *get(); }

            const T &operator*() const noexcept { return *get(); }

        protected:
            resident_impl() = default;

        protected:
            std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
        };

    }  // namespace detail

    // `resident<T>` helps you create objects that are never destroyed
    // (without incuring heap memory allocation.).
    //
    // In certain cases (e.g., singleton), resident object can save you from
    // dealing with destruction order issues.
    //
    // Caveats:
    //
    // - Be caution when declaring `resident<T>` as `thread_local`, this may
    //   cause memory leak.
    //
    // - To construct `resident<T>`, you might have to declare this class as
    //   your friend (if the constructor being called is not publicly accessible).
    //
    // - By declaring `resident<T>` as your friend, it's impossible to
    //   guarantee `T` is used as a singleton as now anybody can construct a new
    //   `resident<T>`. You can use `resident_singleton<T>` in this case.
    //
    // e.g.:
    //
    // void CreateWorld() {
    //   static resident<std::mutex> lock;  // Destructor won't be called.
    //   std::scoped_lock _(*lock);
    //
    //   // ...
    // }
    template<class T>
    class resident final : private detail::resident_impl<T> {
        using Impl = detail::resident_impl<T>;

    public:
        template<class... Ts>
        explicit resident(Ts &&... args) {
            new(&this->storage_) T(std::forward<Ts>(args)...);
        }

        using Impl::get;
        using Impl::operator->;
        using Impl::operator*;
    };

    // Same as `resident`, except that it's constructor is only accessible to
    // `T`. This class is useful when `T` is intended to be used as singleton.
    template<class T>
    class resident_singleton final
            : private detail::resident_impl<T> {
        using Impl = detail::resident_impl<T>;

    public:
        using Impl::get;
        using Impl::operator->;
        using Impl::operator*;

    private:
        friend T;

        template<class... Ts>
        explicit resident_singleton(Ts &&... args) {
            new(&this->storage_) T(std::forward<Ts>(args)...);
        }
    };

}  // namespace flare

#endif  // FLARE_MEMORY_RESIDENT_H_
