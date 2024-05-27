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
#include <turbo/strings/internal/cord_internal.h>

#include <atomic>
#include <cassert>
#include <memory>

#include <turbo/base/internal/raw_logging.h>
#include <turbo/container/inlined_vector.h>
#include <turbo/strings/internal/cord_rep_btree.h>
#include <turbo/strings/internal/cord_rep_crc.h>
#include <turbo/strings/internal/cord_rep_flat.h>
#include <turbo/strings/str_cat.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

TURBO_CONST_INIT std::atomic<bool> shallow_subcords_enabled(
    kCordShallowSubcordsDefault);

void LogFatalNodeType(CordRep* rep) {
  TURBO_INTERNAL_LOG(FATAL, turbo::StrCat("Unexpected node type: ",
                                        static_cast<int>(rep->tag)));
}

void CordRep::Destroy(CordRep* rep) {
  assert(rep != nullptr);

  while (true) {
    assert(!rep->refcount.IsImmortal());
    if (rep->tag == BTREE) {
      CordRepBtree::Destroy(rep->btree());
      return;
    } else if (rep->tag == EXTERNAL) {
      CordRepExternal::Delete(rep);
      return;
    } else if (rep->tag == SUBSTRING) {
      CordRepSubstring* rep_substring = rep->substring();
      rep = rep_substring->child;
      delete rep_substring;
      if (rep->refcount.Decrement()) {
        return;
      }
    } else if (rep->tag == CRC) {
      CordRepCrc::Destroy(rep->crc());
      return;
    } else {
      assert(rep->IsFlat());
      CordRepFlat::Delete(rep);
      return;
    }
  }
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
