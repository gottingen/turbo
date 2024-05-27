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

#ifndef TURBO_STRINGS_INTERNAL_CORD_DATA_EDGE_H_
#define TURBO_STRINGS_INTERNAL_CORD_DATA_EDGE_H_

#include <cassert>
#include <cstddef>

#include <turbo/base/config.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/internal/cord_rep_flat.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

// Returns true if the provided rep is a FLAT, EXTERNAL or a SUBSTRING node
// holding a FLAT or EXTERNAL child rep. Requires `rep != nullptr`.
inline bool IsDataEdge(const CordRep* edge) {
  assert(edge != nullptr);

  // The fast path is that `edge` is an EXTERNAL or FLAT node, making the below
  // if a single, well predicted branch. We then repeat the FLAT or EXTERNAL
  // check in the slow path of the SUBSTRING check to optimize for the hot path.
  if (edge->tag == EXTERNAL || edge->tag >= FLAT) return true;
  if (edge->tag == SUBSTRING) edge = edge->substring()->child;
  return edge->tag == EXTERNAL || edge->tag >= FLAT;
}

// Returns the `turbo::string_view` data reference for the provided data edge.
// Requires 'IsDataEdge(edge) == true`.
inline turbo::string_view EdgeData(const CordRep* edge) {
  assert(IsDataEdge(edge));

  size_t offset = 0;
  const size_t length = edge->length;
  if (edge->IsSubstring()) {
    offset = edge->substring()->start;
    edge = edge->substring()->child;
  }
  return edge->tag >= FLAT
             ? turbo::string_view{edge->flat()->Data() + offset, length}
             : turbo::string_view{edge->external()->base + offset, length};
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_CORD_DATA_EDGE_H_
