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
// This file is an Akaros-specific part of spinlock_wait.cc

#include <atomic>

#include <turbo/base/internal/scheduling_mode.h>

extern "C" {

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockDelay)(
    std::atomic<uint32_t>* /* lock_word */, uint32_t /* value */,
    int /* loop */, turbo::base_internal::SchedulingMode /* mode */) {
  // In Akaros, one must take care not to call anything that could cause a
  // malloc(), a blocking system call, or a uthread_yield() while holding a
  // spinlock. Our callers assume will not call into libraries or other
  // arbitrary code.
}

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockWake)(
    std::atomic<uint32_t>* /* lock_word */, bool /* all */) {}

}  // extern "C"
