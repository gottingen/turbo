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

#ifndef TURBO_STRINGS_INTERNAL_CORDZ_UPDATE_SCOPE_H_
#define TURBO_STRINGS_INTERNAL_CORDZ_UPDATE_SCOPE_H_

#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/internal/cordz_info.h>
#include <turbo/strings/internal/cordz_update_tracker.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

// CordzUpdateScope scopes an update to the provided CordzInfo.
// The class invokes `info->Lock(method)` and `info->Unlock()` to guard
// cordrep updates. This class does nothing if `info` is null.
// See also the 'Lock`, `Unlock` and `SetCordRep` methods in `CordzInfo`.
class TURBO_SCOPED_LOCKABLE CordzUpdateScope {
 public:
  CordzUpdateScope(CordzInfo* info, CordzUpdateTracker::MethodIdentifier method)
      TURBO_EXCLUSIVE_LOCK_FUNCTION(info)
      : info_(info) {
    if (TURBO_PREDICT_FALSE(info_)) {
      info->Lock(method);
    }
  }

  // CordzUpdateScope can not be copied or assigned to.
  CordzUpdateScope(CordzUpdateScope&& rhs) = delete;
  CordzUpdateScope(const CordzUpdateScope&) = delete;
  CordzUpdateScope& operator=(CordzUpdateScope&& rhs) = delete;
  CordzUpdateScope& operator=(const CordzUpdateScope&) = delete;

  ~CordzUpdateScope() TURBO_UNLOCK_FUNCTION() {
    if (TURBO_PREDICT_FALSE(info_)) {
      info_->Unlock();
    }
  }

  void SetCordRep(CordRep* rep) const {
    if (TURBO_PREDICT_FALSE(info_)) {
      info_->SetCordRep(rep);
    }
  }

  CordzInfo* info() const { return info_; }

 private:
  CordzInfo* info_;
};

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_CORDZ_UPDATE_SCOPE_H_
