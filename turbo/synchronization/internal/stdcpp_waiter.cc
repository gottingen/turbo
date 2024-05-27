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

#include <turbo/synchronization/internal/stdcpp_waiter.h>

#ifdef TURBO_INTERNAL_HAVE_STDCPP_WAITER

#include <chrono>  // NOLINT(build/c++11)
#include <condition_variable>  // NOLINT(build/c++11)
#include <mutex>  // NOLINT(build/c++11)

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/base/optimization.h>
#include <turbo/synchronization/internal/kernel_timeout.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr char StdcppWaiter::kName[];
#endif

StdcppWaiter::StdcppWaiter() : waiter_count_(0), wakeup_count_(0) {}

bool StdcppWaiter::Wait(KernelTimeout t) {
  std::unique_lock<std::mutex> lock(mu_);
  ++waiter_count_;

  // Loop until we find a wakeup to consume or timeout.
  // Note that, since the thread ticker is just reset, we don't need to check
  // whether the thread is idle on the very first pass of the loop.
  bool first_pass = true;
  while (wakeup_count_ == 0) {
    if (!first_pass) MaybeBecomeIdle();
    // No wakeups available, time to wait.
    if (!t.has_timeout()) {
      cv_.wait(lock);
    } else {
      auto wait_result = t.SupportsSteadyClock() && t.is_relative_timeout()
                             ? cv_.wait_for(lock, t.ToChronoDuration())
                             : cv_.wait_until(lock, t.ToChronoTimePoint());
      if (wait_result == std::cv_status::timeout) {
        --waiter_count_;
        return false;
      }
    }
    first_pass = false;
  }

  // Consume a wakeup and we're done.
  --wakeup_count_;
  --waiter_count_;
  return true;
}

void StdcppWaiter::Post() {
  std::lock_guard<std::mutex> lock(mu_);
  ++wakeup_count_;
  InternalCondVarPoke();
}

void StdcppWaiter::Poke() {
  std::lock_guard<std::mutex> lock(mu_);
  InternalCondVarPoke();
}

void StdcppWaiter::InternalCondVarPoke() {
  if (waiter_count_ != 0) {
    cv_.notify_one();
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_STDCPP_WAITER
