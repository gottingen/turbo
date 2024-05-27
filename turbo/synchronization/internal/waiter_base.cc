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

#include <turbo/synchronization/internal/waiter_base.h>

#include <turbo/base/config.h>
#include <turbo/base/internal/thread_identity.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr int WaiterBase::kIdlePeriods;
#endif

void WaiterBase::MaybeBecomeIdle() {
  base_internal::ThreadIdentity *identity =
      base_internal::CurrentThreadIdentityIfPresent();
  assert(identity != nullptr);
  const bool is_idle = identity->is_idle.load(std::memory_order_relaxed);
  const int ticker = identity->ticker.load(std::memory_order_relaxed);
  const int wait_start = identity->wait_start.load(std::memory_order_relaxed);
  if (!is_idle && ticker - wait_start > kIdlePeriods) {
    identity->is_idle.store(true, std::memory_order_relaxed);
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo
