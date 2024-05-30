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

#include <turbo/synchronization/internal/pthread_waiter.h>

#ifdef TURBO_INTERNAL_HAVE_PTHREAD_WAITER

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/base/optimization.h>
#include <turbo/synchronization/internal/kernel_timeout.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

namespace {
class PthreadMutexHolder {
 public:
  explicit PthreadMutexHolder(pthread_mutex_t *mu) : mu_(mu) {
    const int err = pthread_mutex_lock(mu_);
    if (err != 0) {
      TURBO_RAW_LOG(FATAL, "pthread_mutex_lock failed: %d", err);
    }
  }

  PthreadMutexHolder(const PthreadMutexHolder &rhs) = delete;
  PthreadMutexHolder &operator=(const PthreadMutexHolder &rhs) = delete;

  ~PthreadMutexHolder() {
    const int err = pthread_mutex_unlock(mu_);
    if (err != 0) {
      TURBO_RAW_LOG(FATAL, "pthread_mutex_unlock failed: %d", err);
    }
  }

 private:
  pthread_mutex_t *mu_;
};
}  // namespace

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr char PthreadWaiter::kName[];
#endif

PthreadWaiter::PthreadWaiter() : waiter_count_(0), wakeup_count_(0) {
  const int err = pthread_mutex_init(&mu_, 0);
  if (err != 0) {
    TURBO_RAW_LOG(FATAL, "pthread_mutex_init failed: %d", err);
  }

  const int err2 = pthread_cond_init(&cv_, 0);
  if (err2 != 0) {
    TURBO_RAW_LOG(FATAL, "pthread_cond_init failed: %d", err2);
  }
}

#ifdef __APPLE__
#define TURBO_INTERNAL_HAS_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP 1
#endif

#if defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30))
#define TURBO_INTERNAL_HAVE_PTHREAD_COND_CLOCKWAIT 1
#elif defined(__ANDROID_API__) && __ANDROID_API__ >= 30
#define TURBO_INTERNAL_HAVE_PTHREAD_COND_CLOCKWAIT 1
#endif

// Calls pthread_cond_timedwait() or possibly something else like
// pthread_cond_timedwait_relative_np() depending on the platform and
// KernelTimeout requested. The return value is the same as the return
// value of pthread_cond_timedwait().
int PthreadWaiter::TimedWait(KernelTimeout t) {
  assert(t.has_timeout());
  if (KernelTimeout::SupportsSteadyClock() && t.is_relative_timeout()) {
#ifdef TURBO_INTERNAL_HAS_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP
    const auto rel_timeout = t.MakeRelativeTimespec();
    return pthread_cond_timedwait_relative_np(&cv_, &mu_, &rel_timeout);
#elif defined(TURBO_INTERNAL_HAVE_PTHREAD_COND_CLOCKWAIT) && \
    defined(CLOCK_MONOTONIC)
    const auto abs_clock_timeout = t.MakeClockAbsoluteTimespec(CLOCK_MONOTONIC);
    return pthread_cond_clockwait(&cv_, &mu_, CLOCK_MONOTONIC,
                                  &abs_clock_timeout);
#endif
  }

  const auto abs_timeout = t.MakeAbsTimespec();
  return pthread_cond_timedwait(&cv_, &mu_, &abs_timeout);
}

bool PthreadWaiter::Wait(KernelTimeout t) {
  PthreadMutexHolder h(&mu_);
  ++waiter_count_;
  // Loop until we find a wakeup to consume or timeout.
  // Note that, since the thread ticker is just reset, we don't need to check
  // whether the thread is idle on the very first pass of the loop.
  bool first_pass = true;
  while (wakeup_count_ == 0) {
    if (!first_pass) MaybeBecomeIdle();
    // No wakeups available, time to wait.
    if (!t.has_timeout()) {
      const int err = pthread_cond_wait(&cv_, &mu_);
      if (err != 0) {
        TURBO_RAW_LOG(FATAL, "pthread_cond_wait failed: %d", err);
      }
    } else {
      const int err = TimedWait(t);
      if (err == ETIMEDOUT) {
        --waiter_count_;
        return false;
      }
      if (err != 0) {
        TURBO_RAW_LOG(FATAL, "PthreadWaiter::TimedWait() failed: %d", err);
      }
    }
    first_pass = false;
  }
  // Consume a wakeup and we're done.
  --wakeup_count_;
  --waiter_count_;
  return true;
}

void PthreadWaiter::Post() {
  PthreadMutexHolder h(&mu_);
  ++wakeup_count_;
  InternalCondVarPoke();
}

void PthreadWaiter::Poke() {
  PthreadMutexHolder h(&mu_);
  InternalCondVarPoke();
}

void PthreadWaiter::InternalCondVarPoke() {
  if (waiter_count_ != 0) {
    const int err = pthread_cond_signal(&cv_);
    if (TURBO_UNLIKELY(err != 0)) {
      TURBO_RAW_LOG(FATAL, "pthread_cond_signal failed: %d", err);
    }
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_PTHREAD_WAITER
