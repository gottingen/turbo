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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_PTHREAD_WAITER_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_PTHREAD_WAITER_H_

#if !defined(_WIN32) && !defined(__MINGW32__)
#include <pthread.h>

#include <turbo/base/config.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/waiter_base.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#define TURBO_INTERNAL_HAVE_PTHREAD_WAITER 1

class PthreadWaiter : public WaiterCrtp<PthreadWaiter> {
 public:
  PthreadWaiter();

  bool Wait(KernelTimeout t);
  void Post();
  void Poke();

  static constexpr char kName[] = "PthreadWaiter";

 private:
  int TimedWait(KernelTimeout t);

  // REQUIRES: mu_ must be held.
  void InternalCondVarPoke();

  pthread_mutex_t mu_;
  pthread_cond_t cv_;
  int waiter_count_;
  int wakeup_count_;  // Unclaimed wakeups.
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // !defined(_WIN32) && !defined(__MINGW32__)

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_PTHREAD_WAITER_H_
