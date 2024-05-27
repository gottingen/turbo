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
// Adapts a policy for nodes.
//
// The node policy should model:
//
// struct Policy {
//   // Returns a new node allocated and constructed using the allocator, using
//   // the specified arguments.
//   template <class Alloc, class... Args>
//   value_type* new_element(Alloc* alloc, Args&&... args) const;
//
//   // Destroys and deallocates node using the allocator.
//   template <class Alloc>
//   void delete_element(Alloc* alloc, value_type* node) const;
// };
//
// It may also optionally define `value()` and `apply()`. For documentation on
// these, see hash_policy_traits.h.

#ifndef TURBO_CONTAINER_INTERNAL_NODE_SLOT_POLICY_H_
#define TURBO_CONTAINER_INTERNAL_NODE_SLOT_POLICY_H_

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

template <class Reference, class Policy>
struct node_slot_policy {
  static_assert(std::is_lvalue_reference<Reference>::value, "");

  using slot_type = typename std::remove_cv<
      typename std::remove_reference<Reference>::type>::type*;

  template <class Alloc, class... Args>
  static void construct(Alloc* alloc, slot_type* slot, Args&&... args) {
    *slot = Policy::new_element(alloc, std::forward<Args>(args)...);
  }

  template <class Alloc>
  static void destroy(Alloc* alloc, slot_type* slot) {
    Policy::delete_element(alloc, *slot);
  }

  // Returns true_type to indicate that transfer can use memcpy.
  template <class Alloc>
  static std::true_type transfer(Alloc*, slot_type* new_slot,
                                 slot_type* old_slot) {
    *new_slot = *old_slot;
    return {};
  }

  static size_t space_used(const slot_type* slot) {
    if (slot == nullptr) return Policy::element_space_used(nullptr);
    return Policy::element_space_used(*slot);
  }

  static Reference element(slot_type* slot) { return **slot; }

  template <class T, class P = Policy>
  static auto value(T* elem) -> decltype(P::value(elem)) {
    return P::value(elem);
  }

  template <class... Ts, class P = Policy>
  static auto apply(Ts&&... ts) -> decltype(P::apply(std::forward<Ts>(ts)...)) {
    return P::apply(std::forward<Ts>(ts)...);
  }
};

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CONTAINER_INTERNAL_NODE_SLOT_POLICY_H_
