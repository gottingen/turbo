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

#include <turbo/strings/internal/cord_rep_crc.h>

#include <cassert>
#include <cstdint>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/strings/internal/cord_internal.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

CordRepCrc* CordRepCrc::New(CordRep* child, crc_internal::CrcCordState state) {
  if (child != nullptr && child->IsCrc()) {
    if (child->refcount.IsOne()) {
      child->crc()->crc_cord_state = std::move(state);
      return child->crc();
    }
    CordRep* old = child;
    child = old->crc()->child;
    CordRep::Ref(child);
    CordRep::Unref(old);
  }
  auto* new_cordrep = new CordRepCrc;
  new_cordrep->length = child != nullptr ? child->length : 0;
  new_cordrep->tag = cord_internal::CRC;
  new_cordrep->child = child;
  new_cordrep->crc_cord_state = std::move(state);
  return new_cordrep;
}

void CordRepCrc::Destroy(CordRepCrc* node) {
  if (node->child != nullptr) {
    CordRep::Unref(node->child);
  }
  delete node;
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
