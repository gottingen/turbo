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

// Common code shared between turbo/hash/hash_test.cc and
// turbo/hash/hash_instantiated_test.cc.

#ifndef TURBO_HASH_INTERNAL_HASH_TEST_H_
#define TURBO_HASH_INTERNAL_HASH_TEST_H_

#include <type_traits>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/hash/hash.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace hash_test_internal {

// Utility wrapper of T for the purposes of testing the `TurboHash` type erasure
// mechanism.  `TypeErasedValue<T>` can be constructed with a `T`, and can
// be compared and hashed.  However, all hashing goes through the hashing
// type-erasure framework.
template <typename T>
class TypeErasedValue {
 public:
  TypeErasedValue() = default;
  TypeErasedValue(const TypeErasedValue&) = default;
  TypeErasedValue(TypeErasedValue&&) = default;
  explicit TypeErasedValue(const T& n) : n_(n) {}

  template <typename H>
  friend H turbo_hash_value(H hash_state, const TypeErasedValue& v) {
    v.HashValue(turbo::HashState::Create(&hash_state));
    return hash_state;
  }

  void HashValue(turbo::HashState state) const {
    turbo::HashState::combine(std::move(state), n_);
  }

  bool operator==(const TypeErasedValue& rhs) const { return n_ == rhs.n_; }
  bool operator!=(const TypeErasedValue& rhs) const { return !(*this == rhs); }

 private:
  T n_;
};

// A TypeErasedValue refinement, for containers.  It exposes the wrapped
// `value_type` and is constructible from an initializer list.
template <typename T>
class TypeErasedContainer : public TypeErasedValue<T> {
 public:
  using value_type = typename T::value_type;
  TypeErasedContainer() = default;
  TypeErasedContainer(const TypeErasedContainer&) = default;
  TypeErasedContainer(TypeErasedContainer&&) = default;
  explicit TypeErasedContainer(const T& n) : TypeErasedValue<T>(n) {}
  TypeErasedContainer(std::initializer_list<value_type> init_list)
      : TypeErasedContainer(T(init_list.begin(), init_list.end())) {}
  // one-argument constructor of value type T, to appease older toolchains that
  // get confused by one-element initializer lists in some contexts
  explicit TypeErasedContainer(const value_type& v)
      : TypeErasedContainer(T(&v, &v + 1)) {}
};

// Helper trait to verify if T is hashable. We use turbo::Hash's poison status to
// detect it.
template <typename T>
using is_hashable = std::is_default_constructible<turbo::Hash<T>>;

}  // namespace hash_test_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_HASH_INTERNAL_HASH_TEST_H_
