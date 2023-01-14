
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_MEMORY_REF_PTR_H_
#define TURBO_MEMORY_REF_PTR_H_

#include <atomic>
#include <utility>
#include <memory>
#include "turbo/log/logging.h"

namespace turbo {

    constexpr struct ref_ptr_t {
        explicit ref_ptr_t() = default;
    } ref_ptr_v;

    constexpr struct adopt_ptr_t {
        explicit adopt_ptr_t() = default;
    } adopt_ptr_v;

    // You need to specialize this traits unless your type is inherited from
    // `ref_counted<T>`.
    template<class T, class = void>
    struct ref_traits {
        // Increment reference counter on `ptr` with `std::memory_order_relaxed`.
        static void reference(T *ptr) noexcept {
            static_assert(
                    sizeof(T) == 0,
                    "To use `ref_ptr<T>`, you need either inherit from `ref_counted<T>` "
                    "(which you didn't), or specialize `ref_traits<T>`.");
        }

        // Decrement reference counter on `ptr` with `std::memory_order_acq_rel`.
        //
        // If the counter after decrement reaches zero, resource allocated for `ptr`
        // should be released.
        static void dereference(T *ptr) noexcept {
            static_assert(
                    sizeof(T) == 0,
                    "To use `ref_ptr<T>`, you need either inherit from `ref_counted<T>` "
                    "(which you didn't), or specialize `ref_traits<T>`.");
        }
    };

    // For types inherited from `ref_counted`, they're supported by `ref_ptr` (or
    // `ref_traits<T>`, to be accurate) by default.
    //
    // Default constructed one has a reference count of one (every object whose
    // reference count reaches zero should have already been destroyed). Should you
    // want to construct a `ref_ptr`, use overload that accepts `adopt_ptr_v`.
    //
    // IMPLEMENTATION DETAIL: For the sake of implementation simplicity, at the
    // moment `Deleter` is declared as a friend to `ref_counted<T>`. This declaration
    // is subject to change. UNLESS YOU'RE A DEVELOPER OF TURBO, YOU MUST NOT RELY
    // ON THIS.
    template<class T, class Deleter = std::default_delete<T>>
    class ref_counted {
    public:
        // Increment ref-count.
        constexpr void add_ref() noexcept;

        // Decrement ref-count, if it reaches zero after the decrement, the pointer is
        // freed by `Deleter`.
        constexpr void sub_ref() noexcept;

        // Get current ref-count.
        //
        // It's unsafe, as by the time the ref-count is returned, it may well have
        // changed. The only return value that you can rely on is 1, which means no
        // one else is referencing this object.
        constexpr std::uint32_t unsafe_ref_count() const noexcept;

    protected:
        // `ref_counted` must be inherited.
        ref_counted() = default;

        // Destructor is NOT defined as virtual on purpose, we always cast `this` to
        // `T*` before calling `delete`.

    private:
        friend Deleter;

        // Hopefully `std::uint32_t` is large enough to store a ref count.
        //
        // We tried using `std::uint_fast32_t` here, it uses 8 bytes. I'm not aware of
        // the benefit of using an 8-byte integer here (at least for the purpose of
        // reference-couting.)
        std::atomic<std::uint32_t> ref_count_{1};
    };

    // Keep the overhead the same as an atomic `std::uint3232_t`.
    static_assert(sizeof(ref_counted<int>) == sizeof(std::atomic<std::uint32_t>));

    // Utilities for internal use.
    //
    // FIXME: I'm not sure if it compiles quickly enough, but it might worth a look.
    namespace memory_internal {

        // This methods serves multiple purposes:
        //
        // - If `T` is explicitly specified, it tests if `T` inherits from
        //   `ref_counted<T, ...>`, and returns `ref_counted<T, ...>`;
        //
        // - If `T` is not specified, it tests if `T` inherits from some `ref_counted<U,
        //   ...>`, and returns `ref_counted<U, ...>`;
        //
        // - Otherwise `int` is returned, which can never inherits from `ref_counted<int,
        //   ...>`.
        template<class T, class... Us>
        ref_counted<T, Us...> get_ref_counted_type(const ref_counted<T, Us...> *);

        template<class T>
        int get_ref_counted_type(...);

        int get_ref_counted_type(...);  // Match failure.

        // Extract `T` from `ref_counted<T, ...>`.
        //
        // We're returning `std::common_type<T>` instead of `T` so as not to require `T`
        // to be complete.
        template<class T, class... Us>
        std::common_type<T> get_refee_type(const ref_counted<T, Us...> *);

        std::common_type<int> get_refee_type(...);

        template<class T>
        using refee_t =
        typename decltype(get_refee_type(reinterpret_cast<const T *>(0)))::type;

        // If `T` inherits from `ref_counted<T, ...>` directly, `T` is returned,
        // otherwise `int` (which can never inherits `ref_counted<int, ...>`) is
        // returned.
        template<class T>
        using direct_refee_t =
        refee_t<decltype(get_ref_counted_type<T>(reinterpret_cast<const T *>(0)))>;

        // Test if `T` is a subclass of some `ref_counted<T, ...>`.
        template<class T>
        constexpr auto is_ref_counted_directly_v =
                !std::is_same_v<int, direct_refee_t<T>>;

        // If `T` is inheriting from some `ref_counted<U, ...>` unambiguously, this
        // method returns `U`. Otherwise it returns `int`.
        template<class T>
        using indirect_refee_t =
        refee_t<decltype(get_ref_counted_type(reinterpret_cast<const T *>(0)))>;

        // Test if:
        //
        // - `T` is inherited from `ref_counted<U, ...>` indirectly;  (1)
        // - `U` is inherited from `ref_counted<U, ...>` directly;    (2)
        // - and `U` has a virtual destructor.                       (3)
        //
        // In this case, we can safely use `ref_traits<ref_counted<U, ...>>` on
        // `ref_counted<T>`.
        template<class T>
        constexpr auto is_ref_counted_indirectly_safe_v =
                !is_ref_counted_directly_v<T> &&                     // 1
                is_ref_counted_directly_v<indirect_refee_t<T>> &&    // 2
                std::has_virtual_destructor_v<indirect_refee_t<T>>;  // 3

        // Test if `ref_traits<as_ref_counted_t<T>>` is safe for `T`.
        //
        // We consider default traits as safe if either:
        //
        // - `T` is directly inheriting `ref_counted<T, ...>` (so its destructor does not
        //   have to be `virtual`), or
        // - `T` inherts from `ref_counted<U, ...>` and `U`'s destructor is virtual.
        template<class T>
        constexpr auto is_default_ref_traits_safe_v =
                memory_internal::is_ref_counted_directly_v<T> ||
                memory_internal::is_ref_counted_indirectly_safe_v<T>;

        template<class T>
        using as_ref_counted_t =
        decltype(get_ref_counted_type(reinterpret_cast<const T *>(0)));

    }  // namespace memory_internal

    // Specialization for `ref_counted<T>` ...
    template<class T, class Deleter>
    struct ref_traits<ref_counted<T, Deleter>> {
        static void reference(ref_counted<T, Deleter> *ptr) noexcept {
            //TURBO_DCHECK_GT(ptr->unsafe_ref_count(), 0);
            ptr->add_ref();
        }

        static void dereference(ref_counted<T, Deleter> *ptr) noexcept {
            //TURBO_DCHECK_GT(ptr->unsafe_ref_count(), 0);
            ptr->sub_ref();
        }
    };

    // ... and its subclasses.
    template<class T>
    struct ref_traits<T, std::enable_if_t<!std::is_same_v<memory_internal::as_ref_counted_t < T>, T> &&
                    memory_internal::is_default_ref_traits_safe_v < T>>>
        : ref_traits<memory_internal::as_ref_counted_t < T>> {
    };

    // @sa: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0468r1.html
    template<class T>
    class ref_ptr final {
        using Traits = ref_traits<T>;

    public:
        // Default constructed one does not own a pointer.
        constexpr ref_ptr() noexcept: ptr_(nullptr) {}

        // Same as default-constructed one.
        /* implicit */ constexpr ref_ptr(std::nullptr_t) noexcept: ptr_(nullptr) {}

        // Reference counter is decrement on destruction.
        ~ref_ptr() {
            if (ptr_) {
                Traits::dereference(ptr_);
            }
        }

        // Increment reference counter on `ptr` (if it's not `nullptr`) and hold it.
        //
        // P0468 declares `xxx_t` as the second argument, but I'd still prefer to
        // place it as the first one. `std::scoped_lock` revealed the shortcoming of
        // placing `std::adopt_lock_t` as the last parameter, I won't repeat that
        // error here.
        constexpr ref_ptr(ref_ptr_t, T *ptr) noexcept;

        // Hold `ptr` without increasing its reference counter.
        constexpr ref_ptr(adopt_ptr_t, T *ptr) noexcept: ptr_(ptr) {}

        // TBH this is a dangerous conversion constructor. Even if `T` does not have
        // virtual destructor.
        //
        // However, testing if `T` has a virtual destructor comes with a price: we'll
        // require `T` to be complete when defining `ref_ptr<T>`, which is rather
        // annoying.
        //
        // Given that `std::unique_ptr` does not test destructor's virtualization
        // state either, we ignore it for now.
        template<class U, class = std::enable_if_t<std::is_convertible_v<U *, T *>>>
        /* implicit */ constexpr ref_ptr(ref_ptr<U> ptr) noexcept : ptr_(ptr.leak()) {}

        // Copyable, movable.
        constexpr ref_ptr(const ref_ptr &ptr) noexcept;

        constexpr ref_ptr(ref_ptr &&ptr) noexcept;

        constexpr ref_ptr &operator=(const ref_ptr &ptr) noexcept;

        constexpr ref_ptr &operator=(ref_ptr &&ptr) noexcept;

        // `boost::intrusive_ptr(T*)` increments reference counter, while
        // `std::retained_ptr(T*)` (as specified by the proposal) does not. The
        // inconsistency between them can be confusing. To be perfect clear, we do not
        // support `ref_ptr(T*)`.
        //
        // ref_ptr(T*) = delete;

        // Accessors.
        constexpr T *operator->() const noexcept { return get(); }

        constexpr T &operator*() const noexcept { return *get(); }

        constexpr T *get() const noexcept { return ptr_; }

        // Test if *this holds a pointer.
        constexpr explicit operator bool() const noexcept { return ptr_; }

        // Equivalent to `reset()`.
        constexpr ref_ptr &operator=(std::nullptr_t) noexcept;

        // reset *this to an empty one.
        constexpr void reset() noexcept;

        // Release whatever *this currently holds and hold `ptr` instead.
        constexpr void reset(ref_ptr_t, T *ptr) noexcept;

        constexpr void reset(adopt_ptr_t, T *ptr) noexcept;

        // Gives up ownership on its internal pointer, which is returned.
        //
        // The caller is responsible for calling `ref_traits<T>::dereference` when it
        // sees fit.
        [[nodiscard]] constexpr T *leak() noexcept {
            return std::exchange(ptr_, nullptr);
        }

    private:
        T *ptr_;
    };

    // Shorthand for `new`-ing an object and **adopt** (NOT *ref*, i.e., the
    // initial ref-count must be 1) it into a `ref_ptr`.
    template<class T, class... Us>
    ref_ptr<T> make_ref_counted(Us &&... args) {
        return ref_ptr(adopt_ptr_v, new T(std::forward<Us>(args)...));
    }

    template<class T, class Deleter>
    constexpr void ref_counted<T, Deleter>::add_ref() noexcept {
        ref_count_.fetch_add(1, std::memory_order_relaxed);
        //TURBO_CHECK_GT(was, 0u);
    }

    template<class T, class Deleter>
    constexpr void ref_counted<T, Deleter>::sub_ref() noexcept {
        // It seems that we can simply test if `ref_count_` is 1, and save an atomic
        // operation if it is (as we're the only reference holder). However I don't
        // see a perf. boost in implementing it, so I keep it unchanged. we might want
        // to take a deeper look later.
        if (auto was = ref_count_.fetch_sub(1, std::memory_order_acq_rel); was == 1) {
            Deleter()(static_cast<T *>(this));  // Hmmm.
        } else {
            //TURBO_CHECK_GT(was, 1);
        }
    }

    template<class T, class Deleter>
    constexpr std::uint32_t ref_counted<T, Deleter>::unsafe_ref_count()
    const noexcept {
        return ref_count_.load(std::memory_order_relaxed);  // FIXME: `acquire`?
    }

    template<class T>
    constexpr ref_ptr<T>::ref_ptr(ref_ptr_t, T *ptr) noexcept : ptr_(ptr) {
        if (ptr) {
            Traits::reference(ptr_);
        }
    }

    template<class T>
    constexpr ref_ptr<T>::ref_ptr(const ref_ptr &ptr) noexcept : ptr_(ptr.ptr_) {
        if (ptr_) {
            Traits::reference(ptr_);
        }
    }

    template<class T>
    constexpr ref_ptr<T>::ref_ptr(ref_ptr &&ptr) noexcept : ptr_(ptr.ptr_) {
        ptr.ptr_ = nullptr;
    }

    template<class T>
    constexpr ref_ptr<T> &ref_ptr<T>::operator=(const ref_ptr &ptr) noexcept {
        if (TURBO_UNLIKELY(&ptr == this)) {
            return *this;
        }
        if (ptr_) {
            Traits::dereference(ptr_);
        }
        ptr_ = ptr.ptr_;
        if (ptr_) {
            Traits::reference(ptr_);
        }
        return *this;
    }

    template<class T>
    constexpr ref_ptr<T> &ref_ptr<T>::operator=(ref_ptr &&ptr) noexcept {
        if (TURBO_UNLIKELY(&ptr == this)) {
            return *this;
        }
        if (ptr_) {
            Traits::dereference(ptr_);
        }
        ptr_ = ptr.ptr_;
        ptr.ptr_ = nullptr;
        return *this;
    }

    template<class T>
    constexpr ref_ptr<T> &ref_ptr<T>::operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    template<class T>
    constexpr void ref_ptr<T>::reset() noexcept {
        if (ptr_) {
            Traits::dereference(ptr_);
            ptr_ = nullptr;
        }
    }

    template<class T>
    constexpr void ref_ptr<T>::reset(ref_ptr_t, T *ptr) noexcept {
        reset();
        ptr_ = ptr;
        Traits::reference(ptr_);
    }

    template<class T>
    constexpr void ref_ptr<T>::reset(adopt_ptr_t, T *ptr) noexcept {
        reset();
        ptr_ = ptr;
    }

    // I'd rather implement `operator<=>` instead.
    template<class T>
    constexpr bool operator==(const ref_ptr<T> &left,
                              const ref_ptr<T> &right) noexcept {
        return left.get() == right.get();
    }

    template<class T>
    constexpr bool operator==(const ref_ptr<T> &ptr, std::nullptr_t) noexcept {
        return ptr.get() == nullptr;
    }

    template<class T>
    constexpr bool operator==(std::nullptr_t, const ref_ptr<T> &ptr) noexcept {
        return ptr.get() == nullptr;
    }

}  // namespace turbo

namespace std {

    // Specialization for `std::atomic<turbo::ref_ptr<T>>`.
    //
    // @sa: https://en.ccreference.com/w/cpp/memory/shared_ptr/atomic2
    template<class T>
    class atomic<turbo::ref_ptr<T>> {
        using Traits = turbo::ref_traits<T>;

    public:
        constexpr atomic() noexcept: ptr_(nullptr) {}

        // FIXME: It destructor of specialization of `std::atomic>` allowed to be
        // non-trivial? I don't see a way to implement a trival destructor while
        // maintaining correctness.
        ~atomic() {
            if (auto ptr = ptr_.load(std::memory_order_acquire)) {
                Traits::dereference(ptr);
            }
        }

        // Receiving a pointer.
        constexpr /* implicit */ atomic(turbo::ref_ptr<T> ptr) noexcept
                : ptr_(ptr.leak()) {}

        atomic &operator=(turbo::ref_ptr<T> ptr) noexcept {
            store(std::move(ptr));
            return *this;
        }

        // Tests if the implementation is lock-free.
        bool is_lock_free() const noexcept { return ptr_.is_lock_free(); }

        // Stores to this atomic ref-ptr.
        void store(turbo::ref_ptr<T> ptr,
                   std::memory_order order = std::memory_order_seq_cst) noexcept {
            // Promoted to `exchange`, otherwise we can't atomically load current
            // pointer (to release it) and store a new one.
            exchange(std::move(ptr), order);
        }

        // Loads from this atomic ref-ptr.
        turbo::ref_ptr<T> load(
                std::memory_order order = std::memory_order_seq_cst) const noexcept {
            return turbo::ref_ptr<T>(turbo::ref_ptr_v, ptr_.load(order));
        }

        // Same as `load()`.
        operator turbo::ref_ptr<T>() const noexcept { return load(); }

        // Exchanges with a (possibly) different ref-ptr.
        turbo::ref_ptr<T> exchange(
                turbo::ref_ptr<T> ptr,
                std::memory_order order = std::memory_order_seq_cst) noexcept {
            return turbo::ref_ptr(turbo::adopt_ptr_v, ptr_.exchange(ptr.leak(), order));
        }

        // Compares if this atomic holds the `expected` pointer, and exchanges it with
        // the new `desired` one if the comparsion holds.
        bool compare_exchange_strong(
                turbo::ref_ptr<T> &expected, turbo::ref_ptr<T> desired,
                std::memory_order order = std::memory_order_seq_cst) noexcept {
            return compare_exchange_impl(
                    [&](auto &&... args) { return ptr_.compare_exchange_strong(args...); },
                    expected, std::move(desired), order);
        }

        bool compare_exchange_weak(
                turbo::ref_ptr<T> &expected, turbo::ref_ptr<T> desired,
                std::memory_order order = std::memory_order_seq_cst) noexcept {
            return compare_exchange_impl(
                    [&](auto &&... args) { return ptr_.compare_exchange_weak(args...); },
                    expected, std::move(desired), order);
        }

        bool compare_exchange_strong(turbo::ref_ptr<T> &expected,
                                     turbo::ref_ptr<T> desired,
                                     std::memory_order success,
                                     std::memory_order failure) noexcept {
            return compare_exchange_impl(
                    [&](auto &&... args) { return ptr_.compare_exchange_strong(args...); },
                    expected, std::move(desired), success, failure);
        }

        bool compare_exchange_weak(turbo::ref_ptr<T> &expected,
                                   turbo::ref_ptr<T> desired,
                                   std::memory_order success,
                                   std::memory_order failure) noexcept {
            return compare_exchange_impl(
                    [&](auto &&... args) { return ptr_.compare_exchange_weak(args...); },
                    expected, std::move(desired), success, failure);
        }

#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L

        // Wait on this atomic.
  void wait(
      turbo::ref_ptr<T> old,
      std::memory_order order = std::memory_order_seq_cst) const noexcept {
    return ptr_.wait(old.get(), order);
  }

  // Notifies one `wait`.
  void notify_one() noexcept { ptr_.notify_one(); }

  void notify_all() noexcept { ptr_.notify_all(); }

#endif

        // Not copyable, as requested by the Standard.
        atomic(const atomic &) = delete;

        atomic &operator=(const atomic &) = delete;

    private:
        template<class F, class... Orders>
        bool compare_exchange_impl(F &&f, turbo::ref_ptr<T> &expected,
                                   turbo::ref_ptr<T> desired, Orders... orders) {
            auto current = expected.get();
            if (std::forward<F>(f)(current, desired.get(), orders...)) {
                (void) desired.leak();  // Ownership transfer to `ptr_`.
                // Ownership of the old pointer is transferred to us, release it.
                turbo::ref_ptr(turbo::adopt_ptr_v, current);
                return true;
            }
            expected = load();  // FIXME: Promoted to `seq_cst` unnecessarily.
            return false;
        }

    private:
        std::atomic<T *> ptr_;  // We hold a reference to it.
    };

}  // namespace std

#endif  // TURBO_MEMORY_REF_PTR_H_
