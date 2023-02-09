//
// Copyright 2020 The Turbo Authors.
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
//
// -----------------------------------------------------------------------------
// any.h
// -----------------------------------------------------------------------------
//
// This header file define the `turbo::any` type for holding a type-safe value
// of any type. The 'turbo::any` type is useful for providing a way to hold
// something that is, as yet, unspecified. Such unspecified types
// traditionally are passed between API boundaries until they are later cast to
// their "destination" types. To cast to such a destination type, use
// `turbo::any_cast()`. Note that when casting an `turbo::any`, you must cast it
// to an explicit type; implicit conversions will throw.
//
// Example:
//
//   auto a = turbo::any(65);
//   turbo::any_cast<int>(a);         // 65
//   turbo::any_cast<char>(a);        // throws turbo::bad_any_cast
//   turbo::any_cast<std::string>(a); // throws turbo::bad_any_cast
//
// `turbo::any` is a C++11 compatible version of the C++17 `std::any` abstraction
// and is designed to be a drop-in replacement for code compliant with C++17.
//
// Traditionally, the behavior of casting to a temporary unspecified type has
// been accomplished with the `void *` paradigm, where the pointer was to some
// other unspecified type. `turbo::any` provides an "owning" version of `void *`
// that avoids issues of pointer management.
//
// Note: just as in the case of `void *`, use of `turbo::any` (and its C++17
// version `std::any`) is a code smell indicating that your API might not be
// constructed correctly. We have seen that most uses of `any` are unwarranted,
// and `turbo::any`, like `std::any`, is difficult to use properly. Before using
// this abstraction, make sure that you should not instead be rewriting your
// code to be more specific.
//
// Turbo has also released an `turbo::variant` type (a C++11 compatible version
// of the C++17 `std::variant`), which is generally preferred for use over
// `turbo::any`.
#ifndef TURBO_TYPES_ANY_H_
#define TURBO_TYPES_ANY_H_

#include "turbo/meta/utility.h"
#include "turbo/platform/port.h"

#ifdef TURBO_USES_STD_ANY

#include <any>  // IWYU pragma: export

namespace turbo {
TURBO_NAMESPACE_BEGIN
using std::any;
using std::any_cast;
using std::bad_any_cast;
using std::make_any;
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // TURBO_USES_STD_ANY

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "turbo/base/internal/fast_type_id.h"
#include "turbo/meta/type_traits.h"
#include "bad_any_cast.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

class any;

// swap()
//
// Swaps two `turbo::any` values. Equivalent to `x.swap(y) where `x` and `y` are
// `turbo::any` types.
void swap(any& x, any& y) noexcept;

// make_any()
//
// Constructs an `turbo::any` of type `T` with the given arguments.
template <typename T, typename... Args>
any make_any(Args&&... args);

// Overload of `turbo::make_any()` for constructing an `turbo::any` type from an
// initializer list.
template <typename T, typename U, typename... Args>
any make_any(std::initializer_list<U> il, Args&&... args);

// any_cast()
//
// Statically casts the value of a `const turbo::any` type to the given type.
// This function will throw `turbo::bad_any_cast` if the stored value type of the
// `turbo::any` does not match the cast.
//
// `any_cast()` can also be used to get a reference to the internal storage iff
// a reference type is passed as its `ValueType`:
//
// Example:
//
//   turbo::any my_any = std::vector<int>();
//   turbo::any_cast<std::vector<int>&>(my_any).push_back(42);
template <typename ValueType>
ValueType any_cast(const any& operand);

// Overload of `any_cast()` to statically cast the value of a non-const
// `turbo::any` type to the given type. This function will throw
// `turbo::bad_any_cast` if the stored value type of the `turbo::any` does not
// match the cast.
template <typename ValueType>
ValueType any_cast(any& operand);  // NOLINT(runtime/references)

// Overload of `any_cast()` to statically cast the rvalue of an `turbo::any`
// type. This function will throw `turbo::bad_any_cast` if the stored value type
// of the `turbo::any` does not match the cast.
template <typename ValueType>
ValueType any_cast(any&& operand);

// Overload of `any_cast()` to statically cast the value of a const pointer
// `turbo::any` type to the given pointer type, or `nullptr` if the stored value
// type of the `turbo::any` does not match the cast.
template <typename ValueType>
const ValueType* any_cast(const any* operand) noexcept;

// Overload of `any_cast()` to statically cast the value of a pointer
// `turbo::any` type to the given pointer type, or `nullptr` if the stored value
// type of the `turbo::any` does not match the cast.
template <typename ValueType>
ValueType* any_cast(any* operand) noexcept;

// -----------------------------------------------------------------------------
// turbo::any
// -----------------------------------------------------------------------------
//
// An `turbo::any` object provides the facility to either store an instance of a
// type, known as the "contained object", or no value. An `turbo::any` is used to
// store values of types that are unknown at compile time. The `turbo::any`
// object, when containing a value, must contain a value type; storing a
// reference type is neither desired nor supported.
//
// An `turbo::any` can only store a type that is copy-constructible; move-only
// types are not allowed within an `any` object.
//
// Example:
//
//   auto a = turbo::any(65);                 // Literal, copyable
//   auto b = turbo::any(std::vector<int>()); // Default-initialized, copyable
//   std::unique_ptr<Foo> my_foo;
//   auto c = turbo::any(std::move(my_foo));  // Error, not copy-constructible
//
// Note that `turbo::any` makes use of decayed types (`turbo::decay_t` in this
// context) to remove const-volatile qualifiers (known as "cv qualifiers"),
// decay functions to function pointers, etc. We essentially "decay" a given
// type into its essential type.
//
// `turbo::any` makes use of decayed types when determining the basic type `T` of
// the value to store in the any's contained object. In the documentation below,
// we explicitly denote this by using the phrase "a decayed type of `T`".
//
// Example:
//
//   const int a = 4;
//   turbo::any foo(a);  // Decay ensures we store an "int", not a "const int&".
//
//   void my_function() {}
//   turbo::any bar(my_function);  // Decay ensures we store a function pointer.
//
// `turbo::any` is a C++11 compatible version of the C++17 `std::any` abstraction
// and is designed to be a drop-in replacement for code compliant with C++17.
class any {
 private:
  template <typename T>
  struct IsInPlaceType;

 public:
  // Constructors

  // Constructs an empty `turbo::any` object (`any::has_value()` will return
  // `false`).
  constexpr any() noexcept;

  // Copy constructs an `turbo::any` object with a "contained object" of the
  // passed type of `other` (or an empty `turbo::any` if `other.has_value()` is
  // `false`.
  any(const any& other)
      : obj_(other.has_value() ? other.obj_->Clone()
                               : std::unique_ptr<ObjInterface>()) {}

  // Move constructs an `turbo::any` object with a "contained object" of the
  // passed type of `other` (or an empty `turbo::any` if `other.has_value()` is
  // `false`).
  any(any&& other) noexcept = default;

  // Constructs an `turbo::any` object with a "contained object" of the decayed
  // type of `T`, which is initialized via `std::forward<T>(value)`.
  //
  // This constructor will not participate in overload resolution if the
  // decayed type of `T` is not copy-constructible.
  template <
      typename T, typename VT = turbo::decay_t<T>,
      turbo::enable_if_t<!turbo::disjunction<
          std::is_same<any, VT>, IsInPlaceType<VT>,
          turbo::negation<std::is_copy_constructible<VT> > >::value>* = nullptr>
  any(T&& value) : obj_(new Obj<VT>(in_place, std::forward<T>(value))) {}

  // Constructs an `turbo::any` object with a "contained object" of the decayed
  // type of `T`, which is initialized via `std::forward<T>(value)`.
  template <typename T, typename... Args, typename VT = turbo::decay_t<T>,
            turbo::enable_if_t<turbo::conjunction<
                std::is_copy_constructible<VT>,
                std::is_constructible<VT, Args...>>::value>* = nullptr>
  explicit any(in_place_type_t<T> /*tag*/, Args&&... args)
      : obj_(new Obj<VT>(in_place, std::forward<Args>(args)...)) {}

  // Constructs an `turbo::any` object with a "contained object" of the passed
  // type `VT` as a decayed type of `T`. `VT` is initialized as if
  // direct-non-list-initializing an object of type `VT` with the arguments
  // `initializer_list, std::forward<Args>(args)...`.
  template <
      typename T, typename U, typename... Args, typename VT = turbo::decay_t<T>,
      turbo::enable_if_t<
          turbo::conjunction<std::is_copy_constructible<VT>,
                            std::is_constructible<VT, std::initializer_list<U>&,
                                                  Args...>>::value>* = nullptr>
  explicit any(in_place_type_t<T> /*tag*/, std::initializer_list<U> ilist,
               Args&&... args)
      : obj_(new Obj<VT>(in_place, ilist, std::forward<Args>(args)...)) {}

  // Assignment operators

  // Copy assigns an `turbo::any` object with a "contained object" of the
  // passed type.
  any& operator=(const any& rhs) {
    any(rhs).swap(*this);
    return *this;
  }

  // Move assigns an `turbo::any` object with a "contained object" of the
  // passed type. `rhs` is left in a valid but otherwise unspecified state.
  any& operator=(any&& rhs) noexcept {
    any(std::move(rhs)).swap(*this);
    return *this;
  }

  // Assigns an `turbo::any` object with a "contained object" of the passed type.
  template <typename T, typename VT = turbo::decay_t<T>,
            turbo::enable_if_t<turbo::conjunction<
                turbo::negation<std::is_same<VT, any>>,
                std::is_copy_constructible<VT>>::value>* = nullptr>
  any& operator=(T&& rhs) {
    any tmp(in_place_type_t<VT>(), std::forward<T>(rhs));
    tmp.swap(*this);
    return *this;
  }

  // Modifiers

  // any::emplace()
  //
  // Emplaces a value within an `turbo::any` object by calling `any::reset()`,
  // initializing the contained value as if direct-non-list-initializing an
  // object of type `VT` with the arguments `std::forward<Args>(args)...`, and
  // returning a reference to the new contained value.
  //
  // Note: If an exception is thrown during the call to `VT`'s constructor,
  // `*this` does not contain a value, and any previously contained value has
  // been destroyed.
  template <
      typename T, typename... Args, typename VT = turbo::decay_t<T>,
      turbo::enable_if_t<std::is_copy_constructible<VT>::value &&
                        std::is_constructible<VT, Args...>::value>* = nullptr>
  VT& emplace(Args&&... args) {
    reset();  // NOTE: reset() is required here even in the world of exceptions.
    Obj<VT>* const object_ptr =
        new Obj<VT>(in_place, std::forward<Args>(args)...);
    obj_ = std::unique_ptr<ObjInterface>(object_ptr);
    return object_ptr->value;
  }

  // Overload of `any::emplace()` to emplace a value within an `turbo::any`
  // object by calling `any::reset()`, initializing the contained value as if
  // direct-non-list-initializing an object of type `VT` with the arguments
  // `initializer_list, std::forward<Args>(args)...`, and returning a reference
  // to the new contained value.
  //
  // Note: If an exception is thrown during the call to `VT`'s constructor,
  // `*this` does not contain a value, and any previously contained value has
  // been destroyed. The function shall not participate in overload resolution
  // unless `is_copy_constructible_v<VT>` is `true` and
  // `is_constructible_v<VT, initializer_list<U>&, Args...>` is `true`.
  template <
      typename T, typename U, typename... Args, typename VT = turbo::decay_t<T>,
      turbo::enable_if_t<std::is_copy_constructible<VT>::value &&
                        std::is_constructible<VT, std::initializer_list<U>&,
                                              Args...>::value>* = nullptr>
  VT& emplace(std::initializer_list<U> ilist, Args&&... args) {
    reset();  // NOTE: reset() is required here even in the world of exceptions.
    Obj<VT>* const object_ptr =
        new Obj<VT>(in_place, ilist, std::forward<Args>(args)...);
    obj_ = std::unique_ptr<ObjInterface>(object_ptr);
    return object_ptr->value;
  }

  // any::reset()
  //
  // Resets the state of the `turbo::any` object, destroying the contained object
  // if present.
  void reset() noexcept { obj_ = nullptr; }

  // any::swap()
  //
  // Swaps the passed value and the value of this `turbo::any` object.
  void swap(any& other) noexcept { obj_.swap(other.obj_); }

  // Observers

  // any::has_value()
  //
  // Returns `true` if the `any` object has a contained value, otherwise
  // returns `false`.
  bool has_value() const noexcept { return obj_ != nullptr; }

#ifdef TURBO_COMPILER_HAVE_RTTI
  // Returns: typeid(T) if *this has a contained object of type T, otherwise
  // typeid(void).
  const std::type_info& type() const noexcept {
    if (has_value()) {
      return obj_->Type();
    }

    return typeid(void);
  }
#endif  // TURBO_COMPILER_HAVE_RTTI

 private:
  // Tagged type-erased abstraction for holding a cloneable object.
  class ObjInterface {
   public:
    virtual ~ObjInterface() = default;
    virtual std::unique_ptr<ObjInterface> Clone() const = 0;
    virtual const void* ObjTypeId() const noexcept = 0;
#ifdef TURBO_COMPILER_HAVE_RTTI
    virtual const std::type_info& Type() const noexcept = 0;
#endif  // TURBO_COMPILER_HAVE_RTTI
  };

  // Hold a value of some queryable type, with an ability to Clone it.
  template <typename T>
  class Obj : public ObjInterface {
   public:
    template <typename... Args>
    explicit Obj(in_place_t /*tag*/, Args&&... args)
        : value(std::forward<Args>(args)...) {}

    std::unique_ptr<ObjInterface> Clone() const final {
      return std::unique_ptr<ObjInterface>(new Obj(in_place, value));
    }

    const void* ObjTypeId() const noexcept final { return IdForType<T>(); }

#ifdef TURBO_COMPILER_HAVE_RTTI
    const std::type_info& Type() const noexcept final { return typeid(T); }
#endif  // TURBO_COMPILER_HAVE_RTTI

    T value;
  };

  std::unique_ptr<ObjInterface> CloneObj() const {
    if (!obj_) return nullptr;
    return obj_->Clone();
  }

  template <typename T>
  constexpr static const void* IdForType() {
    // Note: This type dance is to make the behavior consistent with typeid.
    using NormalizedType =
        typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    return base_internal::FastTypeId<NormalizedType>();
  }

  const void* GetObjTypeId() const {
    return obj_ ? obj_->ObjTypeId() : base_internal::FastTypeId<void>();
  }

  // `turbo::any` nonmember functions //

  // Description at the declaration site (top of file).
  template <typename ValueType>
  friend ValueType any_cast(const any& operand);

  // Description at the declaration site (top of file).
  template <typename ValueType>
  friend ValueType any_cast(any& operand);  // NOLINT(runtime/references)

  // Description at the declaration site (top of file).
  template <typename T>
  friend const T* any_cast(const any* operand) noexcept;

  // Description at the declaration site (top of file).
  template <typename T>
  friend T* any_cast(any* operand) noexcept;

  std::unique_ptr<ObjInterface> obj_;
};

// -----------------------------------------------------------------------------
// Implementation Details
// -----------------------------------------------------------------------------

constexpr any::any() noexcept = default;

template <typename T>
struct any::IsInPlaceType : std::false_type {};

template <typename T>
struct any::IsInPlaceType<in_place_type_t<T>> : std::true_type {};

inline void swap(any& x, any& y) noexcept { x.swap(y); }

// Description at the declaration site (top of file).
template <typename T, typename... Args>
any make_any(Args&&... args) {
  return any(in_place_type_t<T>(), std::forward<Args>(args)...);
}

// Description at the declaration site (top of file).
template <typename T, typename U, typename... Args>
any make_any(std::initializer_list<U> il, Args&&... args) {
  return any(in_place_type_t<T>(), il, std::forward<Args>(args)...);
}

// Description at the declaration site (top of file).
template <typename ValueType>
ValueType any_cast(const any& operand) {
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, const U&>::value,
                "Invalid ValueType");
  auto* const result = (any_cast<U>)(&operand);
  if (result == nullptr) {
    any_internal::ThrowBadAnyCast();
  }
  return static_cast<ValueType>(*result);
}

// Description at the declaration site (top of file).
template <typename ValueType>
ValueType any_cast(any& operand) {  // NOLINT(runtime/references)
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, U&>::value,
                "Invalid ValueType");
  auto* result = (any_cast<U>)(&operand);
  if (result == nullptr) {
    any_internal::ThrowBadAnyCast();
  }
  return static_cast<ValueType>(*result);
}

// Description at the declaration site (top of file).
template <typename ValueType>
ValueType any_cast(any&& operand) {
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, U>::value,
                "Invalid ValueType");
  return static_cast<ValueType>(std::move((any_cast<U&>)(operand)));
}

// Description at the declaration site (top of file).
template <typename T>
const T* any_cast(const any* operand) noexcept {
  using U =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  return operand && operand->GetObjTypeId() == any::IdForType<U>()
             ? std::addressof(
                   static_cast<const any::Obj<U>*>(operand->obj_.get())->value)
             : nullptr;
}

// Description at the declaration site (top of file).
template <typename T>
T* any_cast(any* operand) noexcept {
  using U =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  return operand && operand->GetObjTypeId() == any::IdForType<U>()
             ? std::addressof(
                   static_cast<any::Obj<U>*>(operand->obj_.get())->value)
             : nullptr;
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USES_STD_ANY

#endif  // TURBO_TYPES_ANY_H_
