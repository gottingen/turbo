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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_WIN32_WAITER_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_WIN32_WAITER_H_

#ifdef _WIN32
#include <sdkddkver.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__) && \
    _WIN32_WINNT >= _WIN32_WINNT_VISTA

#include <turbo/base/config.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/waiter_base.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#define TURBO_INTERNAL_HAVE_WIN32_WAITER 1

class Win32Waiter : public WaiterCrtp<Win32Waiter> {
 public:
  Win32Waiter();

  bool Wait(KernelTimeout t);
  void Post();
  void Poke();

  static constexpr char kName[] = "Win32Waiter";

 private:
  // WinHelper - Used to define utilities for accessing the lock and
  // condition variable storage once the types are complete.
  class WinHelper;

  // REQUIRES: WinHelper::GetLock(this) must be held.
  void InternalCondVarPoke();

  // We can't include Windows.h in our headers, so we use aligned character
  // buffers to define the storage of SRWLOCK and CONDITION_VARIABLE.
  // SRW locks and condition variables do not need to be explicitly destroyed.
  // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializesrwlock
  // https://stackoverflow.com/questions/28975958/why-does-windows-have-no-deleteconditionvariable-function-to-go-together-with
  alignas(void*) unsigned char mu_storage_[sizeof(void*)];
  alignas(void*) unsigned char cv_storage_[sizeof(void*)];
  int waiter_count_;
  int wakeup_count_;
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // defined(_WIN32) && !defined(__MINGW32__) &&
        // _WIN32_WINNT >= _WIN32_WINNT_VISTA

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_WIN32_WAITER_H_
