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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_SEM_WAITER_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_SEM_WAITER_H_

#include <turbo/base/config.h>

#ifdef TURBO_HAVE_SEMAPHORE_H
#include <semaphore.h>

#include <atomic>
#include <cstdint>

#include <turbo/base/internal/thread_identity.h>
#include <turbo/synchronization/internal/futex.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/waiter_base.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#define TURBO_INTERNAL_HAVE_SEM_WAITER 1

class SemWaiter : public WaiterCrtp<SemWaiter> {
 public:
  SemWaiter();

  bool Wait(KernelTimeout t);
  void Post();
  void Poke();

  static constexpr char kName[] = "SemWaiter";

 private:
  int TimedWait(KernelTimeout t);

  sem_t sem_;

  // This seems superfluous, but for Poke() we need to cause spurious
  // wakeups on the semaphore. Hence we can't actually use the
  // semaphore's count.
  std::atomic<int> wakeups_;
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_HAVE_SEMAPHORE_H

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_SEM_WAITER_H_
