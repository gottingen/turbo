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
#ifndef TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_H_

#include <turbo/base/config.h>

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <atomic>
#include <cstdint>
#include <limits>

#include <turbo/base/optimization.h>
#include <turbo/synchronization/internal/kernel_timeout.h>

#ifdef TURBO_INTERNAL_HAVE_FUTEX
#error TURBO_INTERNAL_HAVE_FUTEX may not be set on the command line
#elif defined(__BIONIC__)
// Bionic supports all the futex operations we need even when some of the futex
// definitions are missing.
#define TURBO_INTERNAL_HAVE_FUTEX
#elif defined(__linux__) && defined(FUTEX_CLOCK_REALTIME)
// FUTEX_CLOCK_REALTIME requires Linux >= 2.6.28.
#define TURBO_INTERNAL_HAVE_FUTEX
#endif

#ifdef TURBO_INTERNAL_HAVE_FUTEX

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

// Some Android headers are missing these definitions even though they
// support these futex operations.
#ifdef __BIONIC__
#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif
#ifndef FUTEX_WAIT_BITSET
#define FUTEX_WAIT_BITSET 9
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#ifndef FUTEX_CLOCK_REALTIME
#define FUTEX_CLOCK_REALTIME 256
#endif
#ifndef FUTEX_BITSET_MATCH_ANY
#define FUTEX_BITSET_MATCH_ANY 0xFFFFFFFF
#endif
#endif

#if defined(__NR_futex_time64) && !defined(SYS_futex_time64)
#define SYS_futex_time64 __NR_futex_time64
#endif

#if defined(SYS_futex_time64) && !defined(SYS_futex)
#define SYS_futex SYS_futex_time64
using FutexTimespec = struct timespec;
#else
// Some libc implementations have switched to an unconditional 64-bit `time_t`
// definition. This means that `struct timespec` may not match the layout
// expected by the kernel ABI on 32-bit platforms. So we define the
// FutexTimespec that matches the kernel timespec definition. It should be safe
// to use this struct for 64-bit userspace builds too, since it will use another
// SYS_futex kernel call with 64-bit tv_sec inside timespec.
struct FutexTimespec {
  long tv_sec;   // NOLINT
  long tv_nsec;  // NOLINT
};
#endif

class FutexImpl {
 public:
  // Atomically check that `*v == val`, and if it is, then sleep until the until
  // woken by `Wake()`.
  static int Wait(std::atomic<int32_t>* v, int32_t val) {
    return WaitAbsoluteTimeout(v, val, nullptr);
  }

  // Atomically check that `*v == val`, and if it is, then sleep until
  // CLOCK_REALTIME reaches `*abs_timeout`, or until woken by `Wake()`.
  static int WaitAbsoluteTimeout(std::atomic<int32_t>* v, int32_t val,
                                 const struct timespec* abs_timeout) {
    FutexTimespec ts;
    // https://locklessinc.com/articles/futex_cheat_sheet/
    // Unlike FUTEX_WAIT, FUTEX_WAIT_BITSET uses absolute time.
    auto err = syscall(
        SYS_futex, reinterpret_cast<int32_t*>(v),
        FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME, val,
        ToFutexTimespec(abs_timeout, &ts), nullptr, FUTEX_BITSET_MATCH_ANY);
    if (err != 0) {
      return -errno;
    }
    return 0;
  }

  // Atomically check that `*v == val`, and if it is, then sleep until
  // `*rel_timeout` has elapsed, or until woken by `Wake()`.
  static int WaitRelativeTimeout(std::atomic<int32_t>* v, int32_t val,
                                 const struct timespec* rel_timeout) {
    FutexTimespec ts;
    // Atomically check that the futex value is still 0, and if it
    // is, sleep until abs_timeout or until woken by FUTEX_WAKE.
    auto err =
        syscall(SYS_futex, reinterpret_cast<int32_t*>(v), FUTEX_PRIVATE_FLAG,
                val, ToFutexTimespec(rel_timeout, &ts));
    if (err != 0) {
      return -errno;
    }
    return 0;
  }

  // Wakes at most `count` waiters that have entered the sleep state on `v`.
  static int Wake(std::atomic<int32_t>* v, int32_t count) {
    auto err = syscall(SYS_futex, reinterpret_cast<int32_t*>(v),
                       FUTEX_WAKE | FUTEX_PRIVATE_FLAG, count);
    if (TURBO_UNLIKELY(err < 0)) {
      return -errno;
    }
    return 0;
  }

 private:
  static FutexTimespec* ToFutexTimespec(const struct timespec* userspace_ts,
                                        FutexTimespec* futex_ts) {
    if (userspace_ts == nullptr) {
      return nullptr;
    }

    using FutexSeconds = decltype(futex_ts->tv_sec);
    using FutexNanoseconds = decltype(futex_ts->tv_nsec);

    constexpr auto kMaxSeconds{(std::numeric_limits<FutexSeconds>::max)()};
    if (userspace_ts->tv_sec > kMaxSeconds) {
      futex_ts->tv_sec = kMaxSeconds;
    } else {
      futex_ts->tv_sec = static_cast<FutexSeconds>(userspace_ts->tv_sec);
    }
    futex_ts->tv_nsec = static_cast<FutexNanoseconds>(userspace_ts->tv_nsec);
    return futex_ts;
  }
};

class Futex : public FutexImpl {};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_FUTEX

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_H_
