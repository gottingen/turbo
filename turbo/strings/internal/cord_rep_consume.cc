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

#include <turbo/strings/internal/cord_rep_consume.h>

#include <array>
#include <utility>

#include <turbo/container/inlined_vector.h>
#include <turbo/functional/function_ref.h>
#include <turbo/strings/internal/cord_internal.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

namespace {

// Unrefs the provided `substring`, and returns `substring->child`
// Adds or assumes a reference on `substring->child`
CordRep* ClipSubstring(CordRepSubstring* substring) {
  CordRep* child = substring->child;
  if (substring->refcount.IsOne()) {
    delete substring;
  } else {
    CordRep::Ref(child);
    CordRep::Unref(substring);
  }
  return child;
}

}  // namespace

void Consume(CordRep* rep,
             FunctionRef<void(CordRep*, size_t, size_t)> consume_fn) {
  size_t offset = 0;
  size_t length = rep->length;

  if (rep->tag == SUBSTRING) {
    offset += rep->substring()->start;
    rep = ClipSubstring(rep->substring());
  }
  consume_fn(rep, offset, length);
}

void ReverseConsume(CordRep* rep,
                    FunctionRef<void(CordRep*, size_t, size_t)> consume_fn) {
  return Consume(rep, consume_fn);
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
