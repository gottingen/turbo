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
// This file is a Win32-specific part of spinlock_wait.cc

#include <windows.h>
#include <atomic>
#include <turbo/base/internal/scheduling_mode.h>

extern "C" {

void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockDelay)(
    std::atomic<uint32_t>* /* lock_word */, uint32_t /* value */, int loop,
    turbo::base_internal::SchedulingMode /* mode */) {
  if (loop == 0) {
  } else if (loop == 1) {
    Sleep(0);
  } else {
    // SpinLockSuggestedDelayNS() always returns a positive integer, so this
    // static_cast is safe.
    Sleep(static_cast<DWORD>(
        turbo::base_internal::SpinLockSuggestedDelayNS(loop) / 1000000));
  }
}

void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockWake)(
    std::atomic<uint32_t>* /* lock_word */, bool /* all */) {}

}  // extern "C"
