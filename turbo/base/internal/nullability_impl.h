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

#ifndef TURBO_BASE_INTERNAL_NULLABILITY_IMPL_H_
#define TURBO_BASE_INTERNAL_NULLABILITY_IMPL_H_

#include <memory>
#include <type_traits>

#include <turbo/base/attributes.h>
#include <turbo/meta/type_traits.h>

namespace turbo {

namespace nullability_internal {

// `IsNullabilityCompatible` checks whether its first argument is a class
// explicitly tagged as supporting nullability annotations. The tag is the type
// declaration `turbo_nullability_compatible`.
template <typename, typename = void>
struct IsNullabilityCompatible : std::false_type {};

template <typename T>
struct IsNullabilityCompatible<
    T, turbo::void_t<typename T::turbo_nullability_compatible>> : std::true_type {
};

template <typename T>
constexpr bool IsSupportedType = IsNullabilityCompatible<T>::value;

template <typename T>
constexpr bool IsSupportedType<T*> = true;

template <typename T, typename U>
constexpr bool IsSupportedType<T U::*> = true;

template <typename T, typename... Deleter>
constexpr bool IsSupportedType<std::unique_ptr<T, Deleter...>> = true;

template <typename T>
constexpr bool IsSupportedType<std::shared_ptr<T>> = true;

template <typename T>
struct EnableNullable {
  static_assert(nullability_internal::IsSupportedType<std::remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See turbo/base/nullability.h.");
  using type = T;
};

template <typename T>
struct EnableNonnull {
  static_assert(nullability_internal::IsSupportedType<std::remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See turbo/base/nullability.h.");
  using type = T;
};

template <typename T>
struct EnableNullabilityUnknown {
  static_assert(nullability_internal::IsSupportedType<std::remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See turbo/base/nullability.h.");
  using type = T;
};

// Note: we do not apply Clang nullability attributes (e.g. _Nullable).  These
// only support raw pointers, and conditionally enabling them only for raw
// pointers inhibits template arg deduction.  Ideally, they would support all
// pointer-like types.
template <typename T, typename = typename EnableNullable<T>::type>
using NullableImpl
#if TURBO_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullable")]]
#endif
    = T;

template <typename T, typename = typename EnableNonnull<T>::type>
using NonnullImpl
#if TURBO_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nonnull")]]
#endif
    = T;

template <typename T, typename = typename EnableNullabilityUnknown<T>::type>
using NullabilityUnknownImpl
#if TURBO_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullability_Unspecified")]]
#endif
    = T;

}  // namespace nullability_internal
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_NULLABILITY_IMPL_H_
