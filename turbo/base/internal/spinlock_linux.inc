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
// This file is a Linux-specific part of spinlock_wait.cc

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <climits>
#include <cstdint>
#include <ctime>

#include <turbo/base/attributes.h>
#include <turbo/base/internal/errno_saver.h>

// The SpinLock lockword is `std::atomic<uint32_t>`. Here we assert that
// `std::atomic<uint32_t>` is bitwise equivalent of the `int` expected
// by SYS_futex. We also assume that reads/writes done to the lockword
// by SYS_futex have rational semantics with regard to the
// std::atomic<> API. C++ provides no guarantees of these assumptions,
// but they are believed to hold in practice.
static_assert(sizeof(std::atomic<uint32_t>) == sizeof(int),
              "SpinLock lockword has the wrong size for a futex");

// Some Android headers are missing these definitions even though they
// support these futex operations.
#ifdef __BIONIC__
#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#endif

#if defined(__NR_futex_time64) && !defined(SYS_futex_time64)
#define SYS_futex_time64 __NR_futex_time64
#endif

#if defined(SYS_futex_time64) && !defined(SYS_futex)
#define SYS_futex SYS_futex_time64
#endif

extern "C" {

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockDelay)(
    std::atomic<uint32_t> *w, uint32_t value, int,
    turbo::base_internal::SchedulingMode) {
  turbo::base_internal::ErrnoSaver errno_saver;
  syscall(SYS_futex, w, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, value, nullptr);
}

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalSpinLockWake)(
    std::atomic<uint32_t> *w, bool all) {
  syscall(SYS_futex, w, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, all ? INT_MAX : 1, 0);
}

}  // extern "C"
