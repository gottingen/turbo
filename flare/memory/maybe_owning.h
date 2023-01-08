
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_MEMORY_MAYBE_OWNING_H_
#define FLARE_MEMORY_MAYBE_OWNING_H_

#include <memory>
#include <utility>
#include "flare/log/logging.h"

namespace flare {

    inline constexpr struct owning_t {
        explicit owning_t() = default;
    } owning;

    inline constexpr struct non_owning_t {
        explicit non_owning_t() = default;
    } non_owning;

    // You don't know if you own `T`, I don't know either.
    //
    // Seriously, this class allows you to have a unified way for handling pointers
    // that you own and pointers that you don't own, so you don't have to defining
    // pairs of methods such as `AddXxx()` and `AddAllocatedXxx()`.
    template<class T>
    class maybe_owning {
    public:
        // Test if `maybe_owning<T>` should accept `U*` in conversion.
        template<class U>
        inline static constexpr bool is_convertible_from_v =
                std::is_convertible_v<U *, T *> &&
                (std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>> ||
                 std::has_virtual_destructor_v<T>);

        // Construct an empty one.
        constexpr maybe_owning() noexcept: _owning(false), _ptr(nullptr) {}

        /* implicit */ maybe_owning(std::nullptr_t) noexcept: maybe_owning() {}

        // Maybe transferring ownership.
        //
        // The first two overloads are preferred since they're more evident about
        // what's going on.
        constexpr maybe_owning(owning_t, T *ptr) noexcept: maybe_owning(ptr, true) {}

        constexpr maybe_owning(non_owning_t, T *ptr) noexcept
                : maybe_owning(ptr, false) {}

        constexpr maybe_owning(T *ptr, bool owning) noexcept
                : _owning(owning), _ptr(ptr) {}

        // Transferring ownership.
        //
        // This overload only participate in overload resolution if `U*` is implicitly
        // convertible to `T*`.
        template<class U, class = std::enable_if_t<is_convertible_from_v<U>>>
        /* `explicit`? */ constexpr maybe_owning(std::unique_ptr<U> ptr) noexcept
                : _owning(!!ptr), _ptr(ptr.release()) {}

        // Transferring ownership.
        //
        // This overload only participate in overload resolution if `U*` is implicitly
        // convertible to `T*`.
        template<class U, class = std::enable_if_t<is_convertible_from_v<U>>>
        /* implicit */ constexpr maybe_owning(maybe_owning<U> &&ptr) noexcept
                : _owning(ptr._owning), _ptr(ptr._ptr) {
            ptr._owning = false;
            ptr._ptr = nullptr;
        }

        // Movable.
        constexpr maybe_owning(maybe_owning &&other) noexcept
                : _owning(other._owning), _ptr(other._ptr) {
            other._owning = false;
            other._ptr = nullptr;
        }

        maybe_owning &operator=(maybe_owning &&other) noexcept {
            if (&other != this) {
                reset();
                std::swap(_owning, other._owning);
                std::swap(_ptr, other._ptr);
            }
            return *this;
        }

        // Not copyable.
        maybe_owning(const maybe_owning &) = delete;

        maybe_owning &operator=(const maybe_owning &) = delete;

        // The pointer is freed (if we own it) on destruction.
        ~maybe_owning() {
            if (_owning) {
                delete _ptr;
            }
        }

        // Accessor.
        constexpr T *get() const noexcept { return _ptr; }

        constexpr T *operator->() const noexcept { return get(); }

        constexpr T &operator*() const noexcept { return *get(); }

        // Test if we're holding a valid pointer.
        constexpr explicit operator bool() const noexcept { return _ptr; }

        // Test if we own the pointer.
        constexpr bool owning() const noexcept { return _owning; }

        // Reset to empty pointer.
        constexpr maybe_owning &operator=(std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        // Release the pointer (without freeing it) to the caller.
        //
        // If we're not owning the pointer, calling this method cause undefined
        // behavior. (Use `Get()` instead.)
        [[nodiscard]] constexpr T *leak() noexcept {
            FLARE_CHECK(_owning) <<
                           "Calling `Leak()` on non-owning `maybe_owning<T>` is undefined.";
            _owning = false;
            return std::exchange(_ptr, nullptr);
        }

        // Release what we currently hold and hold a new pointer.
        constexpr void reset(owning_t, T *ptr) noexcept { reset(ptr, true); }

        constexpr void reset(non_owning_t, T *ptr) noexcept { reset(ptr, false); }

        // I'm not sure if we want to provide an `operator std::unique_ptr<T>() &&`.
        // This is dangerous as we could not check for `owning()` at compile time.

    private:

        // Reset the pointer we have.
        //
        // These two were public interfaces, but I think we'd better keep them private
        // as `x.Reset(..., false)` is not as obvious as `x =
        // maybe_owning(non_owning, ...)`.
        constexpr void reset() noexcept { return reset(nullptr, false); }

        constexpr void reset(T *ptr, bool owning) noexcept {
            if (_owning) {
                FLARE_CHECK(_ptr);
                delete _ptr;
            }
            FLARE_CHECK(!_owning || _ptr) <<
                                    "Passing a `nullptr` to `ptr` while specifying `owning` as `true` does "
                                    "not make much sense, I think.";
            _ptr = ptr;
            _owning = owning;
        }

    private:

        template<class U>
        friend
        class maybe_owning;

        bool _owning;
        T *_ptr;
    };

    template<class T>
    class maybe_owning<T[]>;  // Not implemented (yet).

    // When used as function argument, it can be annoying to explicitly constructing
    // `maybe_owning<T>` at every call-site. If it deemed acceptable to always treat
    // raw pointer as non-owning (which, in most cases, you should), you can use
    // this class as argument type instead.
    template<class T>
    class maybe_owning_argument {
    public:
        template<class U, class = decltype(maybe_owning<T>(std::declval<U &&>()))>
        /* implicit */ maybe_owning_argument(U &&ptr) : _ptr(std::forward<U>(ptr)) {}

        template<class U, class = std::enable_if_t<
                maybe_owning<T>::template is_convertible_from_v<U>>>
        /* implicit */ maybe_owning_argument(U *ptr) : _ptr(non_owning, ptr) {}

        operator maybe_owning<T>() && noexcept { return std::move(_ptr); }

    private:
        maybe_owning<T> _ptr;
    };

}
#endif // FLARE_MEMORY_MAYBE_OWNING_H_
