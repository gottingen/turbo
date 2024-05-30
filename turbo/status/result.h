// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// -----------------------------------------------------------------------------
// File: statusor.h
// -----------------------------------------------------------------------------
//
// An `turbo::Result<T>` represents a union of an `turbo::Status` object
// and an object of type `T`. The `turbo::Result<T>` will either contain an
// object of type `T` (indicating a successful operation), or an error (of type
// `turbo::Status`) explaining why such a value is not present.
//
// In general, check the success of an operation returning an
// `turbo::Result<T>` like you would an `turbo::Status` by using the `ok()`
// member function.
//
// Example:
//
//   Result<Foo> result = Calculation();
//   if (result.ok()) {
//     result->DoSomethingCool();
//   } else {
//     LOG(ERROR) << result.status();
//   }
#pragma once

#include <exception>
#include <initializer_list>
#include <new>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <turbo/base/attributes.h>
#include <turbo/base/nullability.h>
#include <turbo/base/call_once.h>
#include <turbo/meta/type_traits.h>
#include <turbo/status/internal/statusor_internal.h>
#include <turbo/status/status.h>
#include <turbo/strings/has_stringify.h>
#include <turbo/strings/has_ostream_operator.h>
#include <turbo/strings/str_format.h>
#include <turbo/types/variant.h>
#include <turbo/utility/utility.h>

namespace turbo {

    // BadResultAccess
    //
    // This class defines the type of object to throw (if exceptions are enabled),
    // when accessing the value of an `turbo::Result<T>` object that does not
    // contain a value. This behavior is analogous to that of
    // `std::bad_optional_access` in the case of accessing an invalid
    // `std::optional` value.
    //
    // Example:
    //
    // try {
    //   turbo::Result<int> v = FetchInt();
    //   DoWork(v.value());  // Accessing value() when not "OK" may throw
    // } catch (turbo::BadResultAccess& ex) {
    //   LOG(ERROR) << ex.status();
    // }
    class BadResultAccess : public std::exception {
    public:
        explicit BadResultAccess(turbo::Status status);

        ~BadResultAccess() override = default;

        BadResultAccess(const BadResultAccess &other);

        BadResultAccess &operator=(const BadResultAccess &other);

        BadResultAccess(BadResultAccess &&other);

        BadResultAccess &operator=(BadResultAccess &&other);

        // BadResultAccess::what()
        //
        // Returns the associated explanatory string of the `turbo::Result<T>`
        // object's error code. This function contains information about the failing
        // status, but its exact formatting may change and should not be depended on.
        //
        // The pointer of this string is guaranteed to be valid until any non-const
        // function is invoked on the exception object.
        turbo::Nonnull<const char *> what() const noexcept override;

        // BadResultAccess::status()
        //
        // Returns the associated `turbo::Status` of the `turbo::Result<T>` object's
        // error.
        const turbo::Status &status() const;

    private:
        void InitWhat() const;

        turbo::Status status_;
        mutable turbo::once_flag init_what_;
        mutable std::string what_;
    };

// Returned Result objects may not be ignored.
    template<typename T>
#if TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)
    // TODO(b/176172494): TURBO_MUST_USE_RESULT should expand to the more strict
    // [[nodiscard]]. For now, just use [[nodiscard]] directly when it is available.
    class [[nodiscard]] Result;
#else
    class TURBO_MUST_USE_RESULT Result;

#endif  // TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)

    // turbo::Result<T>
    //
    // The `turbo::Result<T>` class template is a union of an `turbo::Status` object
    // and an object of type `T`. The `turbo::Result<T>` models an object that is
    // either a usable object, or an error (of type `turbo::Status`) explaining why
    // such an object is not present. An `turbo::Result<T>` is typically the return
    // value of a function which may fail.
    //
    // An `turbo::Result<T>` can never hold an "OK" status (an
    // `turbo::StatusCode::kOk` value); instead, the presence of an object of type
    // `T` indicates success. Instead of checking for a `kOk` value, use the
    // `turbo::Result<T>::ok()` member function. (It is for this reason, and code
    // readability, that using the `ok()` function is preferred for `turbo::Status`
    // as well.)
    //
    // Example:
    //
    //   Result<Foo> result = DoBigCalculationThatCouldFail();
    //   if (result.ok()) {
    //     result->DoSomethingCool();
    //   } else {
    //     LOG(ERROR) << result.status();
    //   }
    //
    // Accessing the object held by an `turbo::Result<T>` should be performed via
    // `operator*` or `operator->`, after a call to `ok()` confirms that the
    // `turbo::Result<T>` holds an object of type `T`:
    //
    // Example:
    //
    //   turbo::Result<int> i = GetCount();
    //   if (i.ok()) {
    //     updated_total += *i;
    //   }
    //
    // NOTE: using `turbo::Result<T>::value()` when no valid value is present will
    // throw an exception if exceptions are enabled or terminate the process when
    // exceptions are not enabled.
    //
    // Example:
    //
    //   Result<Foo> result = DoBigCalculationThatCouldFail();
    //   const Foo& foo = result.value();    // Crash/exception if no value present
    //   foo.DoSomethingCool();
    //
    // A `turbo::Result<T*>` can be constructed from a null pointer like any other
    // pointer value, and the result will be that `ok()` returns `true` and
    // `value()` returns `nullptr`. Checking the value of pointer in an
    // `turbo::Result<T*>` generally requires a bit more care, to ensure both that
    // a value is present and that value is not null:
    //
    //  Result<std::unique_ptr<Foo>> result = FooFactory::MakeNewFoo(arg);
    //  if (!result.ok()) {
    //    LOG(ERROR) << result.status();
    //  } else if (*result == nullptr) {
    //    LOG(ERROR) << "Unexpected null pointer";
    //  } else {
    //    (*result)->DoSomethingCool();
    //  }
    //
    // Example factory implementation returning Result<T>:
    //
    //  Result<Foo> FooFactory::MakeFoo(int arg) {
    //    if (arg <= 0) {
    //      return turbo::Status(turbo::StatusCode::kInvalidArgument,
    //                          "Arg must be positive");
    //    }
    //    return Foo(arg);
    //  }
    template<typename T>
    class Result : private internal_statusor::ResultData<T>,
                     private internal_statusor::CopyCtorBase<T>,
                     private internal_statusor::MoveCtorBase<T>,
                     private internal_statusor::CopyAssignBase<T>,
                     private internal_statusor::MoveAssignBase<T> {
        template<typename U>
        friend
        class Result;

        typedef internal_statusor::ResultData<T> Base;

    public:
        // Result<T>::value_type
        //
        // This instance data provides a generic `value_type` member for use within
        // generic programming. This usage is analogous to that of
        // `optional::value_type` in the case of `std::optional`.
        typedef T value_type;

        // Constructors

        // Constructs a new `turbo::Result` with an `turbo::StatusCode::kUnknown`
        // status. This constructor is marked 'explicit' to prevent usages in return
        // values such as 'return {};', under the misconception that
        // `turbo::Result<std::vector<int>>` will be initialized with an empty
        // vector, instead of an `turbo::StatusCode::kUnknown` error code.
        explicit Result();

        // `Result<T>` is copy constructible if `T` is copy constructible.
        Result(const Result &) = default;

        // `Result<T>` is copy assignable if `T` is copy constructible and copy
        // assignable.
        Result &operator=(const Result &) = default;

        // `Result<T>` is move constructible if `T` is move constructible.
        Result(Result &&) = default;

        // `Result<T>` is moveAssignable if `T` is move constructible and move
        // assignable.
        Result &operator=(Result &&) = default;

        // Converting Constructors

        // Constructs a new `turbo::Result<T>` from an `turbo::Result<U>`, when `T`
        // is constructible from `U`. To avoid ambiguity, these constructors are
        // disabled if `T` is also constructible from `Result<U>.`. This constructor
        // is explicit if and only if the corresponding construction of `T` from `U`
        // is explicit. (This constructor inherits its explicitness from the
        // underlying constructor.)
        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        false, T, U, false, const U &>::value,
                int> = 0>
        Result(const Result<U> &other)  // NOLINT
                : Base(static_cast<const typename Result<U>::Base &>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        false, T, U, true, const U &>::value,
                int> = 0>
        Result(const Result<U> &other TURBO_ATTRIBUTE_LIFETIME_BOUND)  // NOLINT
                : Base(static_cast<const typename Result<U>::Base &>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        true, T, U, false, const U &>::value,
                int> = 0>
        explicit Result(const Result<U> &other)
                : Base(static_cast<const typename Result<U>::Base &>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        true, T, U, true, const U &>::value,
                int> = 0>
        explicit Result(const Result<U> &other TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : Base(static_cast<const typename Result<U>::Base &>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        false, T, U, false, U &&>::value,
                int> = 0>
        Result(Result<U> &&other)  // NOLINT
                : Base(static_cast<typename Result<U>::Base &&>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        false, T, U, true, U &&>::value,
                int> = 0>
        Result(Result<U> &&other TURBO_ATTRIBUTE_LIFETIME_BOUND)  // NOLINT
                : Base(static_cast<typename Result<U>::Base &&>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        true, T, U, false, U &&>::value,
                int> = 0>
        explicit Result(Result<U> &&other)
                : Base(static_cast<typename Result<U>::Base &&>(other)) {}

        template<typename U, turbo::enable_if_t<
                internal_statusor::IsConstructionFromStatusOrValid<
                        true, T, U, true, U &&>::value,
                int> = 0>
        explicit Result(Result<U> &&other TURBO_ATTRIBUTE_LIFETIME_BOUND)
                : Base(static_cast<typename Result<U>::Base &&>(other)) {}

        // Converting Assignment Operators

        // Creates an `turbo::Result<T>` through assignment from an
        // `turbo::Result<U>` when:
        //
        //   * Both `turbo::Result<T>` and `turbo::Result<U>` are OK by assigning
        //     `U` to `T` directly.
        //   * `turbo::Result<T>` is OK and `turbo::Result<U>` contains an error
        //      code by destroying `turbo::Result<T>`'s value and assigning from
        //      `turbo::Result<U>'
        //   * `turbo::Result<T>` contains an error code and `turbo::Result<U>` is
        //      OK by directly initializing `T` from `U`.
        //   * Both `turbo::Result<T>` and `turbo::Result<U>` contain an error
        //     code by assigning the `Status` in `turbo::Result<U>` to
        //     `turbo::Result<T>`
        //
        // These overloads only apply if `turbo::Result<T>` is constructible and
        // assignable from `turbo::Result<U>` and `Result<T>` cannot be directly
        // assigned from `Result<U>`.
        template<typename U,
                turbo::enable_if_t<internal_statusor::IsStatusOrAssignmentValid<
                        T, const U &, false>::value,
                        int> = 0>
        Result &operator=(const Result<U> &other) {
            this->Assign(other);
            return *this;
        }

        template<typename U,
                turbo::enable_if_t<internal_statusor::IsStatusOrAssignmentValid<
                        T, const U &, true>::value,
                        int> = 0>
        Result &operator=(const Result<U> &other TURBO_ATTRIBUTE_LIFETIME_BOUND) {
            this->Assign(other);
            return *this;
        }

        template<typename U,
                turbo::enable_if_t<internal_statusor::IsStatusOrAssignmentValid<
                        T, U &&, false>::value,
                        int> = 0>
        Result &operator=(Result<U> &&other) {
            this->Assign(std::move(other));
            return *this;
        }

        template<typename U,
                turbo::enable_if_t<internal_statusor::IsStatusOrAssignmentValid<
                        T, U &&, true>::value,
                        int> = 0>
        Result &operator=(Result<U> &&other TURBO_ATTRIBUTE_LIFETIME_BOUND) {
            this->Assign(std::move(other));
            return *this;
        }

        // Constructs a new `turbo::Result<T>` with a non-ok status. After calling
        // this constructor, `this->ok()` will be `false` and calls to `value()` will
        // crash, or produce an exception if exceptions are enabled.
        //
        // The constructor also takes any type `U` that is convertible to
        // `turbo::Status`. This constructor is explicit if an only if `U` is not of
        // type `turbo::Status` and the conversion from `U` to `Status` is explicit.
        //
        // REQUIRES: !Status(std::forward<U>(v)).ok(). This requirement is DCHECKed.
        // In optimized builds, passing turbo::OkStatus() here will have the effect
        // of passing turbo::StatusCode::kInternal as a fallback.
        template<typename U = turbo::Status,
                turbo::enable_if_t<internal_statusor::IsConstructionFromStatusValid<
                        false, T, U>::value,
                        int> = 0>
        Result(U &&v) : Base(std::forward<U>(v)) {}

        template<typename U = turbo::Status,
                turbo::enable_if_t<internal_statusor::IsConstructionFromStatusValid<
                        true, T, U>::value,
                        int> = 0>
        explicit Result(U &&v) : Base(std::forward<U>(v)) {}

        template<typename U = turbo::Status,
                turbo::enable_if_t<internal_statusor::IsConstructionFromStatusValid<
                        false, T, U>::value,
                        int> = 0>
        Result &operator=(U &&v) {
            this->AssignStatus(std::forward<U>(v));
            return *this;
        }

        // Perfect-forwarding value assignment operator.

        // If `*this` contains a `T` value before the call, the contained value is
        // assigned from `std::forward<U>(v)`; Otherwise, it is directly-initialized
        // from `std::forward<U>(v)`.
        // This function does not participate in overload unless:
        // 1. `std::is_constructible_v<T, U>` is true,
        // 2. `std::is_assignable_v<T&, U>` is true.
        // 3. `std::is_same_v<Result<T>, std::remove_cvref_t<U>>` is false.
        // 4. Assigning `U` to `T` is not ambiguous:
        //  If `U` is `Result<V>` and `T` is constructible and assignable from
        //  both `Result<V>` and `V`, the assignment is considered bug-prone and
        //  ambiguous thus will fail to compile. For example:
        //    Result<bool> s1 = true;  // s1.ok() && *s1 == true
        //    Result<bool> s2 = false;  // s2.ok() && *s2 == false
        //    s1 = s2;  // ambiguous, `s1 = *s2` or `s1 = bool(s2)`?
        template<typename U = T,
                typename std::enable_if<
                        internal_statusor::IsAssignmentValid<T, U, false>::value,
                        int>::type = 0>
        Result &operator=(U &&v) {
            this->Assign(std::forward<U>(v));
            return *this;
        }

        template<typename U = T,
                typename std::enable_if<
                        internal_statusor::IsAssignmentValid<T, U, true>::value,
                        int>::type = 0>
        Result &operator=(U &&v TURBO_ATTRIBUTE_LIFETIME_BOUND) {
            this->Assign(std::forward<U>(v));
            return *this;
        }

        // Constructs the inner value `T` in-place using the provided args, using the
        // `T(args...)` constructor.
        template<typename... Args>
        explicit Result(turbo::in_place_t, Args &&... args);

        template<typename U, typename... Args>
        explicit Result(turbo::in_place_t, std::initializer_list<U> ilist,
                          Args &&... args);

        // Constructs the inner value `T` in-place using the provided args, using the
        // `T(U)` (direct-initialization) constructor. This constructor is only valid
        // if `T` can be constructed from a `U`. Can accept move or copy constructors.
        //
        // This constructor is explicit if `U` is not convertible to `T`. To avoid
        // ambiguity, this constructor is disabled if `U` is a `Result<J>`, where
        // `J` is convertible to `T`.
        template<typename U = T,
                turbo::enable_if_t<internal_statusor::IsConstructionValid<
                        false, T, U, false>::value,
                        int> = 0>
        Result(U &&u)  // NOLINT
                : Result(turbo::in_place, std::forward<U>(u)) {}

        template<typename U = T,
                turbo::enable_if_t<internal_statusor::IsConstructionValid<
                        false, T, U, true>::value,
                        int> = 0>
        Result(U &&u TURBO_ATTRIBUTE_LIFETIME_BOUND)  // NOLINT
                : Result(turbo::in_place, std::forward<U>(u)) {}

        template<typename U = T,
                turbo::enable_if_t<internal_statusor::IsConstructionValid<
                        true, T, U, false>::value,
                        int> = 0>
        explicit Result(U &&u)  // NOLINT
                : Result(turbo::in_place, std::forward<U>(u)) {}

        template<typename U = T,
                turbo::enable_if_t<
                        internal_statusor::IsConstructionValid<true, T, U, true>::value,
                        int> = 0>
        explicit Result(U &&u TURBO_ATTRIBUTE_LIFETIME_BOUND)  // NOLINT
                : Result(turbo::in_place, std::forward<U>(u)) {}

        // Result<T>::ok()
        //
        // Returns whether or not this `turbo::Result<T>` holds a `T` value. This
        // member function is analogous to `turbo::Status::ok()` and should be used
        // similarly to check the status of return values.
        //
        // Example:
        //
        // Result<Foo> result = DoBigCalculationThatCouldFail();
        // if (result.ok()) {
        //    // Handle result
        // else {
        //    // Handle error
        // }
        TURBO_MUST_USE_RESULT bool ok() const { return this->status_.ok(); }

        // Result<T>::status()
        //
        // Returns a reference to the current `turbo::Status` contained within the
        // `turbo::Result<T>`. If `turbo::Result<T>` contains a `T`, then this
        // function returns `turbo::OkStatus()`.
        const Status &status() const &;

        Status status() &&;

        // Result<T>::value()
        //
        // Returns a reference to the held value if `this->ok()`. Otherwise, throws
        // `turbo::BadResultAccess` if exceptions are enabled, or is guaranteed to
        // terminate the process if exceptions are disabled.
        //
        // If you have already checked the status using `this->ok()`, you probably
        // want to use `operator*()` or `operator->()` to access the value instead of
        // `value`.
        //
        // Note: for value types that are cheap to copy, prefer simple code:
        //
        //   T value = statusor.value();
        //
        // Otherwise, if the value type is expensive to copy, but can be left
        // in the Result, simply assign to a reference:
        //
        //   T& value = statusor.value();  // or `const T&`
        //
        // Otherwise, if the value type supports an efficient move, it can be
        // used as follows:
        //
        //   T value = std::move(statusor).value();
        //
        // The `std::move` on statusor instead of on the whole expression enables
        // warnings about possible uses of the statusor object after the move.
        const T &value() const & TURBO_ATTRIBUTE_LIFETIME_BOUND;

        T &value() & TURBO_ATTRIBUTE_LIFETIME_BOUND;

        const T &&value() const && TURBO_ATTRIBUTE_LIFETIME_BOUND;

        T &&value() && TURBO_ATTRIBUTE_LIFETIME_BOUND;

        // Result<T>:: operator*()
        //
        // Returns a reference to the current value.
        //
        // REQUIRES: `this->ok() == true`, otherwise the behavior is undefined.
        //
        // Use `this->ok()` to verify that there is a current value within the
        // `turbo::Result<T>`. Alternatively, see the `value()` member function for a
        // similar API that guarantees crashing or throwing an exception if there is
        // no current value.
        const T &operator*() const & TURBO_ATTRIBUTE_LIFETIME_BOUND;

        T &operator*() & TURBO_ATTRIBUTE_LIFETIME_BOUND;

        const T &&operator*() const && TURBO_ATTRIBUTE_LIFETIME_BOUND;

        T &&operator*() && TURBO_ATTRIBUTE_LIFETIME_BOUND;

        // Result<T>::operator->()
        //
        // Returns a pointer to the current value.
        //
        // REQUIRES: `this->ok() == true`, otherwise the behavior is undefined.
        //
        // Use `this->ok()` to verify that there is a current value.
        const T *operator->() const TURBO_ATTRIBUTE_LIFETIME_BOUND;

        T *operator->() TURBO_ATTRIBUTE_LIFETIME_BOUND;

        // Result<T>::value_or()
        //
        // Returns the current value if `this->ok() == true`. Otherwise constructs a
        // value using the provided `default_value`.
        //
        // Unlike `value`, this function returns by value, copying the current value
        // if necessary. If the value type supports an efficient move, it can be used
        // as follows:
        //
        //   T value = std::move(statusor).value_or(def);
        //
        // Unlike with `value`, calling `std::move()` on the result of `value_or` will
        // still trigger a copy.
        template<typename U>
        T value_or(U &&default_value) const &;

        template<typename U>
        T value_or(U &&default_value) &&;

        // Result<T>::ignore_error()
        //
        // Ignores any errors. This method does nothing except potentially suppress
        // complaints from any tools that are checking that errors are not dropped on
        // the floor.
        void ignore_error() const;

        // Result<T>::emplace()
        //
        // Reconstructs the inner value T in-place using the provided args, using the
        // T(args...) constructor. Returns reference to the reconstructed `T`.
        template<typename... Args>
        T &emplace(Args &&... args) TURBO_ATTRIBUTE_LIFETIME_BOUND {
            if (ok()) {
                this->Clear();
                this->MakeValue(std::forward<Args>(args)...);
            } else {
                this->MakeValue(std::forward<Args>(args)...);
                this->status_ = turbo::OkStatus();
            }
            return this->data_;
        }

        template<
                typename U, typename... Args,
                turbo::enable_if_t<
                        std::is_constructible<T, std::initializer_list<U> &, Args &&...>::value,
                        int> = 0>
        T &emplace(std::initializer_list<U> ilist,
                   Args &&... args) TURBO_ATTRIBUTE_LIFETIME_BOUND {
            if (ok()) {
                this->Clear();
                this->MakeValue(ilist, std::forward<Args>(args)...);
            } else {
                this->MakeValue(ilist, std::forward<Args>(args)...);
                this->status_ = turbo::OkStatus();
            }
            return this->data_;
        }

        // Result<T>::AssignStatus()
        //
        // Sets the status of `turbo::Result<T>` to the given non-ok status value.
        //
        // NOTE: We recommend using the constructor and `operator=` where possible.
        // This method is intended for use in generic programming, to enable setting
        // the status of a `Result<T>` when `T` may be `Status`. In that case, the
        // constructor and `operator=` would assign into the inner value of type
        // `Status`, rather than status of the `Result` (b/280392796).
        //
        // REQUIRES: !Status(std::forward<U>(v)).ok(). This requirement is DCHECKed.
        // In optimized builds, passing turbo::OkStatus() here will have the effect
        // of passing turbo::StatusCode::kInternal as a fallback.
        using internal_statusor::ResultData<T>::AssignStatus;

    private:
        using internal_statusor::ResultData<T>::Assign;

        template<typename U>
        void Assign(const turbo::Result<U> &other);

        template<typename U>
        void Assign(turbo::Result<U> &&other);
    };

// operator==()
//
// This operator checks the equality of two `turbo::Result<T>` objects.
    template<typename T>
    bool operator==(const Result<T> &lhs, const Result<T> &rhs) {
        if (lhs.ok() && rhs.ok()) return *lhs == *rhs;
        return lhs.status() == rhs.status();
    }

// operator!=()
//
// This operator checks the inequality of two `turbo::Result<T>` objects.
    template<typename T>
    bool operator!=(const Result<T> &lhs, const Result<T> &rhs) {
        return !(lhs == rhs);
    }

// Prints the `value` or the status in brackets to `os`.
//
// Requires `T` supports `operator<<`.  Do not rely on the output format which
// may change without notice.
    template<typename T, typename std::enable_if<
            turbo::HasOstreamOperator<T>::value, int>::type = 0>
    std::ostream &operator<<(std::ostream &os, const Result<T> &status_or) {
        if (status_or.ok()) {
            os << status_or.value();
        } else {
            os << internal_statusor::StringifyRandom::OpenBrackets()
               << status_or.status()
               << internal_statusor::StringifyRandom::CloseBrackets();
        }
        return os;
    }

// As above, but supports `str_cat`, `str_format`, etc.
//
// Requires `T` has `turbo_stringify`.  Do not rely on the output format which
// may change without notice.
    template<
            typename Sink, typename T,
            typename std::enable_if<turbo::HasTurboStringify<T>::value, int>::type = 0>
    void turbo_stringify(Sink &sink, const Result<T> &status_or) {
        if (status_or.ok()) {
            turbo::format(&sink, "%v", status_or.value());
        } else {
            turbo::format(&sink, "%s%v%s",
                          internal_statusor::StringifyRandom::OpenBrackets(),
                          status_or.status(),
                          internal_statusor::StringifyRandom::CloseBrackets());
        }
    }

//------------------------------------------------------------------------------
// Implementation details for Result<T>
//------------------------------------------------------------------------------

// TODO(sbenza): avoid the string here completely.
    template<typename T>
    Result<T>::Result() : Base(Status(turbo::StatusCode::kUnknown, "")) {}

    template<typename T>
    template<typename U>
    inline void Result<T>::Assign(const Result<U> &other) {
        if (other.ok()) {
            this->Assign(*other);
        } else {
            this->AssignStatus(other.status());
        }
    }

    template<typename T>
    template<typename U>
    inline void Result<T>::Assign(Result<U> &&other) {
        if (other.ok()) {
            this->Assign(*std::move(other));
        } else {
            this->AssignStatus(std::move(other).status());
        }
    }

    template<typename T>
    template<typename... Args>
    Result<T>::Result(turbo::in_place_t, Args &&... args)
            : Base(turbo::in_place, std::forward<Args>(args)...) {}

    template<typename T>
    template<typename U, typename... Args>
    Result<T>::Result(turbo::in_place_t, std::initializer_list<U> ilist,
                          Args &&... args)
            : Base(turbo::in_place, ilist, std::forward<Args>(args)...) {}

    template<typename T>
    const Status &Result<T>::status() const &{
        return this->status_;
    }

    template<typename T>
    Status Result<T>::status() &&{
        return ok() ? OkStatus() : std::move(this->status_);
    }

    template<typename T>
    const T &Result<T>::value() const &{
        if (!this->ok()) internal_statusor::ThrowBadStatusOrAccess(this->status_);
        return this->data_;
    }

    template<typename T>
    T &Result<T>::value() &{
        if (!this->ok()) internal_statusor::ThrowBadStatusOrAccess(this->status_);
        return this->data_;
    }

    template<typename T>
    const T &&Result<T>::value() const &&{
        if (!this->ok()) {
            internal_statusor::ThrowBadStatusOrAccess(std::move(this->status_));
        }
        return std::move(this->data_);
    }

    template<typename T>
    T &&Result<T>::value() &&{
        if (!this->ok()) {
            internal_statusor::ThrowBadStatusOrAccess(std::move(this->status_));
        }
        return std::move(this->data_);
    }

    template<typename T>
    const T &Result<T>::operator*() const &{
        this->EnsureOk();
        return this->data_;
    }

    template<typename T>
    T &Result<T>::operator*() &{
        this->EnsureOk();
        return this->data_;
    }

    template<typename T>
    const T &&Result<T>::operator*() const &&{
        this->EnsureOk();
        return std::move(this->data_);
    }

    template<typename T>
    T &&Result<T>::operator*() &&{
        this->EnsureOk();
        return std::move(this->data_);
    }

    template<typename T>
    turbo::Nonnull<const T *> Result<T>::operator->() const {
        this->EnsureOk();
        return &this->data_;
    }

    template<typename T>
    turbo::Nonnull<T *> Result<T>::operator->() {
        this->EnsureOk();
        return &this->data_;
    }

    template<typename T>
    template<typename U>
    T Result<T>::value_or(U &&default_value) const &{
        if (ok()) {
            return this->data_;
        }
        return std::forward<U>(default_value);
    }

    template<typename T>
    template<typename U>
    T Result<T>::value_or(U &&default_value) &&{
        if (ok()) {
            return std::move(this->data_);
        }
        return std::forward<U>(default_value);
    }

    template<typename T>
    void Result<T>::ignore_error() const {
        // no-op
    }

}  // namespace turbo
