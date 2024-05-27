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

#include <turbo/log/internal/conditions.h>

#include <atomic>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/internal/cycleclock.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
namespace {

// The following code behaves like AtomicStatsCounter::LossyAdd() for
// speed since it is fine to lose occasional updates.
// Returns old value of *counter.
uint32_t LossyIncrement(std::atomic<uint32_t>* counter) {
  const uint32_t value = counter->load(std::memory_order_relaxed);
  counter->store(value + 1, std::memory_order_relaxed);
  return value;
}

}  // namespace

bool LogEveryNState::ShouldLog(int n) {
  return n > 0 && (LossyIncrement(&counter_) % static_cast<uint32_t>(n)) == 0;
}

bool LogFirstNState::ShouldLog(int n) {
  const uint32_t counter_value = counter_.load(std::memory_order_relaxed);
  if (static_cast<int64_t>(counter_value) < n) {
    counter_.store(counter_value + 1, std::memory_order_relaxed);
    return true;
  }
  return false;
}

bool LogEveryPow2State::ShouldLog() {
  const uint32_t new_value = LossyIncrement(&counter_) + 1;
  return (new_value & (new_value - 1)) == 0;
}

bool LogEveryNSecState::ShouldLog(double seconds) {
  using turbo::base_internal::CycleClock;
  LossyIncrement(&counter_);
  const int64_t now_cycles = CycleClock::Now();
  int64_t next_cycles = next_log_time_cycles_.load(std::memory_order_relaxed);
#if defined(__myriad2__)
  // myriad2 does not have 8-byte compare and exchange.  Use a racy version that
  // is "good enough" but will over-log in the face of concurrent logging.
  if (now_cycles > next_cycles) {
    next_log_time_cycles_.store(now_cycles + seconds * CycleClock::Frequency(),
                                std::memory_order_relaxed);
    return true;
  }
  return false;
#else
  do {
    if (now_cycles <= next_cycles) return false;
  } while (!next_log_time_cycles_.compare_exchange_weak(
      next_cycles, now_cycles + seconds * CycleClock::Frequency(),
      std::memory_order_relaxed, std::memory_order_relaxed));
  return true;
#endif
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
