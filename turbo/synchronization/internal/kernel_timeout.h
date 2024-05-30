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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_KERNEL_TIMEOUT_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_KERNEL_TIMEOUT_H_

#ifndef _WIN32
#include <sys/types.h>
#endif

#include <algorithm>
#include <chrono>  // NOLINT(build/c++11)
#include <cstdint>
#include <ctime>
#include <limits>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/times/clock.h>
#include <turbo/times/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

// An optional timeout, with nanosecond granularity.
//
// This is a private low-level API for use by a handful of low-level
// components. Higher-level components should build APIs based on
// turbo::Time and turbo::Duration.
class KernelTimeout {
 public:
  // Construct an absolute timeout that should expire at `t`.
  explicit KernelTimeout(turbo::Time t);

  // Construct a relative timeout that should expire after `d`.
  explicit KernelTimeout(turbo::Duration d);

  // Infinite timeout.
  constexpr KernelTimeout() : rep_(kNoTimeout) {}

  // A more explicit factory for those who prefer it.
  // Equivalent to `KernelTimeout()`.
  static constexpr KernelTimeout Never() { return KernelTimeout(); }

  // Returns true if there is a timeout that will eventually expire.
  // Returns false if the timeout is infinite.
  bool has_timeout() const { return rep_ != kNoTimeout; }

  // If `has_timeout()` is true, returns true if the timeout was provided as an
  // `turbo::Time`. The return value is undefined if `has_timeout()` is false
  // because all indefinite timeouts are equivalent.
  bool is_absolute_timeout() const { return (rep_ & 1) == 0; }

  // If `has_timeout()` is true, returns true if the timeout was provided as an
  // `turbo::Duration`. The return value is undefined if `has_timeout()` is false
  // because all indefinite timeouts are equivalent.
  bool is_relative_timeout() const { return (rep_ & 1) == 1; }

  // Convert to `struct timespec` for interfaces that expect an absolute
  // timeout. If !has_timeout() or is_relative_timeout(), attempts to convert to
  // a reasonable absolute timeout, but callers should to test has_timeout() and
  // is_relative_timeout() and prefer to use a more appropriate interface.
  struct timespec MakeAbsTimespec() const;

  // Convert to `struct timespec` for interfaces that expect a relative
  // timeout. If !has_timeout() or is_absolute_timeout(), attempts to convert to
  // a reasonable relative timeout, but callers should to test has_timeout() and
  // is_absolute_timeout() and prefer to use a more appropriate interface. Since
  // the return value is a relative duration, it should be recomputed by calling
  // this method in the case of a spurious wakeup.
  struct timespec MakeRelativeTimespec() const;

#ifndef _WIN32
  // Convert to `struct timespec` for interfaces that expect an absolute timeout
  // on a specific clock `c`. This is similar to `MakeAbsTimespec()`, but
  // callers usually want to use this method with `CLOCK_MONOTONIC` when
  // relative timeouts are requested, and when the appropriate interface expects
  // an absolute timeout relative to a specific clock (for example,
  // pthread_cond_clockwait() or sem_clockwait()). If !has_timeout(), attempts
  // to convert to a reasonable absolute timeout, but callers should to test
  // has_timeout() prefer to use a more appropriate interface.
  struct timespec MakeClockAbsoluteTimespec(clockid_t c) const;
#endif

  // Convert to unix epoch nanos for interfaces that expect an absolute timeout
  // in nanoseconds. If !has_timeout() or is_relative_timeout(), attempts to
  // convert to a reasonable absolute timeout, but callers should to test
  // has_timeout() and is_relative_timeout() and prefer to use a more
  // appropriate interface.
  int64_t MakeAbsNanos() const;

  // Converts to milliseconds from now, or INFINITE when
  // !has_timeout(). For use by SleepConditionVariableSRW on
  // Windows. Callers should recognize that the return value is a
  // relative duration (it should be recomputed by calling this method
  // in the case of a spurious wakeup).
  // This header file may be included transitively by public header files,
  // so we define our own DWORD and INFINITE instead of getting them from
  // <intsafe.h> and <WinBase.h>.
  typedef unsigned long DWord;  // NOLINT
  DWord InMillisecondsFromNow() const;

  // Convert to std::chrono::time_point for interfaces that expect an absolute
  // timeout, like std::condition_variable::wait_until(). If !has_timeout() or
  // is_relative_timeout(), attempts to convert to a reasonable absolute
  // timeout, but callers should test has_timeout() and is_relative_timeout()
  // and prefer to use a more appropriate interface.
  std::chrono::time_point<std::chrono::system_clock> ToChronoTimePoint() const;

  // Convert to std::chrono::time_point for interfaces that expect a relative
  // timeout, like std::condition_variable::wait_for(). If !has_timeout() or
  // is_absolute_timeout(), attempts to convert to a reasonable relative
  // timeout, but callers should test has_timeout() and is_absolute_timeout()
  // and prefer to use a more appropriate interface. Since the return value is a
  // relative duration, it should be recomputed by calling this method in the
  // case of a spurious wakeup.
  std::chrono::nanoseconds ToChronoDuration() const;

  // Returns true if steady (aka monotonic) clocks are supported by the system.
  // This method exists because go/btm requires synchronized clocks, and
  // thus requires we use the system (aka walltime) clock.
  static constexpr bool SupportsSteadyClock() { return true; }

 private:
  // Returns the current time, expressed as a count of nanoseconds since the
  // epoch used by an arbitrary clock. The implementation tries to use a steady
  // (monotonic) clock if one is available.
  static int64_t SteadyClockNow();

  // Internal representation.
  //   - If the value is kNoTimeout, then the timeout is infinite, and
  //     has_timeout() will return true.
  //   - If the low bit is 0, then the high 63 bits is the number of nanoseconds
  //     after the unix epoch.
  //   - If the low bit is 1, then the high 63 bits is the number of nanoseconds
  //     after the epoch used by SteadyClockNow().
  //
  // In all cases the time is stored as an absolute time, the only difference is
  // the clock epoch. The use of absolute times is important since in the case
  // of a relative timeout with a spurious wakeup, the program would have to
  // restart the wait, and thus needs a way of recomputing the remaining time.
  uint64_t rep_;

  // Returns the number of nanoseconds stored in the internal representation.
  // When combined with the clock epoch indicated by the low bit (which is
  // accessed through is_absolute_timeout() and is_relative_timeout()), the
  // return value is used to compute when the timeout should occur.
  int64_t RawAbsNanos() const { return static_cast<int64_t>(rep_ >> 1); }

  // Converts to nanoseconds from now. Since the return value is a relative
  // duration, it should be recomputed by calling this method in the case of a
  // spurious wakeup.
  int64_t InNanosecondsFromNow() const;

  // A value that represents no timeout (or an infinite timeout).
  static constexpr uint64_t kNoTimeout = (std::numeric_limits<uint64_t>::max)();

  // The maximum value that can be stored in the high 63 bits.
  static constexpr int64_t kMaxNanos = (std::numeric_limits<int64_t>::max)();
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_KERNEL_TIMEOUT_H_
