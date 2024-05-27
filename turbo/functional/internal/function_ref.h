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

#ifndef TURBO_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
#define TURBO_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_

#include <cassert>
#include <functional>
#include <type_traits>

#include <turbo/base/internal/invoke.h>
#include <turbo/functional/any_invocable.h>
#include <turbo/meta/type_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace functional_internal {

// Like a void* that can handle function pointers as well. The standard does not
// allow function pointers to round-trip through void*, but void(*)() is fine.
//
// Note: It's important that this class remains trivial and is the same size as
// a pointer, since this allows the compiler to perform tail-call optimizations
// when the underlying function is a callable object with a matching signature.
union VoidPtr {
  const void* obj;
  void (*fun)();
};

// Chooses the best type for passing T as an argument.
// Attempt to be close to SystemV AMD64 ABI. Objects with trivial copy ctor are
// passed by value.
template <typename T,
          bool IsLValueReference = std::is_lvalue_reference<T>::value>
struct PassByValue : std::false_type {};

template <typename T>
struct PassByValue<T, /*IsLValueReference=*/false>
    : std::integral_constant<bool,
                             turbo::is_trivially_copy_constructible<T>::value &&
                                 turbo::is_trivially_copy_assignable<
                                     typename std::remove_cv<T>::type>::value &&
                                 std::is_trivially_destructible<T>::value &&
                                 sizeof(T) <= 2 * sizeof(void*)> {};

template <typename T>
struct ForwardT : std::conditional<PassByValue<T>::value, T, T&&> {};

// An Invoker takes a pointer to the type-erased invokable object, followed by
// the arguments that the invokable object expects.
//
// Note: The order of arguments here is an optimization, since member functions
// have an implicit "this" pointer as their first argument, putting VoidPtr
// first allows the compiler to perform tail-call optimization in many cases.
template <typename R, typename... Args>
using Invoker = R (*)(VoidPtr, typename ForwardT<Args>::type...);

//
// InvokeObject and InvokeFunction provide static "Invoke" functions that can be
// used as Invokers for objects or functions respectively.
//
// static_cast<R> handles the case the return type is void.
template <typename Obj, typename R, typename... Args>
R InvokeObject(VoidPtr ptr, typename ForwardT<Args>::type... args) {
  auto o = static_cast<const Obj*>(ptr.obj);
  return static_cast<R>(
      turbo::base_internal::invoke(*o, std::forward<Args>(args)...));
}

template <typename Fun, typename R, typename... Args>
R InvokeFunction(VoidPtr ptr, typename ForwardT<Args>::type... args) {
  auto f = reinterpret_cast<Fun>(ptr.fun);
  return static_cast<R>(
      turbo::base_internal::invoke(f, std::forward<Args>(args)...));
}

template <typename Sig>
void AssertNonNull(const std::function<Sig>& f) {
  assert(f != nullptr);
  (void)f;
}

template <typename Sig>
void AssertNonNull(const AnyInvocable<Sig>& f) {
  assert(f != nullptr);
  (void)f;
}

template <typename F>
void AssertNonNull(const F&) {}

template <typename F, typename C>
void AssertNonNull(F C::*f) {
  assert(f != nullptr);
  (void)f;
}

template <bool C>
using EnableIf = typename ::std::enable_if<C, int>::type;

}  // namespace functional_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FUNCTIONAL_INTERNAL_FUNCTION_REF_H_
