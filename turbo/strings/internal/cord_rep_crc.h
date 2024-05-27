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

#ifndef TURBO_STRINGS_INTERNAL_CORD_REP_CRC_H_
#define TURBO_STRINGS_INTERNAL_CORD_REP_CRC_H_

#include <cassert>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/crc/internal/crc_cord_state.h>
#include <turbo/strings/internal/cord_internal.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

// CordRepCrc is a CordRep node intended only to appear at the top level of a
// cord tree.  It associates an "expected CRC" with the contained data, to allow
// for easy passage of checksum data in Cord data flows.
//
// From Cord's perspective, the crc value has no semantics; any validation of
// the contained checksum is the user's responsibility.
struct CordRepCrc : public CordRep {
  CordRep* child;
  turbo::crc_internal::CrcCordState crc_cord_state;

  // Consumes `child` and returns a CordRepCrc prefixed tree containing `child`.
  // If the specified `child` is itself a CordRepCrc node, then this method
  // either replaces the existing node, or directly updates the crc state in it
  // depending on the node being shared or not, i.e.: refcount.IsOne().
  // `child` must only be null if the Cord is empty. Never returns null.
  static CordRepCrc* New(CordRep* child, crc_internal::CrcCordState state);

  // Destroys (deletes) the provided node. `node` must not be null.
  static void Destroy(CordRepCrc* node);
};

// Consumes `rep` and returns a CordRep* with any outer CordRepCrc wrapper
// removed.  This is usually a no-op (returning `rep`), but this will remove and
// unref an outer CordRepCrc node.
inline CordRep* RemoveCrcNode(CordRep* rep) {
  assert(rep != nullptr);
  if (TURBO_PREDICT_FALSE(rep->IsCrc())) {
    CordRep* child = rep->crc()->child;
    if (rep->refcount.IsOne()) {
      delete rep->crc();
    } else {
      CordRep::Ref(child);
      CordRep::Unref(rep);
    }
    return child;
  }
  return rep;
}

// Returns `rep` if it is not a CordRepCrc node, or its child if it is.
// Does not consume or create a reference on `rep` or the returned value.
inline CordRep* SkipCrcNode(CordRep* rep) {
  assert(rep != nullptr);
  if (TURBO_PREDICT_FALSE(rep->IsCrc())) {
    return rep->crc()->child;
  } else {
    return rep;
  }
}

inline const CordRep* SkipCrcNode(const CordRep* rep) {
  assert(rep != nullptr);
  if (TURBO_PREDICT_FALSE(rep->IsCrc())) {
    return rep->crc()->child;
  } else {
    return rep;
  }
}

inline CordRepCrc* CordRep::crc() {
  assert(IsCrc());
  return static_cast<CordRepCrc*>(this);
}

inline const CordRepCrc* CordRep::crc() const {
  assert(IsCrc());
  return static_cast<const CordRepCrc*>(this);
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_CORD_REP_CRC_H_
